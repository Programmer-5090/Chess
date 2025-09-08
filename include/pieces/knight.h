#ifndef KNIGHT_H
#define KNIGHT_H

#include "piece.h"

class Knight : public Piece {
public:
    Knight(Color color, PieceType type, SDL_Renderer* renderer);
    
    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;
};

#endif // KNIGHT_H
