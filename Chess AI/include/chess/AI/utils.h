#ifndef CHESS_AI_UTILS_H
#define CHESS_AI_UTILS_H

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <future>
#include <mutex>
#include <memory>
#include <chess/utils/logger.h>
#include <chess/utils/thread_pool.h>
#include <chess/enums.h>

namespace chess {

// Formats an integer with thousands separators (e.g., 11,906,0324)
inline std::string formatWithCommas(std::uint64_t value) {
    // Try the user's global locale first; some Windows environments may not have
    // a default C++ locale installed. Fall back gracefully.
    try {
        std::ostringstream oss;
        oss.imbue(std::locale("")); // user-preferred locale
        oss << std::fixed << value;
        return oss.str();
    } catch (...) {
        try {
            std::ostringstream oss;
            oss.imbue(std::locale::classic());
            oss << std::fixed << value;
            return oss.str();
        } catch (...) {
            // Final fallback: manual grouping with commas
            std::string s = std::to_string(value);
            std::string out;
            out.reserve(s.size() + s.size() / 3);
            int count = 0;
            for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
                out.push_back(s[i]);
                if (++count == 3 && i != 0) {
                    out.push_back(',');
                    count = 0;
                }
            }
            std::reverse(out.begin(), out.end());
            return out;
        }
    }
}

// Performance-optimized perft with profiling and bulk counting support
// - BoardT: your board type (passed by reference)  
// - MoveT: your move type
// - ColorT: your color/side enum type
// - UndoT: undo move data type
// - PieceT: piece type 
// - ProfilerT: profiler type with startTimer/endTimer methods
template <typename BoardT, typename MoveT, typename ColorT, typename UndoT, typename PieceT, typename ProfilerT>
std::uint64_t perftOptimized(BoardT& board, 
                            ColorT sideToMove, 
                            int depth,
                            ProfilerT& profiler,
                            bool enableBulkCount = true) {
    if (depth == 0) return 1ULL;

    std::uint64_t nodes = 0ULL;
    profiler.startTimer("move_generation");
    profiler.startTimer("move_generation_top");
    std::vector<MoveT> moves = board.getAllPseudoLegalMoves(sideToMove, true);
    profiler.endTimer("move_generation_top");
    profiler.endTimer("move_generation");
    
    // Bulk-count optimization: at depth 1, test move legality without expensive make/unmake
    if (depth == 1) {
        if (enableBulkCount) {
            profiler.startTimer("perft_leaf_bulk_count");
            for (const MoveT& mv : moves) {
                auto* movingPiece = board.getPieceAt(mv.startPos.first, mv.startPos.second);
                if (!movingPiece) continue; // invalid move
                profiler.startTimer("leaf_king_safety_check");
                bool illegal = board.isKingInCheck(sideToMove, &mv);
                profiler.endTimer("leaf_king_safety_check");
                if (!illegal) nodes += 1ULL;
            }
            profiler.endTimer("perft_leaf_bulk_count");
            return nodes;
        }
    }

    for (const MoveT& mv : moves) {
        UndoT u{};
        profiler.startTimer("make_move");
        u = board.executeMove(mv, true);
        profiler.endTimer("make_move");

        profiler.startTimer("king_safety");
        bool illegal = board.isKingInCheck(sideToMove);
        profiler.endTimer("king_safety");
        if (!illegal) {
            // Flip the color for next player
            ColorT next = (sideToMove == WHITE ? BLACK : WHITE);
            nodes += perftOptimized<BoardT, MoveT, ColorT, UndoT, PieceT, ProfilerT>(board, next, depth - 1, profiler, enableBulkCount);
        }

        profiler.startTimer("unmake_move");
        board.undoMove(mv, u);
        profiler.endTimer("unmake_move");
        
        #ifdef DEBUG
        // Optional validation in debug mode
        if constexpr (requires { board.getPieceManager()->validateKings(); }) {
            if (!(board.getPieceManager()->validateKings())) {
                Logger::log(LogLevel::ERROR, "King validation failed after unmake!", __FILE__, __LINE__);
            }
        }
        #endif
    }
    return nodes;
}

// Generic perft (basic version without optimizations for compatibility)
template <typename BoardT, typename MoveT, typename GenerateFn, typename MakeFn, typename UnmakeFn>
std::uint64_t perft(BoardT& board,
					int depth,
					GenerateFn generate,
					MakeFn make,
					UnmakeFn unmake) {
	if (depth == 0) {
		return 1ULL;
	}

	std::uint64_t nodes = 0ULL;
	std::vector<MoveT> moves = generate(board);
	for (const MoveT& mv : moves) {
		make(board, mv);
		nodes += perft<BoardT, MoveT>(board, depth - 1, generate, make, unmake);
		unmake(board, mv);
	}
	return nodes;
}

// Runs perft for depths [1..maxDepth] and prints results like the screenshot
// Example usage:
//   runPerft(board, 6,
//     [](Board& b){ return generateMoves(b); },
//     [](Board& b, const Move& m){ makeMove(b,m); },
//     [](Board& b, const Move& m){ unmakeMove(b,m); }
//   );
template <typename BoardT, typename MoveT, typename GenerateFn, typename MakeFn, typename UnmakeFn>
void runPerft(BoardT& board,
			  int maxDepth,
			  GenerateFn generate,
			  MakeFn make,
			  UnmakeFn unmake,
			  bool showHeader = true) {
	if (showHeader) {
		Logger::log(LogLevel::INFO, "Running Test... (bulk-counting enabled)", __FILE__, __LINE__);
	}

	for (int d = 1; d <= maxDepth; ++d) {
		auto t0 = std::chrono::high_resolution_clock::now();
		std::uint64_t nodes = perft<BoardT, MoveT>(board, d, generate, make, unmake);
		auto t1 = std::chrono::high_resolution_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

		Logger::log(LogLevel::INFO, std::string("Depth: ") + std::to_string(d) + " ply  Result: " + formatWithCommas(nodes) + " positions  Time: " + std::to_string(ms) + " milliseconds", __FILE__, __LINE__);
	}
}

// Perft function that respects move filter at root level (for regular perft mode)
template <typename BoardT, typename MoveT, typename ColorT, typename UndoT, typename PieceT, typename ProfilerT, typename MoveToStringFn>
std::uint64_t perftWithFilter(BoardT& board, 
                              ColorT sideToMove, 
                              int depth,
                              ProfilerT& profiler,
                              MoveToStringFn moveToString,
                              const std::string& onlyMoveFilter = "",
                              bool enableBulkCount = true) {
    if (!onlyMoveFilter.empty()) {
        // Apply filter at root level only
        std::vector<MoveT> moves = board.getAllPseudoLegalMoves(sideToMove, true);
        std::uint64_t nodes = 0ULL;
        
        for (const MoveT& mv : moves) {
            if (moveToString(mv) != onlyMoveFilter) continue;
            
            UndoT u{};
            u = board.executeMove(mv, true);
            bool illegal = board.isKingInCheck(sideToMove);
            if (!illegal) {
                ColorT next = (sideToMove == WHITE ? BLACK : WHITE);
                nodes += perftOptimized<BoardT, MoveT, ColorT, UndoT, PieceT, ProfilerT>(board, next, depth - 1, profiler, enableBulkCount);
            }
            board.undoMove(mv, u);
        }
        return nodes;
    } else {
        // No filter, use optimized perft
        return perftOptimized<BoardT, MoveT, ColorT, UndoT, PieceT, ProfilerT>(board, sideToMove, depth, profiler, enableBulkCount);
    }
}

// Multi-threaded perft without split output (for regular perft mode)
template <typename BoardT, typename MoveT, typename ColorT, typename UndoT, typename RendererT, typename MoveToStringFn>
std::uint64_t perftMT(const BoardT& rootBoard, 
                      ColorT sideToMove, 
                      int depth, 
                      int maxThreads,
                      RendererT* renderer,
                      MoveToStringFn moveToString,
                      const std::string& onlyMoveFilter = "",
                      bool enableBulkCount = true,
                      bool disableLogging = false) {
    
    if (depth <= 1) {
        // For shallow depths, single-threaded is more efficient
        BoardT tempBoard(800, 800, 20.0f);
        tempBoard.setStartFEN(rootBoard.getStartFEN());
        tempBoard.initializeBoard(renderer);
        
        // Use a no-op profiler for single-threaded case
        struct NoOpProfiler {
            void startTimer(const std::string&) {}
            void endTimer(const std::string&) {}
        } noopProfiler;
        
        return perftWithFilter<BoardT, MoveT, ColorT, UndoT, decltype(tempBoard.getPieceAt(0,0)), NoOpProfiler>(
            tempBoard, sideToMove, depth, noopProfiler, moveToString, onlyMoveFilter, enableBulkCount);
    }

    std::vector<MoveT> moves;
    const_cast<BoardT&>(rootBoard).getAllPseudoLegalMoves(sideToMove, moves, true);
    if (moves.empty()) return 0ULL;

    // Filter moves if onlyMoveFilter is specified
    std::vector<MoveT> filteredMoves;
    for (const MoveT& mv : moves) {
        if (onlyMoveFilter.empty() || moveToString(mv) == onlyMoveFilter) {
            filteredMoves.push_back(mv);
        }
    }
    if (filteredMoves.empty()) return 0ULL;
    
    // Use requested threads or limit to available tasks
    int threads = (maxThreads > 0) ? std::min(maxThreads, (int)filteredMoves.size()) : (int)filteredMoves.size();

    ThreadPool pool(threads);
    std::vector<std::future<std::uint64_t>> futures;
    futures.reserve(filteredMoves.size());

    for (const MoveT& mv : filteredMoves) {
        futures.emplace_back(pool.enqueue([&, mv]() -> std::uint64_t {
            // Each thread gets isolated Board instance to prevent data races
            BoardT freshBoard(800, 800, 20.0f);
            freshBoard.setStartFEN(rootBoard.getStartFEN());
            freshBoard.initializeBoard(renderer);

            // No-op profiler for worker threads
            struct NoOpProfiler {
                void startTimer(const std::string&) {}
                void endTimer(const std::string&) {}
            } noopProfiler;

            // Match move on fresh board by positions and promotion type
            std::vector<MoveT> freshMoves = freshBoard.getAllPseudoLegalMoves(sideToMove, true);
            for (const MoveT& fm : freshMoves) {
                if (fm.startPos == mv.startPos && fm.endPos == mv.endPos && 
                    fm.isPromotion == mv.isPromotion && fm.promotionType == mv.promotionType) {
                    UndoT u{};
                    u = freshBoard.executeMove(fm);
                    bool illegal = freshBoard.isKingInCheck(sideToMove);
                    if (!illegal) {
                        ColorT next = (sideToMove == WHITE ? BLACK : WHITE);
                        std::uint64_t moveNodes = perftOptimized<BoardT, MoveT, ColorT, UndoT, decltype(freshBoard.getPieceAt(0,0)), NoOpProfiler>(
                            freshBoard, next, depth - 1, noopProfiler, enableBulkCount);
                        freshBoard.undoMove(fm, u);
                        return moveNodes;
                    }
                    freshBoard.undoMove(fm, u);
                    return 0ULL;
                }
            }
            // Log warning if move matching fails (but only in verbose mode)
            if (!disableLogging) {
                Logger::log(LogLevel::WARN, "perftMT: failed to apply top move on fresh board", __FILE__, __LINE__);
            }
            return 0ULL;
        }));
    }

    std::uint64_t totalNodes = 0ULL;
    for (auto& fut : futures) {
        totalNodes += fut.get();
    }
    
    return totalNodes;
}

// Multi-threaded perft with split output (shows nodes per move)
template <typename BoardT, typename MoveT, typename ColorT, typename UndoT, typename RendererT, typename MoveToStringFn>
std::uint64_t perftSplitMT(const BoardT& rootBoard, 
                           ColorT sideToMove, 
                           int depth, 
                           int maxThreads,
                           RendererT* renderer,
                           MoveToStringFn moveToString,
                           const std::string& onlyMoveFilter = "") {
    
    Logger::log(LogLevel::INFO, std::string("Perft split (mt) at depth ") + std::to_string(depth) + " threads=" + std::to_string(maxThreads), __FILE__, __LINE__);
    std::cout << "[perft_split_mt] launching with threads=" << maxThreads << " depth=" << depth << std::endl;

    std::vector<MoveT> moves;
    const_cast<BoardT&>(rootBoard).getAllPseudoLegalMoves(sideToMove, moves, true);
    if (moves.empty()) return 0ULL;

    // Filter moves and count planned tasks
    int plannedTasks = 0;
    for (const auto& m : moves) {
        if (onlyMoveFilter.empty() || moveToString(m) == onlyMoveFilter) ++plannedTasks;
    }
    if (plannedTasks == 0) return 0ULL;
    
    // Use requested threads or limit to available tasks
    int threads = (maxThreads > 0) ? std::min(maxThreads, plannedTasks) : plannedTasks;

    ThreadPool pool(threads);
    std::mutex coutMutex;
    std::vector<std::future<std::uint64_t>> futures;
    futures.reserve(plannedTasks);

    auto submitTask = [&](const MoveT& mv) {
        futures.emplace_back(pool.enqueue([&, mv]() -> std::uint64_t {
            // Each worker thread keeps a single Board instance to avoid constructing per task
            thread_local std::unique_ptr<BoardT> threadBoard;
            if (!threadBoard) {
                threadBoard = std::make_unique<BoardT>(800, 800, 20.0f);
                threadBoard->setStartFEN(rootBoard.getStartFEN());
                threadBoard->initializeBoard(renderer);
            }
            BoardT& freshBoard = *threadBoard;

            // No-op profiler for worker threads
            struct NoOpProfiler {
                void startTimer(const std::string&) {}
                void endTimer(const std::string&) {}
            } noopProfiler;

            // Match move on fresh board by positions and promotion type
            std::vector<MoveT> freshMoves;
            freshBoard.getAllPseudoLegalMoves(sideToMove, freshMoves, true);
            for (const MoveT& fm : freshMoves) {
                if (fm.startPos == mv.startPos && fm.endPos == mv.endPos && 
                    fm.isPromotion == mv.isPromotion && fm.promotionType == mv.promotionType) {
                    UndoT u{};
                    u = freshBoard.executeMove(fm);
                    bool illegal = freshBoard.isKingInCheck(sideToMove);
                    std::uint64_t moveNodes = 0ULL;
                    if (!illegal) {
                        ColorT next = (sideToMove == WHITE ? BLACK : WHITE);
                        moveNodes = perftOptimized<BoardT, MoveT, ColorT, UndoT, decltype(freshBoard.getPieceAt(0,0)), NoOpProfiler>(
                            freshBoard, next, depth - 1, noopProfiler, true);
                    }
                    freshBoard.undoMove(fm, u);

                    if (!illegal) {
                        std::lock_guard<std::mutex> lk(coutMutex);
                        std::cout << moveToString(fm) << ": " << moveNodes << std::endl;
                    }
                    return moveNodes;
                }
            }
            Logger::log(LogLevel::WARN, "perftSplitMT: failed to apply top move on fresh board", __FILE__, __LINE__);
            return 0ULL;
        }));
    };

    for (const MoveT& mv : moves) {
        if (!onlyMoveFilter.empty() && moveToString(mv) != onlyMoveFilter) continue;
        submitTask(mv);
    }

    std::uint64_t totalNodes = 0ULL;
    for (auto& fut : futures) {
        totalNodes += fut.get();
    }
    return totalNodes;
}

// Single-threaded perft with split output (shows nodes per move)
template <typename BoardT, typename MoveT, typename ColorT, typename UndoT, typename PieceT, typename ProfilerT, typename MoveToStringFn>
std::uint64_t perftSplit(BoardT& board, 
                         ColorT sideToMove, 
                         int depth,
                         ProfilerT& profiler,
                         MoveToStringFn moveToString,
                         const std::string& onlyMoveFilter = "") {
    Logger::log(LogLevel::INFO, std::string("Perft split at depth ") + std::to_string(depth), __FILE__, __LINE__);
    std::uint64_t totalNodes = 0ULL;

    std::vector<MoveT> moves = board.getAllPseudoLegalMoves(sideToMove, true);

    for (const MoveT& mv : moves) {
        if (!onlyMoveFilter.empty() && moveToString(mv) != onlyMoveFilter) continue;
        
        UndoT u{};
        profiler.startTimer("make_move_top");
        u = board.executeMove(mv);
        profiler.endTimer("make_move_top");
        bool illegal = board.isKingInCheck(sideToMove);
        std::uint64_t moveNodes = 0ULL;
        if (!illegal) {
            ColorT next = (sideToMove == WHITE ? BLACK : WHITE);
            moveNodes = perftOptimized<BoardT, MoveT, ColorT, UndoT, PieceT, ProfilerT>(board, next, depth - 1, profiler, true);
            totalNodes += moveNodes;
        }

        profiler.startTimer("unmake_move_top");
        board.undoMove(mv, u);
        profiler.endTimer("unmake_move_top");

        if (!illegal) {
            std::cout << moveToString(mv) << ": " << moveNodes << std::endl;
        }
    }

    Logger::log(LogLevel::INFO, std::string("\nNodes searched: ") + std::to_string(totalNodes), __FILE__, __LINE__);
    std::cout << "\nNodes searched: " << totalNodes << std::endl;
    return totalNodes;
}

} // namespace chess

#endif // CHESS_AI_UTILS_H


