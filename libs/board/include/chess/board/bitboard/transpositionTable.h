#ifndef TRANS_POSITION_TABLE_H
#define TRANS_POSITION_TABLE_H

#include <cstdint>
#include <chess/board/move_executor.h>


// Forward declaration
class Board;
using byte = unsigned char;
struct Move;

struct Entry {
    // Entry details
    uint64_t key;
    int value;
    int depth;
    byte nodeType;
    Move move;

    Entry() : key(0), value(0), depth(0), nodeType(0), move() {}

    Entry(uint64_t k, int v, int d, byte nt, Move m)
        : key(k), value(v), depth(d), nodeType(nt), move(m) {}

    int getSize() const{
        return sizeof(Entry);
    }
}; 

class TranspositionTable{
    public:
        TranspositionTable(Board &b, size_t sizeInMB = 64);
        ~TranspositionTable();

        uint64_t getIndex() const;

        void storeEval(int depth, int plySearched, int eval, int evalType, const Move &move);

        Move getStoredMove() const {
            return table[getIndex()].move;
        }

        int probeEval(int depth, int plyFromRoot, int alpha, int beta);

        int correctMateScoreForStorage(int score, int numPlySearched) const;

        int correctMateScoreForRetrieval(int score, int numPlyFromRoot) const;

        private:
            std::vector<Entry> table;
            size_t tableSize;
            Board &board;
            const int UpperBound = 2;
            const int LowerBound = 1;
            const int Exact = 0;
            const int lookupFailed = -1;
            bool isEnabled = true;
};
#endif // TRANS_POSITION_TABLE_H