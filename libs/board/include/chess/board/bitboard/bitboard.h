#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint>
#include <bit>

namespace chess {

inline int popCount(uint64_t bb) {
    return std::popcount(bb);
}

inline int getLSB(uint64_t bb) {
    return std::countr_zero(bb);
}

inline int popLSB(uint64_t& bb) {
    int lsb = getLSB(bb);
    bb &= bb - 1;
    return lsb;
}

inline bool getBit(uint64_t bb, int square) {
    return (bb & (1ULL << square)) != 0;
}

inline void setBit(uint64_t& bb, int square) {
    bb |= (1ULL << square);
}

inline void clearBit(uint64_t& bb, int square) {
    bb &= ~(1ULL << square);
}

inline void toggleBit(uint64_t& bb, int square) {
    bb ^= (1ULL << square);
}

inline uint64_t bit(int square) {
    return 1ULL << square;
}

inline bool any(uint64_t bb) {
    return bb != 0;
}

inline bool none(uint64_t bb) {
    return bb == 0;
}

} // namespace chess

#endif // BITBOARD_H
