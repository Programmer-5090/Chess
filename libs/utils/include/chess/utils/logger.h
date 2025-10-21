#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <exception>
#include <cstdio>
#include <filesystem>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

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

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

class Logger {
public:
    static void init(const std::string& logDir, 
                    LogLevel minLevel = LogLevel::INFO,
                    bool redirectStreams = true,
                    size_t maxFileSizeMB = 50);
    
    // Shutdown the logger
    static void shutdown();
    
    // Log a message with level, file and line
    static void log(LogLevel level, const std::string& msg, const char* file, int line);
    
    // Set minimum log level at runtime
    static void setMinLevel(LogLevel level);
    
    // Get current minimum log level
    static LogLevel getMinLevel();
    
    // Get current log file path
    static std::string getCurrentLogFile();
    
    // Check whether logger was initialized
    static bool isInitialized();
    
    // Force flush log buffer
    static void flush();

    // Silent mode: suppresses logging output
    static void setSilent(bool silent);
    static bool isSilent();

private:
    static std::ofstream s_stream;
    static std::mutex s_mutex;
    static bool s_initialized;
    static std::streambuf* s_oldCerrBuf;
    static std::streambuf* s_oldCoutBuf;
    static std::string s_currentLogFile;
    static LogLevel s_minLevel;
    static size_t s_maxFileSize;
    static bool s_redirectStdStreams;
    static bool s_silent;
    
    // Helper methods (internal)
    static const char* getLevelString(LogLevel level);
    static const char* getColorCode(LogLevel level);
    static std::string extractFilename(const char* path);
    static void writeHeader();
    static void checkAndRotateLog();
};

// Convenience macros for easier logging
#define LOG_DEBUG(msg) Logger::log(LogLevel::DEBUG, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  Logger::log(LogLevel::INFO, msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  Logger::log(LogLevel::WARN, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::log(LogLevel::ERROR, msg, __FILE__, __LINE__)

// Formatted logging macros
#define LOG_DEBUG_F(fmt, ...) do { \
    char buffer[1024]; \
    snprintf(buffer, sizeof(buffer), fmt, __VA_ARGS__); \
    Logger::log(LogLevel::DEBUG, std::string(buffer), __FILE__, __LINE__); \
} while(0)

#define LOG_INFO_F(fmt, ...) do { \
    char buffer[1024]; \
    snprintf(buffer, sizeof(buffer), fmt, __VA_ARGS__); \
    Logger::log(LogLevel::INFO, std::string(buffer), __FILE__, __LINE__); \
} while(0)

#define LOG_WARN_F(fmt, ...) do { \
    char buffer[1024]; \
    snprintf(buffer, sizeof(buffer), fmt, __VA_ARGS__); \
    Logger::log(LogLevel::WARN, std::string(buffer), __FILE__, __LINE__); \
} while(0)

#define LOG_ERROR_F(fmt, ...) do { \
    char buffer[1024]; \
    snprintf(buffer, sizeof(buffer), fmt, __VA_ARGS__); \
    Logger::log(LogLevel::ERROR, std::string(buffer), __FILE__, __LINE__); \
} while(0)

#endif // LOGGER_H
