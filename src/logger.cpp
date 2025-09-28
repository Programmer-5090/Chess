#include "logger.h"
#include "perfProfiler.h"
#include <fstream>
#ifdef INFO
#undef INFO
#endif
#ifdef ERROR
#undef ERROR
#endif
#ifdef WARN
#undef WARN
#endif
#ifdef DEBUG
#undef DEBUG
#endif
#include <mutex>
#include <sstream>
#include <string>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <iostream>
#include <thread>

std::ofstream Logger::s_stream;
std::mutex Logger::s_mutex;
bool Logger::s_initialized = false;
std::streambuf* Logger::s_oldCerrBuf = nullptr;
std::streambuf* Logger::s_oldCoutBuf = nullptr;
std::string Logger::s_currentLogFile;
LogLevel Logger::s_minLevel = LogLevel::INFO;
size_t Logger::s_maxFileSize = 50 * 1024 * 1024; // 50MB default
bool Logger::s_redirectStdStreams = true;
bool Logger::s_silent = false;

void Logger::init(const std::string& logDir, LogLevel minLevel, bool redirectStreams, size_t maxFileSizeMB) {
    bool didInit = false;
    try {
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            if (s_initialized) {
                // Avoid calling log() while holding the mutex (would deadlock).
                std::cerr << "Logger already initialized" << std::endl;
                return;
            }

            s_minLevel = minLevel;
            s_redirectStdStreams = redirectStreams;
            s_maxFileSize = maxFileSizeMB * 1024 * 1024;

            std::filesystem::create_directories(logDir);

            // Create unique filename with timestamp and process ID
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif

            // Get process ID for uniqueness
            auto pid = std::hash<std::thread::id>{}(std::this_thread::get_id());

            std::ostringstream filename;
            filename << logDir << "/log_"
                     << (tm.tm_year + 1900)
                     << std::setw(2) << std::setfill('0') << (tm.tm_mon + 1)
                     << std::setw(2) << std::setfill('0') << tm.tm_mday
                     << "_"
                     << std::setw(2) << std::setfill('0') << tm.tm_hour
                     << std::setw(2) << std::setfill('0') << tm.tm_min
                     << std::setw(2) << std::setfill('0') << tm.tm_sec
                     << "_"
                     << std::setw(3) << std::setfill('0') << ms.count()
                     << "_"
                     << std::hex << (pid & 0xFFFF)
                     << ".log";

            s_currentLogFile = filename.str();
            s_stream.open(s_currentLogFile, std::ios::app);

            if (!s_stream.is_open()) {
                std::cerr << "Logger: Failed to open log file " << s_currentLogFile << std::endl;
                return;
            }

            // Write startup header
            writeHeader();

            // Optionally redirect standard streams
            if (s_redirectStdStreams) {
                s_oldCerrBuf = std::cerr.rdbuf();
                s_oldCoutBuf = std::cout.rdbuf();
                std::cerr.rdbuf(s_stream.rdbuf());
                std::cout.rdbuf(s_stream.rdbuf());
            }

            s_initialized = true;
            didInit = true;
        } // release lock here before invoking log()

        if (didInit) {
            // Safe to call log now; mutex is not held by this thread
            log(LogLevel::INFO, "Logger initialized successfully. Log file: " + s_currentLogFile, __FILE__, __LINE__);
        }

    } catch (const std::exception& e) {
        std::cerr << "Logger init exception: " << e.what() << std::endl;
    }
}

void Logger::shutdown() {
    // We'll set initialized=false under lock but call log() outside the lock
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_initialized) {
            return;
        }
        // flip initialized first so log() will take the fallback path
        s_initialized = false;
    }

    // Safe to call log now; it will honor s_initialized==false and print to stderr
    log(LogLevel::INFO, "Logger shutting down", __FILE__, __LINE__);

    {
        std::lock_guard<std::mutex> lock2(s_mutex);
        if (s_stream.is_open()) {
            // Write shutdown footer
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            s_stream << std::endl << "=== Logger shutdown at " 
                     << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") 
                     << " ===" << std::endl << std::endl;

            s_stream.flush();

            // Restore original stream buffers before closing
            if (s_redirectStdStreams) {
                if (s_oldCerrBuf) std::cerr.rdbuf(s_oldCerrBuf);
                if (s_oldCoutBuf) std::cout.rdbuf(s_oldCoutBuf);
            }

            s_stream.close();
        }

        // Prevent any further log calls (for example from destructors)
        // from writing to the restored std::cerr/std::cout by silencing the logger.
        s_silent = true;

        s_currentLogFile.clear();
    }
}

void Logger::log(LogLevel level, const std::string& msg, const char* file, int line) {
    // Measure Logger::log overhead
    g_profiler.startTimer("logger_log_total");
    std::lock_guard<std::mutex> lock(s_mutex);

    // Silent mode: do nothing
    if (s_silent) {
        g_profiler.endTimer("logger_log_total");
        return;
    }

    // Check if we should log this level
    if (level < s_minLevel) {
        return;
    }
    
    // Check file size and rotate if necessary
    if (s_initialized && s_stream.is_open()) {
        checkAndRotateLog();
    }
    
    // Get current timestamp with milliseconds
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    
    // Format timestamp
    std::ostringstream timestamp;
    timestamp << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") 
              << "." << std::setw(3) << std::setfill('0') << ms.count();
    
    // Get level string with color codes for console output
    const char* levelStr = getLevelString(level);
    const char* colorCode = getColorCode(level);
    const char* resetColor = "\033[0m";
    
    // Extract just the filename from the full path
    std::string filename = extractFilename(file);
    
    // Format the complete log message
    std::ostringstream logMessage;
    logMessage << timestamp.str() << " [" << levelStr << "] " 
               << msg << " (" << filename << ":" << line << ")";
    
    // Write to file if initialized
    if (s_initialized && s_stream.is_open()) {
        s_stream << logMessage.str() << std::endl;
        s_stream.flush();
    } else {
        // Fallback to console with colors if not redirected
        if (!s_redirectStdStreams) {
            std::cerr << colorCode << logMessage.str() << resetColor << std::endl;
        } else {
            std::cerr << logMessage.str() << std::endl;
        }
    }

    g_profiler.endTimer("logger_log_total");
}

void Logger::setMinLevel(LogLevel level) {
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_minLevel = level;
    }
    std::string msg = "Log level changed to ";
    msg += getLevelString(level);
    log(LogLevel::INFO, msg, __FILE__, __LINE__);
}

void Logger::setSilent(bool silent) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_silent = silent;
}

bool Logger::isSilent() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_silent;
}

LogLevel Logger::getMinLevel() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_minLevel;
}

std::string Logger::getCurrentLogFile() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_currentLogFile;
}

bool Logger::isInitialized() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_initialized;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_initialized && s_stream.is_open()) {
        s_stream.flush();
    }
}

const char* Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default:              return "UNKN ";
    }
}

const char* Logger::getColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "\033[36m"; // Cyan
        case LogLevel::INFO:  return "\033[32m"; // Green
        case LogLevel::WARN:  return "\033[33m"; // Yellow
        case LogLevel::ERROR: return "\033[31m"; // Red
        default:              return "\033[0m";  // Reset
    }
}

std::string Logger::extractFilename(const char* path) {
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("/\\");
    if (pos != std::string::npos) {
        return fullPath.substr(pos + 1);
    }
    return fullPath;
}

void Logger::writeHeader() {
    if (!s_stream.is_open()) return;
    
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    
    s_stream << "=== Logger started at " 
             << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") 
             << " ===" << std::endl;
    s_stream << "Log file: " << s_currentLogFile << std::endl;
    s_stream << "Min level: " << getLevelString(s_minLevel) << std::endl;
    s_stream << "Max file size: " << (s_maxFileSize / 1024 / 1024) << " MB" << std::endl;
    s_stream << "Stream redirection: " << (s_redirectStdStreams ? "enabled" : "disabled") << std::endl;
    s_stream << "========================================" << std::endl << std::endl;
    s_stream.flush();
}

void Logger::checkAndRotateLog() {
    if (!s_stream.is_open()) return;
    
    try {
        // Get current file size
        s_stream.flush();
        std::filesystem::path logPath(s_currentLogFile);
        
        if (std::filesystem::exists(logPath)) {
            size_t fileSize = std::filesystem::file_size(logPath);
            
            if (fileSize >= s_maxFileSize) {
                // Close current file
                s_stream.close();
                
                // Create new filename with rotation suffix
                std::string baseName = s_currentLogFile.substr(0, s_currentLogFile.find_last_of('.'));
                std::string extension = s_currentLogFile.substr(s_currentLogFile.find_last_of('.'));
                
                // Find next available rotation number
                int rotationNum = 1;
                std::string rotatedName;
                do {
                    std::ostringstream oss;
                    oss << baseName << "_part" << std::setw(3) << std::setfill('0') << rotationNum << extension;
                    rotatedName = oss.str();
                    rotationNum++;
                } while (std::filesystem::exists(rotatedName));
                
                s_currentLogFile = rotatedName;
                s_stream.open(s_currentLogFile, std::ios::app);
                
                if (s_stream.is_open()) {
                    writeHeader();
                    log(LogLevel::INFO, "Log rotated to new file: " + s_currentLogFile, __FILE__, __LINE__);
                }
            }
        }
    } catch (const std::exception& e) {
        // If rotation fails, continue with current file
        if (!s_stream.is_open()) {
            s_stream.open(s_currentLogFile, std::ios::app);
        }
    }
}