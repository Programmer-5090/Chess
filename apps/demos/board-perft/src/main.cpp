#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <csignal>
#include <future>
#include <mutex>

#include <chess/utils/logger.h>
#include <chess/utils/profiler.h>
#include <chess/rendering/texture_cache.h>
#include <chess/utils/thread_pool.h>
#include <chess/board/board.h>
#include <chess/board/pieces/piece.h>
#include <chess/board/piece_manager.h>
#include <chess/AI/utils.h>

struct PerftState {
    Board* board;
    Color sideToMove;
};

static bool g_enableBulkCount = true;

// Move filter for testing specific moves (e.g., "e2e4")
static std::string onlyMoveGlobal = "";
static std::string moveToString(const Move& mv) {
    std::string moveStr;
    moveStr += char('a' + mv.startPos.second);
    moveStr += char('8' - mv.startPos.first);
    moveStr += char('a' + mv.endPos.second);
    moveStr += char('8' - mv.endPos.first);
    if (mv.isPromotion) {
        switch (mv.promotionType) {
            case QUEEN:  moveStr += 'q'; break;
            case ROOK:   moveStr += 'r'; break;
            case BISHOP: moveStr += 'b'; break;
            case KNIGHT: moveStr += 'n'; break;
            default:     moveStr += 'q'; break;
        }
    }
    return moveStr;
}

static std::uint64_t perft_board(Board& board, Color sideToMove, int depth) {
    return chess::perftOptimized<Board, Move, Color, UndoMove, decltype(board.getPieceAt(0,0))>(
        board, sideToMove, depth, g_profiler, g_enableBulkCount);
}

// Perft function that respects --only filter at root level (for regular perft mode)
static std::uint64_t perft_board_with_filter(Board& board, Color sideToMove, int depth) {
    return chess::perftWithFilter<Board, Move, Color, UndoMove, decltype(board.getPieceAt(0,0))>(
        board, sideToMove, depth, g_profiler, moveToString, onlyMoveGlobal, g_enableBulkCount);
}

// ThreadPool-based perft: each move runs on fresh board copy to avoid data races
static std::uint64_t perft_split_mt(const Board& rootBoard, Color sideToMove, int depth, int maxThreads, SDL_Renderer* renderer) {
    // Disable profiler while worker threads run
    g_profiler.setEnabled(false);
    std::uint64_t result = chess::perftSplitMT<Board, Move, Color, UndoMove, SDL_Renderer>(
        rootBoard, sideToMove, depth, maxThreads, renderer, moveToString, onlyMoveGlobal);
    g_profiler.setEnabled(true);
    return result;
}

static bool g_disableLogging = false;

// Multi-threaded perft without split output (for regular perft mode)
static std::uint64_t perft_mt(const Board& rootBoard, Color sideToMove, int depth, int maxThreads, SDL_Renderer* renderer) {
    // Disable profiler while worker threads run (respects --prof-verbose)
    bool profilerWasEnabled = g_profiler.isEnabled();
    g_profiler.setEnabled(false);
    
    std::uint64_t result = chess::perftMT<Board, Move, Color, UndoMove, SDL_Renderer>(
        rootBoard, sideToMove, depth, maxThreads, renderer, moveToString, 
        onlyMoveGlobal, g_enableBulkCount, g_disableLogging);
    
    // Re-enable profiler to original state
    g_profiler.setEnabled(profilerWasEnabled);
    return result;
}

static std::uint64_t perft_split(Board& board, Color sideToMove, int depth) {
    return chess::perftSplit<Board, Move, Color, UndoMove, decltype(board.getPieceAt(0,0))>(
        board, sideToMove, depth, g_profiler, moveToString, onlyMoveGlobal);
}

int main(int argc, char* argv[]) {
    g_disableLogging = false;
    Logger::init("output/logs", LogLevel::INFO, false, 50);
    
    // Suppress verbose logging during perft for performance (re-enabled by --verbose)
    Logger::setMinLevel(LogLevel::ERROR);
    bool verbose = false;

    // Crash handler: log fatal signals before termination
    static auto crashHandlerFn = [](int sig) {
        std::ostringstream oss;
        oss << "Fatal signal caught: " << sig << "\n";
        try {
            Logger::log(LogLevel::ERROR, oss.str(), __FILE__, __LINE__);
            Logger::shutdown();
        } catch (...) {
        }
        std::abort();
    };
    std::signal(SIGSEGV, [](int s){ crashHandlerFn(s); });
    std::signal(SIGABRT, [](int s){ crashHandlerFn(s); });

    int maxDepth = 4;
    std::string fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
    bool splitMode = false;
    bool headless = false;
    std::string onlyMove = "";

    // Parse mode and depth from positional arguments
    if (argc >= 2) {
        std::string a1 = argv[1];
        if (a1 == "split") {
            splitMode = true;
            if (argc >= 3) {
                maxDepth = std::atoi(argv[2]);
                if (maxDepth < 1) maxDepth = 6;
            }
        } else {
            if (a1.rfind("--", 0) != 0) {
                maxDepth = std::atoi(argv[1]);
                if (maxDepth < 1) maxDepth = 1;
            }
        }
    }

    // Parse optional FEN string (reconstruct multi-word FENs)
    int fenArgIndex = splitMode ? 3 : 2;
    if (argc > fenArgIndex) {
        std::string candidate = argv[fenArgIndex];
        if (candidate.rfind("--", 0) != 0) {
            fen = candidate;
            for (int i = fenArgIndex + 1; i < argc; ++i) {
                std::string next = argv[i];
                if (next.rfind("--", 0) == 0) break;
                fen += " ";
                fen += next;
            }
        }
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless") headless = true;
        else if (arg == "--verbose") verbose = true;
        else if (arg == "--no-bulk") g_enableBulkCount = false;
        else if (arg == "--prof-verbose") g_profiler.setVerbose(true);
        else if (arg.rfind("--only=", 0) == 0) onlyMove = arg.substr(7);
    }

        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        if (!headless) {
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                Logger::log(LogLevel::ERROR, std::string("SDL init failed: ") + SDL_GetError(), __FILE__, __LINE__);
                return 1;
            }
            if (IMG_Init(IMG_INIT_PNG) == 0) {
                Logger::log(LogLevel::ERROR, std::string("SDL_image init failed: ") + IMG_GetError(), __FILE__, __LINE__);
                SDL_Quit();
                return 1;
            }

            window = SDL_CreateWindow("Perft", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 800, SDL_WINDOW_HIDDEN);
            if (!window) {
                Logger::log(LogLevel::ERROR, std::string("Window creation failed: ") + SDL_GetError(), __FILE__, __LINE__);
                IMG_Quit();
                SDL_Quit();
                return 1;
            }
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            if (!renderer) {
                Logger::log(LogLevel::ERROR, std::string("Renderer creation failed: ") + SDL_GetError(), __FILE__, __LINE__);
                SDL_DestroyWindow(window);
                IMG_Quit();
                SDL_Quit();
            return 1;
        }

        TextureCache::setRenderer(renderer);
    }

    onlyMoveGlobal = onlyMove;
    Board board(800, 800, 20.0f);
    board.setStartFEN(fen);
    board.initializeBoard(renderer);

    // Get side to move from Board's FEN parsing
    Color stm = board.getCurrentPlayer();

    PerftState state{ &board, stm };

    if (verbose) {
        Logger::setMinLevel(LogLevel::INFO);
        Logger::log(LogLevel::INFO, std::string("Side to move: ") + (stm == WHITE ? "WHITE" : "BLACK"), __FILE__, __LINE__);
        std::vector<Move> debugMoves = board.getAllLegalMoves(stm, true);
        Logger::log(LogLevel::INFO, "First 5 moves generated:", __FILE__, __LINE__);
        for (int i = 0; i < std::min(5, (int)debugMoves.size()); ++i) {
            const Move& mv = debugMoves[i];
            Logger::log(LogLevel::INFO, std::string("  ") + moveToString(mv) + " (piece at " + std::to_string(mv.startPos.first) + "," + std::to_string(mv.startPos.second) + ")", __FILE__, __LINE__);
        }
        Logger::log(LogLevel::INFO, std::string("Running chess perft from FEN: ") + fen, __FILE__, __LINE__);
    }

    // Parse --threads option for both split and regular mode
    int parsedThreads = 0;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        const std::string threadsPrefix = "--threads=";
        if (arg.rfind(threadsPrefix, 0) == 0) {
            try { parsedThreads = std::stoi(arg.substr(threadsPrefix.size())); } catch(...) { parsedThreads = 0; }
        }
    }

    if (splitMode) {
        auto t0 = std::chrono::high_resolution_clock::now();
        std::uint64_t total = (parsedThreads > 0) 
            ? perft_split_mt(*state.board, state.sideToMove, maxDepth, parsedThreads, renderer)
            : perft_split(*state.board, state.sideToMove, maxDepth);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        std::string modeDesc = (parsedThreads > 0) ? "Split (mt)" : "Split";
        std::cout << modeDesc << " completed in " << ms << " milliseconds\nNodes searched: " << total << std::endl;
    } else {
        for (int d = 1; d <= maxDepth; ++d) {
            auto t0 = std::chrono::high_resolution_clock::now();
            std::uint64_t nodes;
            
            if (parsedThreads > 0) {
                // Multi-threaded perft (respects --no-bulk, --only, --verbose, etc.)
                if (verbose) {
                    Logger::log(LogLevel::INFO, std::string("Perft (mt) at depth ") + std::to_string(d) + " threads=" + std::to_string(parsedThreads), __FILE__, __LINE__);
                    std::cout << "[perft_mt] launching with threads=" << parsedThreads << " depth=" << d << std::endl;
                }
                nodes = perft_mt(*state.board, state.sideToMove, d, parsedThreads, renderer);
            } else {
                // Single-threaded perft (respects all options including --only)
                nodes = perft_board_with_filter(board, state.sideToMove, d);
            }
            
            auto t1 = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            std::string modeDesc = (parsedThreads > 0) ? " (mt)" : "";
            std::cout << "Depth: " << d << " ply" << modeDesc << "  Result: " << nodes << " positions  Time: " << ms << " milliseconds" << std::endl;
        }
    }

    // Cleanup
    if (!headless) {
        TextureCache::clear();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
    }

    Logger::shutdown();
    g_profiler.report();
    return 0;
}
