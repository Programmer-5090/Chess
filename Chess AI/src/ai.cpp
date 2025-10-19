#include <chess/AI/ai.h>
#include <chess/board/board.h>
#include <chess/board/piece_manager.h>
#include <chess/utils/thread_pool.h>
#include <chess/board/move_executor.h>
#include <chess/board/pieces/pieces.h>
#include <chess/utils/profiler.h>
#include <algorithm>
#include <future>
#include <mutex>
#include <memory>
#include <vector>
#include <iostream>
#include <atomic>
#include <limits>
#include <iomanip>

AI::AI(Board& b) : board(b) {
    threadPool = new ThreadPool(std::thread::hardware_concurrency());
}

AI::~AI() {
    delete threadPool;
    threadPool = nullptr;
}

int AI::evaluatePosition(Board& b) {
    int whiteScore = countPieces(b, WHITE);
    int blackScore = countPieces(b, BLACK);

    int evaluation = whiteScore - blackScore;
    // Further evaluation logic can be added here

    // For now i am using a simple score difference
    int playerMultiplier = (b.getCurrentPlayer() == WHITE) ? 1 : -1;
    int finalScore = evaluation * playerMultiplier;

    return finalScore;
}

Move AI::getBestMove(int depth) {
    // Start timing and reset counters
    auto startTime = std::chrono::high_resolution_clock::now();
    resetNodesSearched();
    g_profiler.clear(); // Clear previous profiler data
    
    // Disable detailed profiling during search for performance
    bool profilerWasEnabled = g_profiler.isEnabled();
    g_profiler.setEnabled(false);
    
    g_profiler.startTimer("ai_getBestMove");
    
    g_profiler.startTimer("ai_move_generation");
    std::vector<Move> moves = board.getAllPseudoLegalMoves(board.getCurrentPlayer(), true);
    g_profiler.endTimer("ai_move_generation");
    
    if (moves.empty()) {
        g_profiler.endTimer("ai_getBestMove");
        // Return a default invalid move
        return Move{};
    }

    Move bestMove;
    bool foundLegalMove = false;
    int bestScore = (board.getCurrentPlayer() == WHITE) ? -100000 : 100000;
    Color currentPlayer = board.getCurrentPlayer();
    
    // Use multithreading for deeper searches at root level
    if (depth >= 3 && moves.size() > 1) {
        g_profiler.startTimer("ai_mt_root_search");
        
        std::vector<std::future<std::pair<Move, int>>> futures;
        std::vector<Move> legalMoves;
        
        // First pass: find all legal moves
        for (const auto& move : moves) {
            UndoMove undoInfo = board.executeMove(move);
            bool isLegal = !board.isKingInCheck(move.piece->getColor());
            board.undoMove(move, undoInfo);
            if (isLegal) {
                legalMoves.push_back(move);
            }
        }
        
        if (legalMoves.empty()) {
            g_profiler.endTimer("ai_mt_root_search");
            g_profiler.endTimer("ai_getBestMove");
            g_profiler.setEnabled(profilerWasEnabled);
            return Move{};
        }
        
        if (legalMoves.size() == 1) {
            g_profiler.endTimer("ai_mt_root_search");
            g_profiler.endTimer("ai_getBestMove");
            g_profiler.setEnabled(profilerWasEnabled);
            return legalMoves[0];
        }
        
        // Multi-threaded evaluation of each legal move
        for (const auto& move : legalMoves) {
            futures.emplace_back(threadPool->enqueue([this, move, depth, currentPlayer]() -> std::pair<Move, int> {
                // Create board copy for thread safety
                Board boardCopy(800, 800, 20.0f);
                boardCopy.setStartFEN(board.getCurrentFEN());
                boardCopy.initializeBoard(nullptr);
                
                // Make the move and search
                UndoMove undoInfo = boardCopy.executeMove(move);
                int score = search_recursive(boardCopy, depth - 1, true, -100000, 100000);
                
                return std::make_pair(move, score);
            }));
        }
        
        // Collect results and find best move
        for (auto& future : futures) {
            auto result = future.get();
            Move move = result.first;
            int score = result.second;
            nodesSearched.fetch_add(1); // Count root nodes
            
            if ((currentPlayer == WHITE && score > bestScore) ||
                (currentPlayer == BLACK && score < bestScore)) {
                bestScore = score;
                bestMove = move;
                foundLegalMove = true;
            }
        }
        
        g_profiler.endTimer("ai_mt_root_search");
    } else {
        // Single-threaded search for shallow depths
        g_profiler.startTimer("ai_search_loop");
        // Search through moves using perft-style legality checking
        for (const auto& move : moves) {
            g_profiler.startTimer("ai_make_move");
        // Make the move first, then check legality (like perft does)
        UndoMove undoInfo = board.executeMove(move);
        g_profiler.endTimer("ai_make_move");
        
        // Check if this move leaves our king in check (illegal move)
        g_profiler.startTimer("ai_legality_check");
        bool isLegal = !board.isKingInCheck(move.piece->getColor());
        g_profiler.endTimer("ai_legality_check");
        
        if (isLegal) {
            if (!foundLegalMove) {
                // First legal move found
                bestMove = move;
                foundLegalMove = true;
                
                // If only searching depth 1 or we want quick moves, don't search deeper
                if (depth <= 1) {
                    nodesSearched.fetch_add(1);
                    g_profiler.startTimer("ai_undo_move");
                    board.undoMove(move, undoInfo);
                    g_profiler.endTimer("ai_undo_move");
                    break; // Take the first legal move for speed
                }
            }
            
            g_profiler.startTimer("ai_search_recursive_call");
            // Search from the opponent's perspective
            int score = search_recursive(board, depth - 1, true, -100000, 100000);
            g_profiler.endTimer("ai_search_recursive_call");
            
            // Update best move based on current player
            if ((currentPlayer == WHITE && score > bestScore) ||
                (currentPlayer == BLACK && score < bestScore)) {
                bestScore = score;
                bestMove = move;
            }
            }
        
            g_profiler.startTimer("ai_undo_move");
            // Always undo the move
            board.undoMove(move, undoInfo);
            g_profiler.endTimer("ai_undo_move");
        }
        g_profiler.endTimer("ai_search_loop");
    }
    
    // Record timing information
    auto endTime = std::chrono::high_resolution_clock::now();
    lastSearchTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    lastSearchNodes = nodesSearched.load();
    
    g_profiler.endTimer("ai_getBestMove");
    
    // Re-enable profiler to original state
    g_profiler.setEnabled(profilerWasEnabled);
    
    // Return best legal move found, or invalid move if none found
    if (!foundLegalMove) {
        return Move{}; // Invalid move
    }
    return bestMove;
}

int AI::countPieces(Board& b, Color color) {
    int count = 0;
    for (const auto& piece : b.pieceManager->getPieces(color)) {
        count += piece->getValue();
    }
    return count;
}

int AI::search(int depth, int alpha, int beta) {
    g_profiler.startTimer("ai_search");
    
    // Use single-threaded for shallow depths where overhead isn't worth it
    if (depth <= 2) {
        int result = search_recursive(board, depth, true, alpha, beta);
        g_profiler.endTimer("ai_search");
        return result;
    }

    // Multi-threaded for deeper searches
    std::vector<Move> moves = board.getAllPseudoLegalMoves(board.getCurrentPlayer(), true);
    orderMoves(board, moves);

    if (moves.empty()) {
        g_profiler.endTimer("ai_search");
        if (board.isKingInCheck(board.getCurrentPlayer())) {
            return (board.getCurrentPlayer() == WHITE) ? -10000 : 10000;
        } else {
            return 0;
        }
    }

    // Use optimized multithreading with minimal board copying
    std::vector<std::future<int>> futures;
    std::mutex bestScoreMutex;
    
    if (board.getCurrentPlayer() == WHITE) {
        int bestScore = -100000;
        std::atomic<int> shared_alpha = alpha;

        for (const auto& move : moves) {
            futures.emplace_back(threadPool->enqueue([this, move, depth, &shared_alpha, beta, &bestScore, &bestScoreMutex]() -> int {
                // Create minimal board copy only for thread safety
                Board boardCopy(800, 800, 20.0f);
                boardCopy.setStartFEN(board.getCurrentFEN());
                boardCopy.initializeBoard(nullptr);
                
                // Apply move using perft-style approach
                UndoMove undoInfo = boardCopy.executeMove(move);
                if (boardCopy.isKingInCheck(move.piece->getColor())) {
                    // Illegal move
                    return -10000;
                }
                
                // Search deeper with optimized recursive function
                int result = search_recursive(boardCopy, depth - 1, true, shared_alpha, beta);
                
                // Update shared alpha for pruning
                {
                    std::lock_guard<std::mutex> lock(bestScoreMutex);
                    bestScore = std::max(bestScore, result);
                    shared_alpha = std::max((int)shared_alpha, result);
                }
                
                return result;
            }));
        }

        for (auto& future : futures) {
            int score = future.get();
            bestScore = std::max(bestScore, score);
        }
        g_profiler.endTimer("ai_search");
        return bestScore;
        
    } else { // BLACK's turn
        int bestScore = 100000;
        std::atomic<int> shared_beta = beta;

        for (const auto& move : moves) {
            futures.emplace_back(threadPool->enqueue([this, move, depth, alpha, &shared_beta, &bestScore, &bestScoreMutex]() -> int {
                // Create minimal board copy only for thread safety
                Board boardCopy(800, 800, 20.0f);
                boardCopy.setStartFEN(board.getCurrentFEN());
                boardCopy.initializeBoard(nullptr);
                
                // Apply move using perft-style approach
                UndoMove undoInfo = boardCopy.executeMove(move);
                if (boardCopy.isKingInCheck(move.piece->getColor())) {
                    // Illegal move
                    return 10000;
                }
                
                // Search deeper with optimized recursive function
                int result = search_recursive(boardCopy, depth - 1, true, alpha, shared_beta);
                
                // Update shared beta for pruning
                {
                    std::lock_guard<std::mutex> lock(bestScoreMutex);
                    bestScore = std::min(bestScore, result);
                    shared_beta = std::min((int)shared_beta, result);
                }
                
                return result;
            }));
        }

        for (auto& future : futures) {
            int score = future.get();
            bestScore = std::min(bestScore, score);
        }
        g_profiler.endTimer("ai_search");
        return bestScore;
    }
}

int AI::search_recursive(Board& b, int depth, bool enableBulkCount, int alpha, int beta) {
    // Count this node
    nodesSearched.fetch_add(1);
    
    if (depth == 0) {
        // Simple leaf evaluation instead of expensive position analysis
        // Just return material balance or a simple heuristic
        return (b.getCurrentPlayer() == WHITE) ? 1 : -1;
    }

    std::vector<Move> moves = b.getAllPseudoLegalMoves(b.getCurrentPlayer(), true);
    
    // Bulk counting optimization at depth 1
    if (depth == 1 && enableBulkCount) {
        int legalMoveCount = 0;
        for (const auto& move : moves) {
            UndoMove undoInfo = b.executeMove(move);
            if (!b.isKingInCheck(move.piece->getColor())) {
                legalMoveCount++;
                nodesSearched.fetch_add(1); // Count each leaf node in bulk counting
            }
            b.undoMove(move, undoInfo);
        }
        // Return a simple evaluation based on move count
        return (b.getCurrentPlayer() == WHITE) ? legalMoveCount : -legalMoveCount;
    }
    
    orderMoves(b, moves);

    if (moves.empty()) {
        if (b.isKingInCheck(b.getCurrentPlayer())) {
            return (b.getCurrentPlayer() == WHITE) ? -10000 : 10000;
        } else {
            return 0;
        }
    }

    if (b.getCurrentPlayer() == WHITE) {
        int maxEval = -100000;
        bool hasLegalMove = false;
        for (const auto& move : moves) {
            UndoMove undoInfo = b.executeMove(move);
            // Check legality after making the move (like perft does)
            if (!b.isKingInCheck(move.piece->getColor())) {
                hasLegalMove = true;
                int eval = search_recursive(b, depth - 1, enableBulkCount, alpha, beta);
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                b.undoMove(move, undoInfo);
                if (beta <= alpha) {
                    break; // Beta cutoff
                }
            } else {
                b.undoMove(move, undoInfo);
            }
        }
        // If no legal moves, check for checkmate or stalemate
        if (!hasLegalMove) {
            return b.isKingInCheck(b.getCurrentPlayer()) ? -10000 : 0;
        }
        return maxEval;
    } else { // BLACK's turn
        int minEval = 100000;
        bool hasLegalMove = false;
        for (const auto& move : moves) {
            UndoMove undoInfo = b.executeMove(move);
            // Check legality after making the move (like perft does)
            if (!b.isKingInCheck(move.piece->getColor())) {
                hasLegalMove = true;
                int eval = search_recursive(b, depth - 1, enableBulkCount, alpha, beta);
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                b.undoMove(move, undoInfo);
                if (beta <= alpha) {
                    break; // Alpha cutoff
                }
            } else {
                b.undoMove(move, undoInfo);
            }
        }
        // If no legal moves, check for checkmate or stalemate  
        if (!hasLegalMove) {
            return b.isKingInCheck(b.getCurrentPlayer()) ? 10000 : 0;
        }
        return minEval;
    }
}

void AI::orderMoves(Board& gameBoard, std::vector<Move>& moves) {
    std::sort(moves.begin(), moves.end(), [&gameBoard](const Move& a, const Move& b) {
        int scoreA = 0;
        int scoreB = 0;

        if (a.capturedPiece) scoreA += 10 * a.capturedPiece->getValue() - a.piece->getValue();
        if (b.capturedPiece) scoreB += 10 * b.capturedPiece->getValue() - b.piece->getValue();
        
        if (a.isPromotion) scoreA += a.promotionType;
        if (b.isPromotion) scoreB += b.promotionType;

        // En passant and castling scoring (these are methods, not properties)
        if (a.isCastling()) scoreA += 100;
        if (b.isCastling()) scoreB += 100;

        // Lower the score of moves to squares that are attacked by the opponent
        if (gameBoard.isSquareAttacked(a.endPos.first, a.endPos.second, (a.piece->getColor() == WHITE ? BLACK : WHITE))) {
            scoreA -= 50;
        }
        if (gameBoard.isSquareAttacked(b.endPos.first, b.endPos.second, (b.piece->getColor() == WHITE ? BLACK : WHITE))) {
            scoreB -= 50;
        }

        return scoreA > scoreB;
    });
}

void AI::printPerformanceStats() const {
    // Helper function to format numbers with commas
    auto formatNumber = [](std::uint64_t num) -> std::string {
        std::string s = std::to_string(num);
        std::string formatted;
        int count = 0;
        for (int i = s.length() - 1; i >= 0; --i) {
            if (count > 0 && count % 3 == 0) {
                formatted = "," + formatted;
            }
            formatted = s[i] + formatted;
            count++;
        }
        return formatted;
    };
    
    // Temporarily disable profiler logging to console to avoid interference
    bool profilerWasEnabled = g_profiler.isEnabled();
    g_profiler.setEnabled(false);
    
    std::cout << "\n========== AI Performance Analysis ==========\n";
    std::cout << "Total Search Time: " << std::fixed << std::setprecision(2) << lastSearchTimeMs << " ms\n";
    std::cout << "Nodes Searched:    " << formatNumber(lastSearchNodes) << " nodes\n";
    
    if (lastSearchTimeMs > 0) {
        double nodesPerSecond = (lastSearchNodes * 1000.0) / lastSearchTimeMs;
        std::cout << "Search Speed:      " << formatNumber(static_cast<std::uint64_t>(nodesPerSecond)) << " nodes/sec\n";
        
        // Compare to perft speed (approximate baseline)
        double perfEstimate = 50000000.0; // ~50M nodes/sec baseline for perft
        double efficiency = (nodesPerSecond / perfEstimate) * 100.0;
        std::cout << "Efficiency:        " << std::setprecision(1) << efficiency << "% of perft speed\n";
    }
    
    // Get top time-consuming functions from profiler
    std::cout << "\nFunction Performance Breakdown:\n";
    std::cout << "Function                    | Time (ms) | %Total | Calls | Avg (ms)\n";
    std::cout << "----------------------------|-----------|--------|-------|----------\n";
    
    auto detailedItems = g_profiler.getDetailedItems();
    
    // Show top 10 functions by inclusive time
    int count = 0;
    for (const auto& item : detailedItems) {
        if (count >= 10) break;
        if (item.inclusive_us < 500) break; // Skip functions taking less than 0.5ms
        
        double ms = item.inclusive_us / 1000.0;
        double avgMs = item.calls > 0 ? ms / item.calls : 0.0;
        double percentage = (lastSearchTimeMs > 0) ? (ms / lastSearchTimeMs) * 100.0 : 0.0;
        
        // Format the function name (truncate if too long)
        std::string funcName = item.name;
        if (funcName.length() > 26) {
            funcName = funcName.substr(0, 23) + "...";
        }
        
        std::cout << std::left << std::setw(27) << funcName << " | "
                  << std::right << std::setw(8) << std::fixed << std::setprecision(2) << ms << " | "
                  << std::setw(5) << std::setprecision(1) << percentage << "% | "
                  << std::setw(5) << item.calls << " | "
                  << std::setw(7) << std::setprecision(3) << avgMs << "\n";
        count++;
    }
    
    std::cout << "============================================\n\n";
    
    // Re-enable profiler to its original state
    g_profiler.setEnabled(profilerWasEnabled);
}