#include <chess/board/bitboard/precomputed_data.h>
#include <chess/board/bitboard/board_state.h>
#include <vector>
#include <cmath>

namespace chess {

int PrecomputedData::numSquaresToEdge[64][8] = {};
uint64_t PrecomputedData::knightAttackBitboards[64] = {};
uint64_t PrecomputedData::kingAttackBitboards[64] = {};
uint64_t PrecomputedData::pawnAttackBitboards[64][2] = {};
int* PrecomputedData::knightMoves[64] = {};
int PrecomputedData::numKnightMoves[64] = {};
int* PrecomputedData::kingMoves[64] = {};
int PrecomputedData::numKingMoves[64] = {};
int PrecomputedData::pawnAttackDirections[2][2] = {{4, 6}, {7, 5}};
int PrecomputedData::directionLookup[127] = {};
bool PrecomputedData::initialized = false;

void PrecomputedData::init() {
    if (initialized) return;
    
    for (int file = 0; file < 8; ++file) {
        for (int rank = 0; rank < 8; ++rank) {
            int square = rank * 8 + file;
            
            int north = 7 - rank;
            int south = rank;
            int west = file;
            int east = 7 - file;
            
            numSquaresToEdge[square][NORTH] = north;
            numSquaresToEdge[square][SOUTH] = south;
            numSquaresToEdge[square][WEST] = west;
            numSquaresToEdge[square][EAST] = east;
            numSquaresToEdge[square][NORTH_WEST] = std::min(north, west);
            numSquaresToEdge[square][SOUTH_EAST] = std::min(south, east);
            numSquaresToEdge[square][NORTH_EAST] = std::min(north, east);
            numSquaresToEdge[square][SOUTH_WEST] = std::min(south, west);
        }
    }
    
    for (int square = 0; square < 64; ++square) {
        uint64_t attacks = 0ULL;
        std::vector<int> legalMoves;
        int file = square % 8;
        int rank = square / 8;
        
        int possibleMoves[8][2] = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };
        
        for (auto& move : possibleMoves) {
            int newRank = rank + move[0];
            int newFile = file + move[1];
            
            if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
                int targetSquare = newRank * 8 + newFile;
                attacks |= (1ULL << targetSquare);
                legalMoves.push_back(targetSquare);
            }
        }
        
        knightAttackBitboards[square] = attacks;
        numKnightMoves[square] = legalMoves.size();
        knightMoves[square] = new int[legalMoves.size()];
        for (size_t i = 0; i < legalMoves.size(); ++i) {
            knightMoves[square][i] = legalMoves[i];
        }
    }
    
    for (int square = 0; square < 64; ++square) {
        uint64_t attacks = 0ULL;
        std::vector<int> legalMoves;
        int file = square % 8;
        int rank = square / 8;
        
        for (int dRank = -1; dRank <= 1; ++dRank) {
            for (int dFile = -1; dFile <= 1; ++dFile) {
                if (dRank == 0 && dFile == 0) continue;
                
                int newRank = rank + dRank;
                int newFile = file + dFile;
                
                if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
                    int targetSquare = newRank * 8 + newFile;
                    attacks |= (1ULL << targetSquare);
                    legalMoves.push_back(targetSquare);
                }
            }
        }
        
        kingAttackBitboards[square] = attacks;
        numKingMoves[square] = legalMoves.size();
        kingMoves[square] = new int[legalMoves.size()];
        for (size_t i = 0; i < legalMoves.size(); ++i) {
            kingMoves[square][i] = legalMoves[i];
        }
    }
    
    for (int square = 0; square < 64; ++square) {
        int file = square % 8;
        int rank = square / 8;
        
        uint64_t whiteAttacks = 0ULL;
        if (rank < 7) {
            if (file > 0) whiteAttacks |= (1ULL << (square + 7));
            if (file < 7) whiteAttacks |= (1ULL << (square + 9));
        }
        pawnAttackBitboards[square][0] = whiteAttacks;
        
        uint64_t blackAttacks = 0ULL;
        if (rank > 0) {
            if (file > 0) blackAttacks |= (1ULL << (square - 9));
            if (file < 7) blackAttacks |= (1ULL << (square - 7));
        }
        pawnAttackBitboards[square][1] = blackAttacks;
    }
    
    // Maps square offset to direction for isMovingAlongRay() checks
    for (int i = 0; i < 127; i++) {
        int offset = i - 63;
        int absOffset = std::abs(offset);
        int absDir = 1;
        if (absOffset % 9 == 0) {
            absDir = 9;
        } else if (absOffset % 8 == 0) {
            absDir = 8;
        } else if (absOffset % 7 == 0) {
            absDir = 7;
        }
        
        directionLookup[i] = absDir * (offset > 0 ? 1 : (offset < 0 ? -1 : 0));
    }
    
    initialized = true;
}

} // namespace chess
