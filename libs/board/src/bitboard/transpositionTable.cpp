#include <chess/board/bitboard/transpositionTable.h>
#include <chess/board/boardBB.h>
#include <chess/board/bitboard/board_state.h>
#include <chess/board/bitboard/move.h>
#include <chess/utils/logger.h>
#include <algorithm>
#include <iostream>

TranspositionTable::TranspositionTable(BoardBB& b, size_t sizeInMB)
    : board(b), isEnabled(true) {
    
    size_t sizeInBytes = sizeInMB * 1024 * 1024;
    numEntries = sizeInBytes / sizeof(TTEntry);
    
    if (numEntries < 2) {
        numEntries = 2;
    }
    
    size_t powerOf2 = 1;
    while (powerOf2 < numEntries) {
        powerOf2 *= 2;
    }
    tableSize = powerOf2;
    
    if (tableSize == 0) {
        tableSize = 1024;
    }
    
    table.resize(tableSize);
    numEntries = tableSize;
    
    LOG_INFO(std::string("Transposition table initialized with ") + 
             std::to_string(tableSize) + " entries (" + 
             std::to_string((tableSize * sizeof(TTEntry)) / (1024 * 1024)) + " MB)");
}

TranspositionTable::~TranspositionTable() {
    table.clear();
}

uint64_t TranspositionTable::getIndex() const {
    uint64_t key = board.getLastState();
    
    // Use bitwise AND with (tableSize - 1) for fast modulo
    // This works because tableSize is a power of 2
    uint64_t index = key & (tableSize - 1);
    
    if (index >= table.size()) {
        std::cerr << "[TT INDEX ERROR] Index " << index << " >= table size " << table.size() << std::endl;
    }
    
    return index;
}

void TranspositionTable::storeEval(int depth, int plySearched, int eval, int evalType, const chess::BBMove& move) {
    if (!isEnabled) return;
    
    uint64_t index = getIndex();
    uint64_t zobristKey = board.getLastState();
    
    int correctedEval = correctMateScoreForStorage(eval, plySearched);
    
    TTEntry& entry = table[index];
    
    bool shouldReplace = (entry.key == 0) || 
                        (entry.key == zobristKey) || 
                        (depth >= entry.depth);
    
    if (shouldReplace) {
        entry.key = zobristKey;
        entry.value = correctedEval;
        entry.depth = depth;
        entry.nodeType = static_cast<byte>(evalType);
        entry.move = move;
        
        LOG_DEBUG(std::string("TT Store: depth=") + std::to_string(depth) + 
                  " eval=" + std::to_string(eval) + 
                  " type=" + std::to_string(evalType));
    }
}

chess::BBMove TranspositionTable::getStoredMove() const {
    if (!isEnabled) return chess::BBMove();
    
    uint64_t index = getIndex();
    
    if (index >= table.size()) {
        std::cerr << "[TT getStoredMove ERROR] Index " << index << " >= table size " << table.size() << " - RETURNING INVALID MOVE!" << std::endl;
        return chess::BBMove();
    }
    
    const TTEntry& entry = table[index];
    
    uint64_t zobristKey = board.getLastState();
    
    if (entry.key == zobristKey) {
        return entry.move;
    }
    
    return chess::BBMove();
}

int TranspositionTable::getStoredValue() const {
    if (!isEnabled) return 0;
    
    uint64_t index = getIndex();
    
    if (index >= table.size()) {
        std::cerr << "[TT getStoredValue ERROR] Index " << index << " >= table size " << table.size() << std::endl;
        return 0;
    }
    
    const TTEntry& entry = table[index];
    return entry.value;
}

int TranspositionTable::probeEval(int depth, int plyFromRoot, int alpha, int beta) {
    if (!isEnabled) return LOOKUP_FAILED;
    
    uint64_t index = getIndex();
    const TTEntry& entry = table[index];
    
    uint64_t zobristKey = board.getLastState();
    
    if (entry.key != zobristKey) {
        return LOOKUP_FAILED;
    }
    
    if (entry.depth < depth) {
        return LOOKUP_FAILED;
    }
    
    int correctedValue = correctMateScoreForRetrieval(entry.value, plyFromRoot);
    
    if (entry.nodeType == EXACT) {
        LOG_DEBUG(std::string("TT Hit (EXACT): depth=") + std::to_string(entry.depth) + 
                  " value=" + std::to_string(correctedValue));
        return correctedValue;
    }
    
    if (entry.nodeType == LOWER_BOUND && correctedValue >= beta) {
        LOG_DEBUG(std::string("TT Hit (LOWER_BOUND): depth=") + std::to_string(entry.depth) + 
                  " value=" + std::to_string(correctedValue) + " >= beta=" + std::to_string(beta));
        return correctedValue;
    }
    
    if (entry.nodeType == UPPER_BOUND && correctedValue <= alpha) {
        LOG_DEBUG(std::string("TT Hit (UPPER_BOUND): depth=") + std::to_string(entry.depth) + 
                  " value=" + std::to_string(correctedValue) + " <= alpha=" + std::to_string(alpha));
        return correctedValue;
    }
    
    return LOOKUP_FAILED;
}

int TranspositionTable::correctMateScoreForStorage(int score, int numPlySearched) const {
    if (score >= MATE_SCORE - MAX_MATE_DEPTH) {
        return score + numPlySearched;
    } else if (score <= -MATE_SCORE + MAX_MATE_DEPTH) {
        return score - numPlySearched;
    }
    return score;
}

int TranspositionTable::correctMateScoreForRetrieval(int score, int numPlyFromRoot) const {
    if (score >= MATE_SCORE - MAX_MATE_DEPTH) {
        return score - numPlyFromRoot;
    } else if (score <= -MATE_SCORE + MAX_MATE_DEPTH) {
        return score + numPlyFromRoot;
    }
    return score;
}

void TranspositionTable::clear() {
    std::fill(table.begin(), table.end(), TTEntry());
    LOG_INFO("Transposition table cleared");
}