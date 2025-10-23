#include <chess/AI/ai_bb.h>
#include <chess/board/boardBB.h>
#include <chess/board/bitboard/move.h>
#include <chess/board/bitboard/board_state.h>
#include <chess/board/bitboard/move_generator_bb.h>
#include <chess/board/bitboard/move_exec.h>
#include <chess/board/bitboard/transpositionTable.h>
#include <chess/board/bitboard/precomputed_data.h>
#include <chess/board/pieces/piece_const.h>
#include <chess/AI/pieceST.h>
#include <algorithm>
#include <memory>
#include <vector>
#include <limits>
#include <cmath>
#include <future>
#include <thread>

AI_BB::AI_BB(unsigned int numThreads) : threadCount(numThreads) {
    useIterativeDeepening = true;
    useTranspositionTable = true;
    useMoveOrdering = true;
    
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 1;
        threadCount = numThreads;
    }
    
    if (threadCount > 1) {
        try {
            threadPool = std::make_unique<ThreadPool>(threadCount);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create thread pool: " << e.what() << std::endl;
            threadPool = nullptr;
            threadCount = 1;
        }
    }
}

AI_BB::~AI_BB() {
    if (threadPool) {
        threadPool->shutdown();
    }
}

void AI_BB::updateSettings(const Settings& newSettings) {
    useIterativeDeepening = newSettings.useIterativeDeepening;
    useTranspositionTable = newSettings.useTranspositionTable;
    useMoveOrdering = newSettings.useMoveOrdering;
    abortSearch = newSettings.exitSearch;
}

int AI_BB::evaluate(BoardBB& board) {
    int whiteEval = 0;
    int blackEval = 0;
    
    // (0=white, 1=black), not COLOR bit flags
    int whiteMaterial = countMaterial(board, 0);  // 0 = white array index
    int blackMaterial = countMaterial(board, 1);  // 1 = black array index
    
    int whiteMaterialWithoutPawns = whiteMaterial - board.bbState->pawns[0].count() * PAWN_VALUE;
    int blackMaterialWithoutPawns = blackMaterial - board.bbState->pawns[1].count() * PAWN_VALUE;
    
    float whiteEndgamePhaseWeight = endgamePhaseWeight(whiteMaterialWithoutPawns);
    float blackEndgamePhaseWeight = endgamePhaseWeight(blackMaterialWithoutPawns);
    
    whiteEval += whiteMaterial;
    blackEval += blackMaterial;
    
    whiteEval += mopUpEval(board, 0, 1, whiteMaterial, blackMaterial, blackEndgamePhaseWeight);
    blackEval += mopUpEval(board, 1, 0, blackMaterial, whiteMaterial, whiteEndgamePhaseWeight);
    
    whiteEval += evaluatePieceSquareTables(board, 0, blackEndgamePhaseWeight);
    blackEval += evaluatePieceSquareTables(board, 1, whiteEndgamePhaseWeight);
    
    int eval = whiteEval - blackEval;
    int perspective = board.bbState->whiteToMove ? 1 : -1;
    return eval * perspective;
}

int AI_BB::countMaterial(BoardBB& board, int colorIdx) {
    int material = 0;
    material += board.bbState->pawns[colorIdx].count() * PAWN_VALUE;
    material += board.bbState->knights[colorIdx].count() * KNIGHT_VALUE;
    material += board.bbState->bishops[colorIdx].count() * BISHOP_VALUE;
    material += board.bbState->rooks[colorIdx].count() * ROOK_VALUE;
    material += board.bbState->queens[colorIdx].count() * QUEEN_VALUE;
    return material;
}

int AI_BB::getPieceValue(int pieceType) const {
    switch (pieceType) {
        case chess::PIECE_PAWN:   return PAWN_VALUE;
        case chess::PIECE_KNIGHT: return KNIGHT_VALUE;
        case chess::PIECE_BISHOP: return BISHOP_VALUE;
        case chess::PIECE_ROOK:   return ROOK_VALUE;
        case chess::PIECE_QUEEN:  return QUEEN_VALUE;
        default:                  return 0;
    }
}

float AI_BB::endgamePhaseWeight(int materialCountWithoutPawns) const {
    const float endgameMaterialStart = ROOK_VALUE * 2 + BISHOP_VALUE + KNIGHT_VALUE;
    const float multiplier = 1.0f / endgameMaterialStart;
    return 1.0f - std::min(1.0f, materialCountWithoutPawns * multiplier);
}

int AI_BB::mopUpEval(BoardBB& board, int friendlyIdx, int opponentIdx, int myMaterial, int opponentMaterial, float endgameWeight) {
    int mopUpScore = 0;
    if (myMaterial > opponentMaterial + PAWN_VALUE * 2 && endgameWeight > 0) {
        int friendlyKingSquare = board.bbState->kingSquare[friendlyIdx];
        int opponentKingSquare = board.bbState->kingSquare[opponentIdx];
        
        // Safety check for valid king positions
        if (friendlyKingSquare < 0 || friendlyKingSquare >= 64 || 
            opponentKingSquare < 0 || opponentKingSquare >= 64) {
            return 0;
        }
        
        // Manhattan distance from center
        int oppKingFile = opponentKingSquare % 8;
        int oppKingRank = opponentKingSquare / 8;
        int centerManhattanDistance = std::abs(oppKingFile - 3) + std::abs(oppKingRank - 3) + 
                                      std::abs(oppKingFile - 4) + std::abs(oppKingRank - 4);
        mopUpScore += centerManhattanDistance * 10;
        
        // Orthogonal distance between kings
        int fileDist = std::abs((friendlyKingSquare % 8) - (opponentKingSquare % 8));
        int rankDist = std::abs((friendlyKingSquare / 8) - (opponentKingSquare / 8));
        mopUpScore += (14 - (fileDist + rankDist)) * 4;
        
        return static_cast<int>(mopUpScore * endgameWeight);
    }
    return 0;
}

int AI_BB::evaluatePieceSquareTables(BoardBB& board, int colorIdx, float endgamePhaseWeight) {
    int value = 0;
    bool isWhite = (colorIdx == chess::COLOR_WHITE);
    
    value += evaluatePieceSquareTable(PawnTable, board.bbState->pawns[colorIdx], isWhite);
    value += evaluatePieceSquareTable(RookTable, board.bbState->rooks[colorIdx], isWhite);
    value += evaluatePieceSquareTable(KnightTable, board.bbState->knights[colorIdx], isWhite);
    value += evaluatePieceSquareTable(BishopTable, board.bbState->bishops[colorIdx], isWhite);
    value += evaluatePieceSquareTable(QueenTable, board.bbState->queens[colorIdx], isWhite);
    
    // King evaluation with phase blending
    int kingSq = board.bbState->kingSquare[colorIdx];
    if (kingSq >= 0 && kingSq < 64) {
        int sq = isWhite ? kingSq : (63 - kingSq);
        int kingEarlyPhase = KingTable[sq];
        value += static_cast<int>(kingEarlyPhase * (1.0f - endgamePhaseWeight));
    }
    
    return value;
}

int AI_BB::evaluatePieceSquareTable(const std::array<short, 64>& table, const chess::PieceList& pieceList, bool isWhite) {
    int value = 0;
    for (int i = 0; i < pieceList.count(); ++i) {
        int sq = pieceList.squares[i];
        if (!isWhite) {
            sq = 63 - sq;
        }
        if (sq >= 0 && sq < 64) {
            value += table[sq];
        }
    }
    return value;
}

std::pair<chess::BBMove, int> AI_BB::getSearchResult(BoardBB& board, int depth) {
    std::unique_ptr<TranspositionTable> transpositionTable; // size in MB
    try {
        transpositionTable = std::make_unique<TranspositionTable>(board, 16);
    } catch (const std::exception& e) {
        std::cerr << "[AI ERROR] Failed to initialize TT: " << e.what() << std::endl;
        return {chess::BBMove(), 0};
    }
    
    bestEvalThisIteration = bestEval = 0;
    bestMoveThisIteration = bestMove = chess::BBMove();
    currentIterativeSearchDepth = 0;
    abortSearch = false;
    numNodes = 0;
    numQNodes = 0;
    numCutoffs = 0;
    numTranspositions = 0;
    
    std::vector<chess::BBMove> rootMoves;
    try {
        rootMoves = board.getAllLegalMoves(board.getCurrentPlayer());
    } catch (const std::exception& e) {
        std::cerr << "[AI ERROR] Exception generating moves: " << e.what() << std::endl;
        return {chess::BBMove(), 0};
    } catch (...) {
        std::cerr << "[AI ERROR] Unknown exception generating moves" << std::endl;
        return {chess::BBMove(), 0};
    }
    
    if (rootMoves.empty()) {
        chess::BBMove terminalMove;
        terminalMove.value = 1; // Non-zero signals valid terminal state
        int eval = 0;
        try {
            if (board.isCheckMate(board.getCurrentPlayer())) {
                eval = -IMMEDIATE_MATE_SCORE;
            }
        } catch (...) {}
        return {terminalMove, eval};
    }
    
    if (useIterativeDeepening) {
        for (int searchDepth = 1; searchDepth <= depth; ++searchDepth) {
            try {
                searchMoves(board, *transpositionTable, searchDepth, 0, NEGATIVE_INFINITY, POSITIVE_INFINITY);
            } catch (const std::exception& e) {
                std::cerr << "[AI ERROR] Exception at depth " << searchDepth << ": " << e.what() << std::endl;
                break;
            } catch (...) {
                std::cerr << "[AI ERROR] Unknown exception at depth " << searchDepth << std::endl;
                break;
            }
            
            if (abortSearch) break;
            
            currentIterativeSearchDepth = searchDepth;
            bestMove = bestMoveThisIteration;
            bestEval = bestEvalThisIteration;
            
            if (isMateScore(bestEval)) break;
        }
    } else {
        searchMoves(board, *transpositionTable, depth, 0, NEGATIVE_INFINITY, POSITIVE_INFINITY);
        bestMove = bestMoveThisIteration;
        bestEval = bestEvalThisIteration;
    }
    
    return {bestMove, bestEval};
}

std::pair<chess::BBMove, int> AI_BB::getSearchResultParallel(BoardBB& board, int depth) {
    std::vector<chess::BBMove> rootMoves;
    try {
        rootMoves = board.getAllLegalMoves(board.getCurrentPlayer());
    } catch (const std::exception& e) {
        std::cerr << "[AI PARALLEL ERROR] Exception generating moves: " << e.what() << std::endl;
        return {chess::BBMove(), 0};
    }
    
    if (rootMoves.empty()) {
        chess::BBMove terminalMove;
        terminalMove.value = 1;
        int eval = 0;
        try {
            if (board.isCheckMate(board.getCurrentPlayer())) {
                eval = -IMMEDIATE_MATE_SCORE;
            }
        } catch (...) {}
        return {terminalMove, eval};
    }
    
    if (!threadPool || threadCount <= 1 || rootMoves.size() == 1) {
        return getSearchResult(board, depth);
    }
    
    // Parallel search: evaluate each root move on separate thread
    // Each thread searches ONE root move at the target depth only (no iterative deepening per thread)
    std::string boardFEN = board.getCurrentFEN();
    std::vector<std::future<std::pair<chess::BBMove, int>>> futures;
    futures.reserve(rootMoves.size());
    
    for (const auto& move : rootMoves) {
        // Launch parallel search for this root move
        futures.emplace_back(threadPool->enqueue([boardFEN, move, depth]() -> std::pair<chess::BBMove, int> {
            try {
                BoardBB localBoard(100, 100, 30.0f);
                localBoard.loadFEN(boardFEN, nullptr);
                
                chess::UndoState undo = localBoard.executeMove(move, true);
                
                AI_BB localAI(1);
                
                std::unique_ptr<TranspositionTable> localTT;
                try {
                    localTT = std::make_unique<TranspositionTable>(localBoard, 16);
                } catch (...) {
                    localBoard.undoMove(move, undo);
                    return {move, NEGATIVE_INFINITY};
                }
                
                // Use iterative deepening for better TT utilization and move ordering
                // After making the root move, we're at ply 1, so all searches start from ply 1
                int eval = 0;
                for (int searchDepth = 1; searchDepth < depth; ++searchDepth) {
                    eval = -localAI.searchMoves(localBoard, *localTT, searchDepth, 1, NEGATIVE_INFINITY, POSITIVE_INFINITY);
                    if (localAI.abortSearch) break;
                }
                
                localBoard.undoMove(move, undo);
                
                return {move, eval};
            } catch (const std::exception& e) {
                std::cerr << "[AI PARALLEL ERROR] Exception: " << e.what() << std::endl;
                return {move, NEGATIVE_INFINITY};
            }
        }));
    }
    
    chess::BBMove bestRootMove;
    int bestRootEval = NEGATIVE_INFINITY;
    
    for (auto& future : futures) {
        try {
            auto [move, eval] = future.get();
            if (eval > bestRootEval) {
                bestRootEval = eval;
                bestRootMove = move;
            }
        } catch (const std::exception& e) {
            std::cerr << "[AI PARALLEL ERROR] Future exception: " << e.what() << std::endl;
        }
    }
    
    std::cout << "[AI PARALLEL] Search complete. Best eval: " << bestRootEval << std::endl;
    return {bestRootMove, bestRootEval};
}

int AI_BB::searchMoves(BoardBB& board, TranspositionTable& tt, int depth, int plyFromRoot, int alpha, int beta) {
    numNodes++;
    
    if (abortSearch) {
        return 0;
    }
    
    if (plyFromRoot > 0 && !board.bbState->repetitionHistory.empty()) {
        if (std::find(board.bbState->repetitionHistory.begin(),
                     board.bbState->repetitionHistory.end(),
                     board.bbState->zobristKey) != board.bbState->repetitionHistory.end()) {
            return 0;
        }
    }
    
    if (plyFromRoot > 0) {
        alpha = std::max(alpha, -IMMEDIATE_MATE_SCORE + plyFromRoot);
        beta = std::min(beta, IMMEDIATE_MATE_SCORE - plyFromRoot);
        if (alpha >= beta) {
            return alpha;
        }
    }
    
    int ttVal = tt.probeEval(depth, plyFromRoot, alpha, beta);
    if (ttVal != tt.LOOKUP_FAILED) {
        numTranspositions++;
        if (plyFromRoot == 0) {
            bestMoveThisIteration = tt.getStoredMove();
            bestEvalThisIteration = tt.getStoredValue();
        }
        return ttVal;
    }
    
    if (depth == 0) {
        return quiescenceSearch(board, tt, alpha, beta, 0);
    }
    
    std::vector<chess::BBMove> moves;
    try {
        moves = board.getAllLegalMoves(board.getCurrentPlayer());
    } catch (const std::exception& e) {
        std::cerr << "[SEARCH ERROR] Exception in getAllLegalMoves: " << e.what() << std::endl;
        return 0;
    } catch (...) {
        std::cerr << "[SEARCH ERROR] Unknown exception in getAllLegalMoves" << std::endl;
        return 0;
    }
    
    orderMoves(board, tt, moves);
    
    // Checkmate and stalemate detection
    if (moves.empty()) {
        try {
            if (board.isCheckMate(board.getCurrentPlayer())) {
                return -(IMMEDIATE_MATE_SCORE - plyFromRoot);
            }
        } catch (...) {}
        return 0;
    }
    
    int evalType = tt.UPPER_BOUND;
    chess::BBMove bestMoveInThisPosition;
    
    for (size_t i = 0; i < moves.size(); ++i) {
        chess::UndoState undo = board.executeMove(moves[i], true);
        int eval = -searchMoves(board, tt, depth - 1, plyFromRoot + 1, -beta, -alpha);
        board.undoMove(moves[i], undo);
        
        if (eval >= beta) {
            tt.storeEval(depth, plyFromRoot, beta, tt.LOWER_BOUND, moves[i]);
            numCutoffs++;
            return beta;
        }
        
        if (eval > alpha) {
            evalType = tt.EXACT;
            bestMoveInThisPosition = moves[i];
            alpha = eval;
            
            if (plyFromRoot == 0) {
                bestMoveThisIteration = moves[i];
                bestEvalThisIteration = eval;
            }
        }
    }
    
    tt.storeEval(depth, plyFromRoot, alpha, evalType, bestMoveInThisPosition);
    return alpha;
}

int AI_BB::quiescenceSearch(BoardBB& board, TranspositionTable& tt, int alpha, int beta, int depth) {
    constexpr int MAX_QUIESCENCE_DEPTH = 10;
    if (depth >= MAX_QUIESCENCE_DEPTH) {
        return evaluate(board);
    }
    
    int eval = evaluate(board);
    if (eval >= beta) {
        return beta;
    }
    if (eval > alpha) {
        alpha = eval;
    }
    
    numQNodes++;
    
    // Only search capture moves to resolve tactical sequences
    std::vector<chess::BBMove> moves = board.bbGenerator->generateMoves(*board.bbState, true);
    orderMoves(board, tt, moves);
    
    for (size_t i = 0; i < moves.size(); i++) {
        chess::UndoState undo = board.executeMove(moves[i], true);
        eval = -quiescenceSearch(board, tt, -beta, -alpha, depth + 1);
        board.undoMove(moves[i], undo);
        
        if (eval >= beta) {
            numCutoffs++;
            return beta;
        }
        if (eval > alpha) {
            alpha = eval;
        }
    }
    
    return alpha;
}

void AI_BB::orderMoves(BoardBB& board, TranspositionTable& tt, std::vector<chess::BBMove>& moves) {
    if (!useMoveOrdering || moves.empty()) return;

    chess::BBMove hashMove = useTranspositionTable ? tt.getStoredMove() : chess::BBMove();
    std::vector<int> scores;
    scores.reserve(moves.size());
    
    uint64_t opponentPawnAttackMap = 0;
    int opponentColourIndex = board.bbState->whiteToMove ? 1 : 0;
    for (int i = 0; i < board.bbState->pawns[opponentColourIndex].count(); ++i) {
        int psq = board.bbState->pawns[opponentColourIndex].squares[i];
        opponentPawnAttackMap |= chess::PrecomputedData::pawnAttackBitboards[psq][opponentColourIndex];
    }
    
    for (const auto& move : moves) {
        int score = 0;
        int movingPieceType = board.bbState->getPieceTypeAt(chess::toRow(move.startSquare()), chess::toCol(move.startSquare()));
        int targetPieceType = board.bbState->getPieceTypeAt(chess::toRow(move.targetSquare()), chess::toCol(move.targetSquare()));
        
        // Captures: MVV-LVA
        if (targetPieceType != chess::PIECE_NONE) {
            score += CAPTURED_PIECE_VALUE_MULTIPLIER * getPieceValue(targetPieceType) - getPieceValue(movingPieceType);
        }
        
        if (movingPieceType == chess::PIECE_PAWN) {
            switch (move.flag()) {
                case chess::BBMove::PromoteToQueen:  score += QUEEN_VALUE; break;
                case chess::BBMove::PromoteToRook:   score += ROOK_VALUE; break;
                case chess::BBMove::PromoteToBishop: score += BISHOP_VALUE; break;
                case chess::BBMove::PromoteToKnight: score += KNIGHT_VALUE; break;
                default: break;
            }
        } else {
            if (board.bbState->containSquare(opponentPawnAttackMap, move.targetSquare())) {
                score -= SQUARE_CONTROLLED_BY_OPPONENT_PAWN_PENALTY;
            }
        }
        
        // Hash move priority
        if (move.value == hashMove.value) {
            score += 10000;
        }
        
        scores.push_back(score);
    }
    
    // Sort moves by score
    for (size_t i = 0; i < moves.size() - 1; ++i) {
        for (size_t j = i + 1; j > 0; --j) {
            size_t swapIndex = j - 1;
            if (scores[swapIndex] < scores[j]) {
                std::swap(moves[j], moves[swapIndex]);
                std::swap(scores[j], scores[swapIndex]);
            }
        }
    }
}

bool AI_BB::isMateScore(int score) const {
    const int maxMateDepth = 1000;
    return std::abs(score) > IMMEDIATE_MATE_SCORE - maxMateDepth;
}

void AI_BB::endSearch() {
    abortSearch = true;
}

void AI_BB::setThreadCount(unsigned int numThreads) {
    if (numThreads == threadCount) return;
    
    if (threadPool) {
        threadPool->shutdown();
        threadPool.reset();
    }
    
    threadCount = (numThreads == 0) ? std::thread::hardware_concurrency() : numThreads;
    if (threadCount == 0) threadCount = 1;
    
    if (threadCount > 1) {
        try {
            threadPool = std::make_unique<ThreadPool>(threadCount);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create thread pool: " << e.what() << std::endl;
            threadPool = nullptr;
            threadCount = 1;
        }
    }
}
