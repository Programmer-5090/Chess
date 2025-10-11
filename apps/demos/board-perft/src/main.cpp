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
    if (depth == 0) return 1ULL;

    std::uint64_t nodes = 0ULL;
    g_profiler.startTimer("move_generation");
    g_profiler.startTimer("move_generation_top");
    std::vector<Move> moves = board.getAllPseudoLegalMoves(sideToMove, true);
    g_profiler.endTimer("move_generation_top");
    g_profiler.endTimer("move_generation");
    
    // Bulk-count optimization: at depth 1, test move legality without expensive make/unmake
    if (depth == 1) {
        if (g_enableBulkCount) {
            g_profiler.startTimer("perft_leaf_bulk_count");
            for (const Move& mv : moves) {
                Piece* movingPiece = board.getPieceAt(mv.startPos.first, mv.startPos.second);
                if (!movingPiece) continue; // invalid move
                g_profiler.startTimer("leaf_king_safety_check");
                bool illegal = board.isKingInCheck(sideToMove, &mv);
                g_profiler.endTimer("leaf_king_safety_check");
                if (!illegal) nodes += 1ULL;
            }
            g_profiler.endTimer("perft_leaf_bulk_count");
            return nodes;
        }
    }

    for (const Move& mv : moves) {
        UndoMove u{};
        g_profiler.startTimer("make_move");
        u = board.executeMove(mv, true);
        g_profiler.endTimer("make_move");

        g_profiler.startTimer("king_safety");
        bool illegal = board.isKingInCheck(sideToMove);
        g_profiler.endTimer("king_safety");
        if (!illegal) {
            Color next = (sideToMove == WHITE ? BLACK : WHITE);
            nodes += perft_board(board, next, depth - 1);
        }

        g_profiler.startTimer("unmake_move");
        board.undoMove(mv, u);
        g_profiler.endTimer("unmake_move");
        #ifdef DEBUG
            if (!(board.getPieceManager()->validateKings())) {
                Logger::log(LogLevel::ERROR, "King validation failed after unmake!", __FILE__, __LINE__);
            }
        #endif
    }
    return nodes;
}

// Perft function that respects --only filter at root level (for regular perft mode)
static std::uint64_t perft_board_with_filter(Board& board, Color sideToMove, int depth) {
    if (!onlyMoveGlobal.empty()) {
        // Apply --only filter at root level only
        std::vector<Move> moves = board.getAllPseudoLegalMoves(sideToMove, true);
        std::uint64_t nodes = 0ULL;
        
        for (const Move& mv : moves) {
            if (moveToString(mv) != onlyMoveGlobal) continue;
            
            UndoMove u{};
            u = board.executeMove(mv, true);
            bool illegal = board.isKingInCheck(sideToMove);
            if (!illegal) {
                Color next = (sideToMove == WHITE ? BLACK : WHITE);
                nodes += perft_board(board, next, depth - 1);
            }
            board.undoMove(mv, u);
        }
        return nodes;
    } else {
        // No filter, use normal perft
        return perft_board(board, sideToMove, depth);
    }
}

// ThreadPool-based perft: each move runs on fresh board copy to avoid data races
static std::uint64_t perft_split_mt(const Board& rootBoard, Color sideToMove, int depth, int maxThreads, SDL_Renderer* renderer) {
    Logger::log(LogLevel::INFO, std::string("Perft split (mt) at depth ") + std::to_string(depth) + " threads=" + std::to_string(maxThreads), __FILE__, __LINE__);
    std::cout << "[perft_split_mt] launching with threads=" << maxThreads << " depth=" << depth << std::endl;

    std::vector<Move> moves;
    rootBoard.getAllPseudoLegalMoves(sideToMove, moves, /*generateCastlingMoves=*/true);
    if (moves.empty()) return 0ULL;

    // Filter moves and count planned tasks
    int plannedTasks = 0;
    for (const auto& m : moves) {
        if (onlyMoveGlobal.empty() || moveToString(m) == onlyMoveGlobal) ++plannedTasks;
    }
    if (plannedTasks == 0) return 0ULL;
    
    // Use requested threads or limit to available tasks
    int threads = (maxThreads > 0) ? std::min(maxThreads, plannedTasks) : plannedTasks;

    // Disable profiler while worker threads run
    g_profiler.setEnabled(false);
    ThreadPool pool(threads);
    std::mutex coutMutex;
    std::vector<std::future<std::uint64_t>> futures;
    futures.reserve(plannedTasks);

    auto submitTask = [&](const Move& mv) {
        futures.emplace_back(pool.enqueue([&, mv]() -> std::uint64_t {
            // Each worker thread keeps a single Board instance to avoid constructing per task
            thread_local std::unique_ptr<Board> threadBoard;
            if (!threadBoard) {
                threadBoard = std::make_unique<Board>(800, 800, 20.0f);
                threadBoard->setStartFEN(rootBoard.getStartFEN());
                threadBoard->initializeBoard(renderer);
            }
            Board& freshBoard = *threadBoard;

            // Match move on fresh board by positions and promotion type
            std::vector<Move> freshMoves;
            freshBoard.getAllPseudoLegalMoves(sideToMove, freshMoves, true);
            for (const Move& fm : freshMoves) {
                if (fm.startPos == mv.startPos && fm.endPos == mv.endPos && fm.isPromotion == mv.isPromotion && fm.promotionType == mv.promotionType) {
                    UndoMove u{};
                    u = freshBoard.executeMove(fm);
                    bool illegal = freshBoard.isKingInCheck(sideToMove);
                    std::uint64_t moveNodes = 0ULL;
                    if (!illegal) {
                        Color next = (sideToMove == WHITE ? BLACK : WHITE);
                        moveNodes = perft_board(freshBoard, next, depth - 1);
                    }
                    freshBoard.undoMove(fm, u);

                    if (!illegal) {
                        std::lock_guard<std::mutex> lk(coutMutex);
                        std::cout << moveToString(fm) << ": " << moveNodes << std::endl;
                    }
                    return moveNodes;
                }
            }
            Logger::log(LogLevel::WARN, "perft_split_mt: failed to apply top move on fresh board", __FILE__, __LINE__);
            return 0ULL;
        }));
    };

    for (const Move& mv : moves) {
        if (!onlyMoveGlobal.empty() && moveToString(mv) != onlyMoveGlobal) continue;
        submitTask(mv);
    }

    std::uint64_t totalNodes = 0ULL;
    for (auto& fut : futures) {
        totalNodes += fut.get();
    }
    g_profiler.setEnabled(true);
    return totalNodes;
}

static bool g_disableLogging = false;

// Multi-threaded perft without split output (for regular perft mode)
static std::uint64_t perft_mt(const Board& rootBoard, Color sideToMove, int depth, int maxThreads, SDL_Renderer* renderer) {
    if (depth <= 1) {
        // For shallow depths, single-threaded is more efficient
        Board tempBoard(800, 800, 20.0f);
        tempBoard.setStartFEN(rootBoard.getStartFEN());
        tempBoard.initializeBoard(renderer);
        return perft_board_with_filter(tempBoard, sideToMove, depth);
    }

    std::vector<Move> moves;
    rootBoard.getAllPseudoLegalMoves(sideToMove, moves, /*generateCastlingMoves=*/true);
    if (moves.empty()) return 0ULL;

    // Filter moves if --only option is used
    std::vector<Move> filteredMoves;
    for (const Move& mv : moves) {
        if (onlyMoveGlobal.empty() || moveToString(mv) == onlyMoveGlobal) {
            filteredMoves.push_back(mv);
        }
    }
    if (filteredMoves.empty()) return 0ULL;
    
    // Use requested threads or limit to available tasks
    int threads = (maxThreads > 0) ? std::min(maxThreads, (int)filteredMoves.size()) : (int)filteredMoves.size();

    // Disable profiler while worker threads run (respects --prof-verbose)
    bool profilerWasEnabled = g_profiler.isEnabled();
    g_profiler.setEnabled(false);
    
    ThreadPool pool(threads);
    std::vector<std::future<std::uint64_t>> futures;
    futures.reserve(filteredMoves.size());

    for (const Move& mv : filteredMoves) {
        futures.emplace_back(pool.enqueue([&, mv]() -> std::uint64_t {
            // Each thread gets isolated Board instance to prevent data races
            Board freshBoard(800, 800, 20.0f);
            freshBoard.setStartFEN(rootBoard.getStartFEN());
            freshBoard.initializeBoard(renderer);

            // Match move on fresh board by positions and promotion type
            std::vector<Move> freshMoves = freshBoard.getAllPseudoLegalMoves(sideToMove, true);
            for (const Move& fm : freshMoves) {
                if (fm.startPos == mv.startPos && fm.endPos == mv.endPos && 
                    fm.isPromotion == mv.isPromotion && fm.promotionType == mv.promotionType) {
                    UndoMove u{};
                    u = freshBoard.executeMove(fm);
                    bool illegal = freshBoard.isKingInCheck(sideToMove);
                    if (!illegal) {
                        Color next = (sideToMove == WHITE ? BLACK : WHITE);
                        // Use perft_board which respects g_enableBulkCount (--no-bulk option)
                        std::uint64_t moveNodes = perft_board(freshBoard, next, depth - 1);
                        freshBoard.undoMove(fm, u);
                        return moveNodes;
                    }
                    freshBoard.undoMove(fm, u);
                    return 0ULL;
                }
            }
            // Log warning if move matching fails (but only in verbose mode)
            if (!g_disableLogging) {
                Logger::log(LogLevel::WARN, "perft_mt: failed to apply top move on fresh board", __FILE__, __LINE__);
            }
            return 0ULL;
        }));
    }

    std::uint64_t totalNodes = 0ULL;
    for (auto& fut : futures) {
        totalNodes += fut.get();
    }
    
    // Re-enable profiler to original state
    g_profiler.setEnabled(profilerWasEnabled);
    return totalNodes;
}

static std::uint64_t perft_split(Board& board, Color sideToMove, int depth) {
    Logger::log(LogLevel::INFO, std::string("Perft split at depth ") + std::to_string(depth), __FILE__, __LINE__);
    std::uint64_t totalNodes = 0ULL;

    std::vector<Move> moves = board.getAllPseudoLegalMoves(sideToMove, true);

    for (const Move& mv : moves) {
        if (!onlyMoveGlobal.empty() && moveToString(mv) != onlyMoveGlobal) continue;
        UndoMove u{};
    g_profiler.startTimer("make_move_top");
    u = board.executeMove(mv);
    g_profiler.endTimer("make_move_top");
    bool illegal = board.isKingInCheck(sideToMove);
        std::uint64_t moveNodes = 0ULL;
        if (!illegal) {
            Color next = (sideToMove == WHITE ? BLACK : WHITE);
            moveNodes = perft_board(board, next, depth - 1);
            totalNodes += moveNodes;
        }

    g_profiler.startTimer("unmake_move_top");
    board.undoMove(mv, u);
    g_profiler.endTimer("unmake_move_top");

        if (!illegal) {
            std::cout << moveToString(mv) << ": " << moveNodes << std::endl;
        }
    }

    Logger::log(LogLevel::INFO, std::string("\nNodes searched: ") + std::to_string(totalNodes), __FILE__, __LINE__);
    std::cout << "\nNodes searched: " << totalNodes << std::endl;
    return totalNodes;
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
