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
    int playerMultiplier = (b.getCurrentPlayer() == WHITE) ? 1 : -1;
    int finalScore = evaluation * playerMultiplier;

    return finalScore;
}

Move AI::getBestMove(int depth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    resetNodesSearched();
    g_profiler.clear();
    bool profilerWasEnabled = g_profiler.isEnabled();
    g_profiler.setEnabled(false);

    g_profiler.startTimer("ai_getBestMove");

    g_profiler.startTimer("ai_move_generation");
    std::vector<Move> moves = board.getAllPseudoLegalMoves(board.getCurrentPlayer(), true);
    g_profiler.endTimer("ai_move_generation");

    if (moves.empty()) {
        g_profiler.endTimer("ai_getBestMove");
        return Move{};
    }

    Move bestMove;
    bool foundLegalMove = false;
    int bestScore = (board.getCurrentPlayer() == WHITE) ? -100000 : 100000;
    Color currentPlayer = board.getCurrentPlayer();

    if (depth >= 3 && moves.size() > 1) {
        g_profiler.startTimer("ai_mt_root_search");

        std::vector<std::future<std::pair<Move, int>>> futures;
        std::vector<Move> legalMoves;

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

        for (const auto& move : legalMoves) {
            futures.emplace_back(threadPool->enqueue([this, move, depth, currentPlayer]() -> std::pair<Move, int> {
                Board boardCopy(800, 800, 20.0f);
                boardCopy.setStartFEN(board.getCurrentFEN());
                boardCopy.initializeBoard(nullptr);
                UndoMove undoInfo = boardCopy.executeMove(move);
                int score = search_recursive(boardCopy, depth - 1, true, -100000, 100000);

                return std::make_pair(move, score);
            }));
        }

        for (auto& future : futures) {
            auto result = future.get();
            Move move = result.first;
            int score = result.second;
            nodesSearched.fetch_add(1);

            if ((currentPlayer == WHITE && score > bestScore) ||
                (currentPlayer == BLACK && score < bestScore)) {
                bestScore = score;
                bestMove = move;
                foundLegalMove = true;
            }
        }

        g_profiler.endTimer("ai_mt_root_search");
    } else {
        g_profiler.startTimer("ai_search_loop");
        for (const auto& move : moves) {
            g_profiler.startTimer("ai_make_move");
            UndoMove undoInfo = board.executeMove(move);
            g_profiler.endTimer("ai_make_move");

            g_profiler.startTimer("ai_legality_check");
            bool isLegal = !board.isKingInCheck(move.piece->getColor());
            g_profiler.endTimer("ai_legality_check");

            if (isLegal) {
                if (!foundLegalMove) {
                    bestMove = move;
                    foundLegalMove = true;

                    if (depth <= 1) {
                        nodesSearched.fetch_add(1);
                        g_profiler.startTimer("ai_undo_move");
                        board.undoMove(move, undoInfo);
                        g_profiler.endTimer("ai_undo_move");
                        break;
                    }
                }

                g_profiler.startTimer("ai_search_recursive_call");
                int score = search_recursive(board, depth - 1, true, -100000, 100000);
                g_profiler.endTimer("ai_search_recursive_call");

                if ((currentPlayer == WHITE && score > bestScore) ||
                    (currentPlayer == BLACK && score < bestScore)) {
                    bestScore = score;
                    bestMove = move;
                }
            }

            g_profiler.startTimer("ai_undo_move");
            board.undoMove(move, undoInfo);
            g_profiler.endTimer("ai_undo_move");
        }
        g_profiler.endTimer("ai_search_loop");
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    lastSearchTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    lastSearchNodes = nodesSearched.load();

    g_profiler.endTimer("ai_getBestMove");

    g_profiler.setEnabled(profilerWasEnabled);

    if (!foundLegalMove) {
        return Move{};
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

    if (depth <= 2) {
        int result = search_recursive(board, depth, true, alpha, beta);
        g_profiler.endTimer("ai_search");
        return result;
    }

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

    std::vector<std::future<int>> futures;
    std::mutex bestScoreMutex;

    if (board.getCurrentPlayer() == WHITE) {
        int bestScore = -100000;
        std::atomic<int> shared_alpha = alpha;

        for (const auto& move : moves) {
            futures.emplace_back(threadPool->enqueue([this, move, depth, &shared_alpha, beta, &bestScore, &bestScoreMutex]() -> int {
                Board boardCopy(800, 800, 20.0f);
                boardCopy.setStartFEN(board.getCurrentFEN());
                boardCopy.initializeBoard(nullptr);

                UndoMove undoInfo = boardCopy.executeMove(move);
                if (boardCopy.isKingInCheck(move.piece->getColor())) {
                    return -10000;
                }

                int result = search_recursive(boardCopy, depth - 1, true, shared_alpha, beta);

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
                Board boardCopy(800, 800, 20.0f);
                boardCopy.setStartFEN(board.getCurrentFEN());
                boardCopy.initializeBoard(nullptr);

                UndoMove undoInfo = boardCopy.executeMove(move);
                if (boardCopy.isKingInCheck(move.piece->getColor())) {
                    return 10000;
                }

                int result = search_recursive(boardCopy, depth - 1, true, alpha, shared_beta);

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
    nodesSearched.fetch_add(1);

    if (depth == 0) {
        return (b.getCurrentPlayer() == WHITE) ? 1 : -1;
    }

    std::vector<Move> moves = b.getAllPseudoLegalMoves(b.getCurrentPlayer(), true);

    if (depth == 1 && enableBulkCount) {
        int legalMoveCount = 0;
        for (const auto& move : moves) {
            UndoMove undoInfo = b.executeMove(move);
            if (!b.isKingInCheck(move.piece->getColor())) {
                legalMoveCount++;
                nodesSearched.fetch_add(1);
            }
            b.undoMove(move, undoInfo);
        }
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
            if (!b.isKingInCheck(move.piece->getColor())) {
                hasLegalMove = true;
                int eval = search_recursive(b, depth - 1, enableBulkCount, alpha, beta);
                maxEval = std::max(maxEval, eval);
                alpha = std::max(alpha, eval);
                b.undoMove(move, undoInfo);
                if (beta <= alpha) {
                    break;
                }
            } else {
                b.undoMove(move, undoInfo);
            }
        }
        if (!hasLegalMove) {
            return b.isKingInCheck(b.getCurrentPlayer()) ? -10000 : 0;
        }
        return maxEval;
    } else { // BLACK's turn
        int minEval = 100000;
        bool hasLegalMove = false;
        for (const auto& move : moves) {
            UndoMove undoInfo = b.executeMove(move);
            if (!b.isKingInCheck(move.piece->getColor())) {
                hasLegalMove = true;
                int eval = search_recursive(b, depth - 1, enableBulkCount, alpha, beta);
                minEval = std::min(minEval, eval);
                beta = std::min(beta, eval);
                b.undoMove(move, undoInfo);
                if (beta <= alpha) {
                    break;
                }
            } else {
                b.undoMove(move, undoInfo);
            }
        }
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

        if (a.isCastling()) scoreA += 100;
        if (b.isCastling()) scoreB += 100;

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

    bool profilerWasEnabled = g_profiler.isEnabled();
    g_profiler.setEnabled(false);

    std::cout << "\n========== AI Performance Analysis ==========\n";
    std::cout << "Total Search Time: " << std::fixed << std::setprecision(2) << lastSearchTimeMs << " ms\n";
    std::cout << "Nodes Searched:    " << formatNumber(lastSearchNodes) << " nodes\n";

    if (lastSearchTimeMs > 0) {
        double nodesPerSecond = (lastSearchNodes * 1000.0) / lastSearchTimeMs;
        std::cout << "Search Speed:      " << formatNumber(static_cast<std::uint64_t>(nodesPerSecond)) << " nodes/sec\n";

        double perfEstimate = 50000000.0; // ~50M nodes/sec baseline for perft
        double efficiency = (nodesPerSecond / perfEstimate) * 100.0;
        std::cout << "Efficiency:        " << std::setprecision(1) << efficiency << "% of perft speed\n";
    }

    std::cout << "\nFunction Performance Breakdown:\n";
    std::cout << "Function                    | Time (ms) | %Total | Calls | Avg (ms)\n";
    std::cout << "----------------------------|-----------|--------|-------|----------\n";

    auto detailedItems = g_profiler.getDetailedItems();
    int count = 0;
    for (const auto& item : detailedItems) {
        if (count >= 10) break;
        if (item.inclusive_us < 500) break;

        double ms = item.inclusive_us / 1000.0;
        double avgMs = item.calls > 0 ? ms / item.calls : 0.0;
        double percentage = (lastSearchTimeMs > 0) ? (ms / lastSearchTimeMs) * 100.0 : 0.0;
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

    g_profiler.setEnabled(profilerWasEnabled);
}