#ifndef QUEEN_H
#define QUEEN_H

#include "piece.h"

class Queen : public Piece {
public:
    Queen(Color color, PieceType type, SDL_Renderer* renderer);

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;
};

#endif // QUEEN_H
