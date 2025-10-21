#ifndef ENUMS_H
#define ENUMS_H

enum Color {
    WHITE = 0,
    BLACK = 1,
    NO_COLOR = 2
};

enum PieceType {
    PAWN = 0,
    ROOK = 1,
    KNIGHT = 2,
    BISHOP = 3,
    QUEEN = 4,
    KING = 5,
    NONE = 6
};

enum class CastlingType {
    NONE,
    KING_SIDE,
    QUEEN_SIDE
};

#endif // ENUMS_H
