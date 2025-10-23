#ifndef AI_BB_H
#define AI_BB_H

#include <chess/enums.h>
#include <chess/board/bitboard/move.h>
#include <chess/board/bitboard/board_state.h>
#include <chess/utils/thread_pool.h>
#include <vector>
#include <memory>
#include <array>
#include <future>

// Forward declarations
class BoardBB;
class TranspositionTable;

// Piece values - use constexpr to ensure compile-time constants
constexpr int PAWN_VALUE = 100;
constexpr int KNIGHT_VALUE = 300;
constexpr int BISHOP_VALUE = 320;
constexpr int ROOK_VALUE = 500;
constexpr int QUEEN_VALUE = 900;

// Search constants
constexpr int IMMEDIATE_MATE_SCORE = 100000;
constexpr int POSITIVE_INFINITY = 9999999;
constexpr int NEGATIVE_INFINITY = -POSITIVE_INFINITY;

// Move ordering constants
constexpr int SQUARE_CONTROLLED_BY_OPPONENT_PAWN_PENALTY = 350;
constexpr int CAPTURED_PIECE_VALUE_MULTIPLIER = 10;

struct Settings {
    bool useIterativeDeepening = true;
    bool useTranspositionTable = true;
    bool useMoveOrdering = true;
    bool exitSearch = false;
};

class AI_BB {
public:
    AI_BB(unsigned int numThreads = 0);
    ~AI_BB();

    std::pair<chess::BBMove, int> getSearchResult(BoardBB& board, int depth);
    std::pair<chess::BBMove, int> getSearchResultParallel(BoardBB& board, int depth);
    void updateSettings(const Settings& newSettings);
    void endSearch();
    void setThreadCount(unsigned int numThreads);
    
    chess::BBMove getBestMove() const { return bestMove; }
    int getBestEval() const { return bestEval; }
    int getNumNodes() const { return numNodes; }
    int getNumQNodes() const { return numQNodes; }
    int getNumCutoffs() const { return numCutoffs; }
    int getNumTranspositions() const { return numTranspositions; }

private:
    // Evaluation functions
    int evaluate(BoardBB& board);
    int countMaterial(BoardBB& board, int colorIdx);
    float endgamePhaseWeight(int materialCountWithoutPawns) const;
    int mopUpEval(BoardBB& board, int friendlyIdx, int opponentIdx, int myMaterial, int opponentMaterial, float endgameWeight);
    int evaluatePieceSquareTables(BoardBB& board, int colorIdx, float endgamePhaseWeight);
    int evaluatePieceSquareTable(const std::array<short, 64>& table, const chess::PieceList& pieceList, bool isWhite);
    int getPieceValue(int pieceType) const;
    
    // Search functions
    int searchMoves(BoardBB& board, TranspositionTable& tt, int depth, int plyFromRoot, int alpha, int beta);
    int quiescenceSearch(BoardBB& board, TranspositionTable& tt, int alpha, int beta, int depth = 0);
    void orderMoves(BoardBB& board, TranspositionTable& tt, std::vector<chess::BBMove>& moves);  // With TT
    bool isMateScore(int score) const;
    
    // Best moves tracking
    chess::BBMove bestMove;
    int bestEval = 0;
    chess::BBMove bestMoveThisIteration;
    int bestEvalThisIteration = 0;
    int currentIterativeSearchDepth = 0;
    
    // Settings
    bool useIterativeDeepening = true;
    bool useTranspositionTable = true;
    bool useMoveOrdering = true;
    bool abortSearch = false;
    
    // Performance tracking
    int numNodes = 0;
    int numQNodes = 0;
    int numCutoffs = 0;
    int numTranspositions = 0;
    
    // Threading
    std::unique_ptr<ThreadPool> threadPool;
    unsigned int threadCount = 0;
};

#endif // AI_BB_H