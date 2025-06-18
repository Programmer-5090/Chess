#pragma once

#include "piece.h" // For Piece base class and Move struct


class Rook : public Piece {
public:
    Rook(Color color, PieceType type, SDL_Renderer* renderer);

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

    bool getIsCastlingEligible() const;

    void setIsCastlingEligible(bool eligible);

private:
    bool castlingEligible;
};