#ifndef TRANS_POSITION_TABLE_H
#define TRANS_POSITION_TABLE_H

#include <cstdint>
#include <vector>
#include <chess/board/bitboard/move.h>

// Forward declarations
class BoardBB;
namespace chess {
    struct BitboardState;
}

using byte = unsigned char;

struct TTEntry {
    uint64_t key;
    int value;
    int depth;
    byte nodeType;
    chess::BBMove move;

    TTEntry() : key(0), value(0), depth(0), nodeType(0), move() {}

    TTEntry(uint64_t k, int v, int d, byte nt, chess::BBMove m)
        : key(k), value(v), depth(d), nodeType(nt), move(m) {}

    int getSize() const {
        return sizeof(TTEntry);
    }
}; 

class TranspositionTable {
public:
    static constexpr int EXACT = 0;
    static constexpr int LOWER_BOUND = 1;
    static constexpr int UPPER_BOUND = 2;
    static constexpr int LOOKUP_FAILED = -1;
    
    static constexpr int MATE_SCORE = 100000;
    static constexpr int MAX_MATE_DEPTH = 1000;

    TranspositionTable(BoardBB& b, size_t sizeInMB = 64);
    ~TranspositionTable();

    friend class AI_BB;

    uint64_t getIndex() const;

    void storeEval(int depth, int plySearched, int eval, int evalType, const chess::BBMove& move);

    chess::BBMove getStoredMove() const;

    int getStoredValue() const;

    int probeEval(int depth, int plyFromRoot, int alpha, int beta);

    int correctMateScoreForStorage(int score, int numPlySearched) const;
    int correctMateScoreForRetrieval(int score, int numPlyFromRoot) const;

    void clear();

    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() const { return isEnabled; }

    size_t getSize() const { return tableSize; }
    size_t getNumEntries() const { return numEntries; }

private:
    std::vector<TTEntry> table;
    size_t tableSize;
    size_t numEntries;
    BoardBB& board;
    bool isEnabled;
};

#endif // TRANS_POSITION_TABLE_H