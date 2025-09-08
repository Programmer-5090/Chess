#ifndef ROOK_H
#define ROOK_H

#include "piece.h"

class Rook : public Piece {
public:
    Rook(Color color, PieceType type, SDL_Renderer* renderer);

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

    bool getIsCastlingEligible() const;
    void setIsCastlingEligible(bool eligible);

private:
    bool castlingEligible;
};

#endif // ROOK_H
