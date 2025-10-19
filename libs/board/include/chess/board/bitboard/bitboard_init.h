#ifndef BITBOARD_INIT_H
#define BITBOARD_INIT_H

#include <chess/board/bitboard/zoborist.h>
#include <chess/board/bitboard/precomputed_data.h>

namespace chess {

inline void initBitboardSystem() {
    Zobrist::init();
    PrecomputedData::init();
}

} // namespace chess

#endif // BITBOARD_INIT_H
