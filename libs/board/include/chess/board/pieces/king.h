#ifndef KING_H
#define KING_H

#include <chess/board/pieces/piece.h>

class King : public Piece {
public:
    King(Color color, PieceType type, SDL_Renderer* renderer);

    std::unique_ptr<Piece> clone() const override;

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

    bool getIsCastlingEligible() const;
    void setIsCastlingEligible(bool eligible);
    bool getIsKingInCheck(const Board& board) const;

private:
    bool castlingEligible;
};

#endif // KING_H
