#pragma once

#include "piece.h" // For Piece base class and Move struct

class Pawn : public Piece{  
public:
    Pawn(Color color, PieceType type, SDL_Renderer* renderer);

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

    bool getEnPassantCaptureEligible() const;
    void setEnPassantCaptureEligible(bool eligible);

    private:
        bool enPassantCaptureEligible;
};

// helper:
bool is_back_rank(int row, Color color);