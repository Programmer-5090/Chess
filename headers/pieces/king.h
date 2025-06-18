#pragma once 

#include "piece.h" // For Piece base class and Move struct

class King: public Piece{
public:
    King(Color color, PieceType type, SDL_Renderer* renderer);

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

    bool getIsCastlingEligible() const;

    void setIsCastlingEligible(bool eligible);

    bool getIsKingInCheck(const Board& board) const;

private:
    bool castlingEligible;
};
