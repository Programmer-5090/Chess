#ifndef PIECE_CONST_H
#define PIECE_CONST_H

#include <cstdint>

namespace chess {

    constexpr int PIECE_NONE  = 0;
    constexpr int PIECE_KING  = 1;
    constexpr int PIECE_PAWN  = 2;
    constexpr int PIECE_KNIGHT= 3;
    constexpr int PIECE_BISHOP= 5;
    constexpr int PIECE_ROOK  = 6;
    constexpr int PIECE_QUEEN = 7;

    constexpr int COLOR_WHITE = 8;   // 0b01000
    constexpr int COLOR_BLACK = 16;  // 0b10000

    constexpr int TYPE_MASK   = 0b00111;
    constexpr int COLOR_MASK  = 0b11000;

    inline bool isColor(int piece, int color)
    { return (piece & COLOR_MASK) == color; }

    inline int  colorOf(int piece)
    { return piece & COLOR_MASK; }

    inline int  typeOf(int piece)
     { return piece & TYPE_MASK; }
    
    inline bool isRookOrQueen(int pieceType)
     { return (pieceType & 0b110) == 0b110; }

    inline bool isBishopOrQueen(int pieceType)
     { return (pieceType & 0b101) == 0b101; }

    inline bool isSlidingPiece(int pieceType)
     { return (pieceType & 0b100) != 0; }

    inline bool bbHas(uint64_t bb, int sq) { return (bb & (1ULL << sq)) != 0ULL; }
} // namespace chess
#endif // PIECE_CONST_H