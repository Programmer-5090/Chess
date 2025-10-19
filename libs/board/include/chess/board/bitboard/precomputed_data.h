#ifndef PRECOMPUTED_DATA_H
#define PRECOMPUTED_DATA_H

#include <cstdint>
#include <array>

namespace chess {

class PrecomputedData {
public:
    static void init();
    
    static constexpr int directionOffsets[8] = {8, -8, -1, 1, 7, -7, 9, -9};
    static int numSquaresToEdge[64][8];
    
    static uint64_t knightAttackBitboards[64];
    static uint64_t kingAttackBitboards[64];
    static uint64_t pawnAttackBitboards[64][2];
    
    static int* knightMoves[64];
    static int numKnightMoves[64];
    static int* kingMoves[64];
    static int numKingMoves[64];
    
    static int pawnAttackDirections[2][2];
    static int directionLookup[127];
    
    static constexpr int NORTH = 0;
    static constexpr int SOUTH = 1;
    static constexpr int WEST = 2;
    static constexpr int EAST = 3;
    static constexpr int NORTH_WEST = 4;
    static constexpr int SOUTH_EAST = 5;
    static constexpr int NORTH_EAST = 6;
    static constexpr int SOUTH_WEST = 7;
    
    static constexpr int knightOffsets[8] = {15, 17, -17, -15, 10, -6, 6, -10};
    static constexpr int kingOffsets[8] = {8, -8, 1, -1, 7, -7, 9, -9};
    
private:
    static bool initialized;
};

} // namespace chess

#endif // PRECOMPUTED_DATA_H

