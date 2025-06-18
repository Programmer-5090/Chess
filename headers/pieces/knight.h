#pragma once 

#include "piece.h" // For Piece base class and Move struct

class Knight: public Piece{
public:
    Knight(Color color, PieceType type, SDL_Renderer* renderer);
    
    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

};