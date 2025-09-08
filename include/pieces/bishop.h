#ifndef BISHOP_H
#define BISHOP_H

#include "piece.h"

class Bishop : public Piece {
public:
    Bishop(Color color, PieceType type, SDL_Renderer* renderer);

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;
};

#endif // BISHOP_H
