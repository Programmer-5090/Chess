#ifndef KNIGHT_H
#define KNIGHT_H

#include <chess/board/pieces/piece.h>

class Knight : public Piece {
public:
    Knight(Color color, PieceType type, SDL_Renderer* renderer);
    
    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

    int getValue() const override { return 3; }
};

#endif // KNIGHT_H
