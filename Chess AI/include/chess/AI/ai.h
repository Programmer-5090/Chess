#ifndef AI_H
#define AI_H

#include <chess/enums.h>
#include <vector>
#include <atomic>
#include <chrono>

// Forward declaration
class Board;
class ThreadPool;
struct Move;

class AI {
public:
    AI(Board& b);
    virtual ~AI();

    Move getBestMove(int depth = 4);
    int search(int depth, int alpha, int beta);
    
    // Performance tracking methods
    std::uint64_t getNodesSearched() const { return nodesSearched.load(); }
    void resetNodesSearched() { nodesSearched.store(0); }
    
    // Get last search timing info
    double getLastSearchTimeMs() const { return lastSearchTimeMs; }
    std::uint64_t getLastSearchNodes() const { return lastSearchNodes; }
    
    // Print performance statistics to terminal
    void printPerformanceStats() const;

private:
    int evaluatePosition(Board& b);
    int countPieces(Board& b, Color color);
    int search_recursive(Board& b, int depth, bool enableBulkCount, int alpha, int beta);
    void orderMoves(Board& b, std::vector<Move>& moves);
    ThreadPool* threadPool = nullptr; // Thread pool for parallel processing
    Board& board; // Reference to the chess board
    
    // Performance tracking
    mutable std::atomic<std::uint64_t> nodesSearched{0};
    double lastSearchTimeMs = 0.0;
    std::uint64_t lastSearchNodes = 0;
};

#endif // AI_H