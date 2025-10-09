#ifndef QUEEN_H
#define QUEEN_H

#include <chess/board/pieces/piece.h>

class Queen : public Piece {
public:
    Queen(Color color, PieceType type, SDL_Renderer* renderer);

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

    int getValue() const override { return 6; }
};

#endif // QUEEN_H
