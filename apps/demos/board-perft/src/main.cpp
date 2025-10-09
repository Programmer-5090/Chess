#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>
#include <csignal>
#include <thread>
#include <future>
#include <atomic>
#include <mutex>

#include "../include/logger.h"
#include "../include/perfProfiler.h"
#include "../include/textureCache.h"

#include "../Chess AI/utils.h"
#include "../include/board.h"
#include "../include/pieces/piece.h"
#include "../include/board/pieceManager.h"

// Simple wrapper to track side-to-move outside of Board
struct PerftState {
    Board* board;
    Color sideToMove;
};

// Toggle for bulk-count fast-path at leaf (depth==1). Can be disabled via --no-bulk
static bool g_enableBulkCount = true;

// Optimized perft for Board: generate pseudo-legal once, make once, check king safety, recurse, unmake.
static std::uint64_t perft_board(Board& board, Color sideToMove, int depth) {
    if (depth == 0) return 1ULL;

    std::uint64_t nodes = 0ULL;
    // getAllLegalMoves actually returns pseudo-legal in this codebase
    g_profiler.startTimer("move_generation");
    g_profiler.startTimer("move_generation_top");
    std::vector<Move> moves = board.getAllLegalMoves(sideToMove, /*generateCastlingMoves=*/true);
    g_profiler.endTimer("move_generation_top");
    g_profiler.endTimer("move_generation");
    // Bulk-counting fast path: at depth == 1, avoid making/unmaking moves and
    // instead count the number of legal moves by checking king safety with a
    // hypothetical move evaluation (Board::isKingInCheck(Color, const Move*)).
    if (depth == 1) {
        if (g_enableBulkCount) {
            g_profiler.startTimer("perft_leaf_bulk_count");
            for (const Move& mv : moves) {
                g_profiler.startTimer("leaf_king_safety_check");
                bool illegal = board.isKingInCheck(sideToMove, &mv);
                g_profiler.endTimer("leaf_king_safety_check");
                if (!illegal) nodes += 1ULL;
            }
            g_profiler.endTimer("perft_leaf_bulk_count");
            return nodes;
        } else {
            // Bulk-count disabled: fallthrough to normal make/unmake path below
        }
    }

    for (const Move& mv : moves) {
        UndoMove u{};
        g_profiler.startTimer("make_move");
        u = board.executeMove(mv, true);
        g_profiler.endTimer("make_move");

        // If our king is in check after making the move, it's illegal
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
                // Handle the error appropriately
            }
        #endif
    }
    return nodes;
}

// Multithreaded perft split: parallelize top-level moves. Each worker creates
// a fresh Board initialized from the root FEN to avoid shared-state mutations.
static std::uint64_t perft_split_mt(const Board& rootBoard, Color sideToMove, int depth, int maxThreads, SDL_Renderer* renderer) {
    Logger::log(LogLevel::INFO, std::string("Perft split (mt) at depth ") + std::to_string(depth) + " threads=" + std::to_string(maxThreads), __FILE__, __LINE__);
    // Also print a concise message to stdout so CI/terminal runs can see that
    // the multithreaded path was chosen and how many threads will be used.
    std::cout << "[perft_split_mt] launching with threads=" << maxThreads << " depth=" << depth << std::endl;
    std::vector<Move> moves = rootBoard.getAllLegalMoves(sideToMove, /*generateCastlingMoves=*/true);
    if (moves.empty()) return 0ULL;

    int hw = std::max(1, (int)std::thread::hardware_concurrency());
    int threads = (maxThreads > 0) ? std::min(maxThreads, (int)moves.size()) : std::min(hw, (int)moves.size());
    std::atomic<std::uint64_t> totalNodes{0};
    std::atomic<size_t> idx{0};

    auto worker = [&](int threadIndex){
        // Announce worker startup once
        static std::atomic<int> startedWorkers{0};
        static std::mutex cout_mutex;
        int myStartIndex = startedWorkers.fetch_add(1, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lk(cout_mutex);
            std::cout << "[perft_split_mt] worker " << threadIndex << " started (index=" << myStartIndex << ")" << std::endl;
            std::cout.flush();
        }

        try {
            for (;;) {
                size_t i = idx.fetch_add(1, std::memory_order_relaxed);
                if (i >= moves.size()) break;
                const Move mv = moves[i]; // copy

            // Create a fresh board for this top-level move
            Board freshBoard(800, 800, 20.0f);
            freshBoard.setStartFEN(rootBoard.getStartFEN());
            freshBoard.initializeBoard(renderer);

            // Find and apply equivalent move on the fresh board
            std::vector<Move> freshMoves = freshBoard.getAllLegalMoves(sideToMove, true);
            bool applied = false;
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
                    if (!illegal) totalNodes.fetch_add(moveNodes, std::memory_order_relaxed);
                    applied = true;
                    break;
                }
            }
                if (!applied) {
                    Logger::log(LogLevel::WARN, "perft_split_mt: failed to apply top move on fresh board", __FILE__, __LINE__);
                }
            }
        } catch (const std::exception &ex) {
            std::lock_guard<std::mutex> lk(cout_mutex);
            std::cout << "[perft_split_mt] worker " << threadIndex << " caught exception: " << ex.what() << std::endl;
            std::cout.flush();
            Logger::log(LogLevel::ERROR, std::string("perft_split_mt: worker exception: ") + ex.what(), __FILE__, __LINE__);
        } catch (...) {
            std::lock_guard<std::mutex> lk(cout_mutex);
            std::cout << "[perft_split_mt] worker " << threadIndex << " caught unknown exception" << std::endl;
            std::cout.flush();
            Logger::log(LogLevel::ERROR, "perft_split_mt: worker unknown exception", __FILE__, __LINE__);
        }

        {
            std::lock_guard<std::mutex> lk(cout_mutex);
            std::cout << "[perft_split_mt] worker " << threadIndex << " finished" << std::endl;
            std::cout.flush();
        }
    };

    std::vector<std::future<void>> futs;
    futs.reserve(threads);
    // Disable profiler while worker threads run to avoid concurrent access to
    // the non-threadsafe profiler data structures. The profiler will still be
    // available in single-threaded runs.
    g_profiler.setEnabled(false);
    for (int t = 0; t < threads; ++t) futs.emplace_back(std::async(std::launch::async, worker, t));
    for (auto &f : futs) f.get();
    g_profiler.setEnabled(true);

    return totalNodes.load(std::memory_order_relaxed);
}

// Global filter for running only a specific top-level move (e.g., "c4b5")
static std::string onlyMoveGlobal = "";
static bool g_disableLogging = false;

// Perft split: show per-move node counts for debugging
// Return the total node count from the split run so caller can reuse it
static std::uint64_t perft_split(Board& board, Color sideToMove, int depth) {
    Logger::log(LogLevel::INFO, std::string("Perft split at depth ") + std::to_string(depth), __FILE__, __LINE__);
    std::uint64_t totalNodes = 0ULL;

    std::vector<Move> moves = board.getAllLegalMoves(sideToMove, /*generateCastlingMoves=*/true);

    for (const Move& mv : moves) {
        // If onlyMoveGlobal is set, skip moves that don't match
        if (!onlyMoveGlobal.empty()) {
            std::string moveStr = "";
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
            if (moveStr != onlyMoveGlobal) continue;
        }
        UndoMove u{};
    g_profiler.startTimer("make_move_top");
    u = board.executeMove(mv);
    g_profiler.endTimer("make_move_top");
    // Ensure caches reflect the applied move before checking king safety
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
            // Convert move to algebraic notation
            // Fix coordinate system: row 0 = rank 8, row 7 = rank 1
            std::string moveStr = "";
            moveStr += char('a' + mv.startPos.second);
            moveStr += char('8' - mv.startPos.first);
            moveStr += char('a' + mv.endPos.second);
            moveStr += char('8' - mv.endPos.first);

            // Append promotion suffix if applicable (e.g., d7c8q)
            if (mv.isPromotion) {
                switch (mv.promotionType) {
                    case QUEEN:  moveStr += 'q'; break;
                    case ROOK:   moveStr += 'r'; break;
                    case BISHOP: moveStr += 'b'; break;
                    case KNIGHT: moveStr += 'n'; break;
                    default:     moveStr += 'q'; break;
                }
            }

                // Print per-move node count only to stdout so split runs are visible in terminal.
                std::cout << moveStr << ": " << moveNodes << std::endl;
        }
    }

    // Remove debug output for clean profiling
    // Profile breakdown — print top entries from the profiler to console only
    auto detailed = g_profiler.getDetailedItems();
    long long total_inclusive_us = 0;
    for (const auto &it : detailed) total_inclusive_us += it.inclusive_us;
    std::cout << "\nProfiling breakdown:\n";

    // External (root) totals: timers started at stack depth 0
    auto roots = g_profiler.getRootItems();
    long long total_root_us = 0;
    for (const auto &r : roots) total_root_us += r.second;
    std::cout << "\nExternal (root) totals:" << std::endl;
    int root_show = std::min<int>(10, (int)roots.size());
    for (int i = 0; i < root_show; ++i) {
        double ms = roots[i].second / 1000.0;
        double pct = (total_root_us > 0) ? (100.0 * roots[i].second / total_root_us) : 0.0;
        std::cout << i+1 << ") " << roots[i].first << ": " << ms << " ms (" << pct << "% of external time)" << std::endl;
    }
    if (roots.size() > (size_t)root_show) std::cout << "...and " << (roots.size() - root_show) << " more root entries" << std::endl;

    // Internal: inclusive/exclusive breakdown of all timers (per-call averages shown too)
    std::cout << "\nInternal (inclusive/exclusive):" << std::endl;
    int to_show = std::min<int>(10, (int)detailed.size());
    for (int i = 0; i < to_show; ++i) {
        const auto &p = detailed[i];
        double incl_ms = p.inclusive_us / 1000.0;
        double excl_ms = p.exclusive_us / 1000.0;
        double avg_incl_ms = p.calls ? (p.inclusive_us / 1000.0 / p.calls) : 0.0;
        double pct = (total_inclusive_us > 0) ? (100.0 * p.inclusive_us / total_inclusive_us) : 0.0;
        std::cout << i+1 << ") " << p.name << ": incl=" << incl_ms << " ms, excl=" << excl_ms << " ms, calls=" << p.calls << ", avg(incl)=" << avg_incl_ms << " ms (" << pct << "% of measured time)" << std::endl;

        // Show top internal children with per-call averages
        auto children = g_profiler.getChildItemsDetailed(p.name);
        int child_show = std::min<int>(3, (int)children.size());
        for (int c = 0; c < child_show; ++c) {
            double child_ms = children[c].inclusive_us / 1000.0;
            double child_avg = children[c].calls ? (children[c].inclusive_us / 1000.0 / children[c].calls) : 0.0;
            std::cout << "    - " << children[c].name << ": " << child_ms << " ms (calls=" << children[c].calls << ", avg=" << child_avg << " ms)" << std::endl;
        }
    }
    if (detailed.size() > (size_t)to_show) std::cout << "...and " << (detailed.size() - to_show) << " more entries" << std::endl;

    // Print total nodes accumulated by this split run to both log and stdout
    Logger::log(LogLevel::INFO, std::string("\nNodes searched: ") + std::to_string(totalNodes), __FILE__, __LINE__);
    std::cout << "\nNodes searched: " << totalNodes << std::endl;
    return totalNodes;
}

// Safe split: recreate a fresh Board for each top-level move (avoids relying
// on incremental caches staying perfectly consistent across deep recursion).
static std::uint64_t perft_split_fresh(const Board& rootBoard, Color sideToMove, int depth, SDL_Renderer* renderer) {
    Logger::log(LogLevel::INFO, std::string("Perft split (fresh boards) at depth ") + std::to_string(depth), __FILE__, __LINE__);
    std::uint64_t totalNodes = 0ULL;

    // Get top-level moves from a const reference to the root board
    std::vector<Move> moves = rootBoard.getAllLegalMoves(sideToMove, /*generateCastlingMoves=*/true);

    for (const Move& mv : moves) {
        // If onlyMoveGlobal is set, skip moves that don't match the filter
        if (!onlyMoveGlobal.empty()) {
            std::string moveStr = "";
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
            if (moveStr != onlyMoveGlobal) continue;
        }

        // For each top-level move, create a fresh board initialized from the same FEN
        Board freshBoard(800, 800, 20.0f);
        freshBoard.setStartFEN(rootBoard.getStartFEN());
        freshBoard.initializeBoard(renderer);

        // Find and apply the same move on the fresh board. We assume Move equality
        // by start/end positions and promotion flags.
        std::vector<Move> freshMoves = freshBoard.getAllLegalMoves(sideToMove, true);
        bool applied = false;
        for (const Move& fm : freshMoves) {
            if (fm.startPos == mv.startPos && fm.endPos == mv.endPos && fm.isPromotion == mv.isPromotion && fm.promotionType == mv.promotionType) {
                UndoMove u{};
                u = freshBoard.executeMove(fm);
                // If king is in check after the move, skip
                bool illegal = freshBoard.isKingInCheck(sideToMove);
                std::uint64_t moveNodes = 0ULL;
                if (!illegal) {
                    Color next = (sideToMove == WHITE ? BLACK : WHITE);
                    moveNodes = perft_board(freshBoard, next, depth - 1);
                    totalNodes += moveNodes;
                }
                freshBoard.undoMove(fm, u);

                if (!illegal) {
                    std::string moveStr = "";
                    moveStr += char('a' + fm.startPos.second);
                    moveStr += char('8' - fm.startPos.first);
                    moveStr += char('a' + fm.endPos.second);
                    moveStr += char('8' - fm.endPos.first);
                    if (fm.isPromotion) {
                        switch (fm.promotionType) {
                            case QUEEN:  moveStr += 'q'; break;
                            case ROOK:   moveStr += 'r'; break;
                            case BISHOP: moveStr += 'b'; break;
                            case KNIGHT: moveStr += 'n'; break;
                            default:     moveStr += 'q'; break;
                        }
                    }
                    // For fresh-split, also avoid logging per-move details; print to stdout only.
                    std::cout << moveStr << ": " << moveNodes << std::endl;
                }

                applied = true;
                break;
            }
        }
        if (!applied) {
            // If we couldn't find the matching move on the fresh board, warn
        std::ostringstream warnoss;
        warnoss << "Warning: top-level move not found on fresh board: "
            << mv.startPos.first << "," << mv.startPos.second << " -> "
            << mv.endPos.first << "," << mv.endPos.second;
        Logger::log(LogLevel::WARN, warnoss.str(), __FILE__, __LINE__);
        }
    }

    Logger::log(LogLevel::INFO, std::string("\nNodes searched (fresh split): ") + std::to_string(totalNodes), __FILE__, __LINE__);
    std::cout << "\nNodes searched (fresh split): " << totalNodes << std::endl;
    return totalNodes;
}

int main(int argc, char* argv[]) {
    using namespace chessai;

    // SDL needed for piece textures in loadFEN()
    // Initialize logger first (write logs to output/logs).
    // Always enable file logging for perft runs so detailed traces are captured
    // in `output/logs` while still printing summary totals to stdout.
    g_disableLogging = false;
    Logger::init("output/logs", LogLevel::INFO, /*redirectStreams=*/false, 50);
    // Quick mitigation for profiling: raise minimum log level to ERROR to
    // avoid spending time on INFO/DEBUG logging during perft runs. This is
    // intentionally a temporary runtime tweak to measure the impact of
    // logging on make/unmake performance. A `--verbose` flag can re-enable
    // these additional logs when needed.
    Logger::setMinLevel(LogLevel::ERROR);
    bool verbose = false;

    // Install basic signal handlers so crashes like access violations get
    // recorded in the log file. This won't replace a debugger, but it
    // helps capture why the process suddenly terminates.
    static auto crashHandlerFn = [](int sig) {
        std::ostringstream oss;
        oss << "Fatal signal caught: " << sig << "\n";
        try {
            Logger::log(LogLevel::ERROR, oss.str(), __FILE__, __LINE__);
            Logger::shutdown();
        } catch (...) {
            // Fallback: nothing we can do if logging fails
        }
        std::abort();
    };
    std::signal(SIGSEGV, [](int s){ crashHandlerFn(s); });
    std::signal(SIGABRT, [](int s){ crashHandlerFn(s); });

        // Parse optional args: depth and FEN
    int maxDepth = 4;
        std::string fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
    bool freshSplitMode = false;
    bool splitMode = false;
        bool headless = false;
        
        std::string onlyMove = ""; // if non-empty, only run this top-level move (e.g., c4b5)

        // First pass: detect modes based on positional args (split/splitsafe or depth)
        if (argc >= 2) {
            std::string a1 = argv[1];
            if (a1 == "split") {
                splitMode = true;
                if (argc >= 3) {
                    maxDepth = std::atoi(argv[2]);
                    if (maxDepth < 1) maxDepth = 6;
                }
            } else if (a1 == "splitsafe") {
                freshSplitMode = true;
                if (argc >= 3) {
                    maxDepth = std::atoi(argv[2]);
                    if (maxDepth < 1) maxDepth = 6;
                }
            } else {
                // If first arg isn't a flag, treat it as depth
                if (a1.rfind("--", 0) != 0) {
                    maxDepth = std::atoi(argv[1]);
                    if (maxDepth < 1) maxDepth = 1;
                }
            }
        }

        // Determine where a FEN argument would be (positional after mode/depth)
        int fenArgIndex = (splitMode || freshSplitMode) ? 3 : 2;
        if (argc > fenArgIndex) {
            std::string candidate = argv[fenArgIndex];
            // If candidate looks like a flag (starts with --), don't treat it as FEN
            if (candidate.rfind("--", 0) != 0) {
                fen = candidate;
                // Reconstruct a possibly space-containing FEN until a flag or end
                for (int i = fenArgIndex + 1; i < argc; ++i) {
                    std::string next = argv[i];
                    if (next.rfind("--", 0) == 0) break;
                    fen += " ";
                    fen += next;
                }
            }
        }

        // Support flags: --only=move, --prof-verbose, --headless, --threads=N
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            const std::string prefix = "--only=";
            const std::string threadsPrefix = "--threads=";
            if (arg == "--headless") {
                headless = true;
                continue;
            }
            if (arg == "--verbose") {
                verbose = true;
                continue;
            }
            if (arg == "--no-bulk") {
                g_enableBulkCount = false;
                continue;
            }
            if (arg.rfind(prefix, 0) == 0) {
                onlyMove = arg.substr(prefix.size());
                continue;
            }
            if (arg == "--prof-verbose") {
                g_profiler.setVerbose(true);
                continue;
            }
            if (arg.rfind(threadsPrefix, 0) == 0) {
                // parse in-place by storing in a local var; we'll read later
                try {
                    int t = std::stoi(arg.substr(threadsPrefix.size()));
                    (void)t; // we'll pass this to perft_split_mt via a captured var
                } catch (...) {
                    // ignore parse errors; we'll use hardware_concurrency by default
                }
                continue;
            }
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

        // Initialize texture cache with the renderer so piece ctors can reuse textures
        TextureCache::setRenderer(renderer);
    }

    // Propagate to global filter used by perft functions
    onlyMoveGlobal = onlyMove;    Board board(800, 800, 20.0f);
    board.setStartFEN(fen);
    board.initializeBoard(renderer);
    // `initializeBoard` calls `loadFEN`, which already builds the caches via `cachePieces()`.

    // Determine side to move from FEN (field 2)
    Color stm = WHITE;
    {
        // Very small parser: split by spaces, check second token
        size_t p1 = fen.find(' ');
        if (p1 != std::string::npos) {
            size_t p2 = fen.find(' ', p1 + 1);
            std::string active = fen.substr(p1 + 1, (p2 == std::string::npos ? std::string::npos : p2 - (p1 + 1)));
            if (active == "b" || active == "B") stm = BLACK; else stm = WHITE;
        }
    }

    PerftState state{ &board, stm };

    // Optionally print debug details when verbose mode is enabled
    if (verbose) {
        Logger::log(LogLevel::INFO, std::string("Side to move: ") + (stm == WHITE ? "WHITE" : "BLACK"), __FILE__, __LINE__);
        std::vector<Move> debugMoves = board.getAllLegalMoves(stm, true);
        Logger::log(LogLevel::INFO, "First 5 moves generated:", __FILE__, __LINE__);
        for (int i = 0; i < std::min(5, (int)debugMoves.size()); ++i) {
            const Move& mv = debugMoves[i];
            std::string moveStr = "";
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
            Logger::log(LogLevel::INFO, std::string("  ") + moveStr + " (piece at " + std::to_string(mv.startPos.first) + "," + std::to_string(mv.startPos.second) + ")", __FILE__, __LINE__);
        }
        Logger::log(LogLevel::INFO, "\n", __FILE__, __LINE__);
        Logger::log(LogLevel::INFO, std::string("Running chess perft from FEN: ") + fen, __FILE__, __LINE__);
    }

    if (freshSplitMode) {
        auto t0 = std::chrono::high_resolution_clock::now();
        std::uint64_t total = perft_split_fresh(board, state.sideToMove, maxDepth, renderer);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        Logger::log(LogLevel::INFO, std::string("Split (fresh) completed in ") + std::to_string(ms) + " milliseconds", __FILE__, __LINE__);
        Logger::log(LogLevel::INFO, std::string("Nodes searched: ") + std::to_string(total), __FILE__, __LINE__);
        std::cout << "Split (fresh) completed in " << ms << " milliseconds\n";
        std::cout << "Nodes searched: " << total << std::endl;
    } else if (splitMode) {
        // Show per-move breakdown for debugging
        auto t0 = std::chrono::high_resolution_clock::now();
        // If onlyMove is set, perft_split will need to be run on that specific move.
        // We'll pass it via a thread-local/global-like mechanism by setting a global
        // variable. For simplicity, reuse the existing perft_split implementation
        // by filtering moves inside that function (we'll check `onlyMove` there).
        // To avoid changing many signatures, we use a lambda wrapper.
        // Check for --threads=N flag manually from argv to control concurrency
        int parsedThreads = 0;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            const std::string threadsPrefix = "--threads=";
            if (arg.rfind(threadsPrefix, 0) == 0) {
                try { parsedThreads = std::stoi(arg.substr(threadsPrefix.size())); } catch(...) { parsedThreads = 0; }
            }
        }

        // If parsedThreads > 0 use multithreaded split, else use single-threaded split
        std::uint64_t total = 0ULL;
        if (parsedThreads > 0) {
            auto t0 = std::chrono::high_resolution_clock::now();
            total = perft_split_mt(*state.board, state.sideToMove, maxDepth, parsedThreads, renderer);
            auto t1 = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            Logger::log(LogLevel::INFO, std::string("Split (mt) completed in ") + std::to_string(ms) + " milliseconds", __FILE__, __LINE__);
            std::cout << "Split (mt) completed in " << ms << " milliseconds\n";
        } else {
            auto perft_wrapper = [&](Board& b, Color stm, int d) {
                // Forward to perft_split but use the external onlyMove via capture
                return perft_split(b, stm, d);
            };
            total = perft_wrapper(*state.board, state.sideToMove, maxDepth);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        Logger::log(LogLevel::INFO, std::string("Split completed in ") + std::to_string(ms) + " milliseconds", __FILE__, __LINE__);
        Logger::log(LogLevel::INFO, std::string("Nodes searched: ") + std::to_string(total), __FILE__, __LINE__);
        std::cout << "Split completed in " << ms << " milliseconds\n";
        std::cout << "Nodes searched: " << total << std::endl;
    } else {
        // Print header
    Logger::log(LogLevel::INFO, "Running Test... (bulk-counting enabled)", __FILE__, __LINE__);
        for (int d = 1; d <= maxDepth; ++d) {
            // Recreate a fresh board for each depth to avoid any residual state
            // from previous runs (helps detect make/unmake bugs and ensures
            // perft_board is invoked on a clean position each time).
            Board freshBoard(800, 800, 20.0f);
            freshBoard.setStartFEN(board.getStartFEN());
            freshBoard.initializeBoard(renderer);
            auto t0 = std::chrono::high_resolution_clock::now();
            std::uint64_t nodes = perft_board(freshBoard, state.sideToMove, d);
            auto t1 = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

            Logger::log(LogLevel::INFO, std::string("Depth: ") + std::to_string(d) + " ply  Result: " + chessai::formatWithCommas(nodes) + " positions  Time: " + std::to_string(ms) + " milliseconds", __FILE__, __LINE__);
            std::cout << "Depth: " << d << " ply  Result: " << nodes << " positions  Time: " << ms << " milliseconds" << std::endl;
        }
    }

    // Free cached textures before destroying the renderer / quitting SDL.
    // TextureCache::clear() will call SDL_DestroyTexture on all cached textures
    // and must be invoked while the renderer is still valid.
    TextureCache::clear();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    // Shutdown logger
    Logger::shutdown();
    // Print profiler report (written to the project's log via Logger)
    g_profiler.report();
    std::cout<<"size of board piece map "<<board.getPieceManager()->getAllPieceMap().size()<<std::endl;
    return 0;
}
