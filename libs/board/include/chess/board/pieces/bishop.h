#ifndef BISHOP_H
#define BISHOP_H

#include <chess/board/pieces/piece.h>

class Bishop : public Piece {
public:
    Bishop(Color color, PieceType type, SDL_Renderer* renderer);

    std::unique_ptr<Piece> clone() const override;

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

    int getValue() const override { return 4; }
};

#endif // BISHOP_H
