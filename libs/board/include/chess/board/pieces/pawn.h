#ifndef PAWN_H
#define PAWN_H

#include <chess/board/pieces/piece.h>

class Pawn : public Piece {
public:
    Pawn(Color color, PieceType type, SDL_Renderer* renderer);

    std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const override;

    bool getEnPassantCaptureEligible() const;
    
    void setEnPassantCaptureEligible(bool eligible);

    int getValue() const override { return 1; }
    

private:
    bool enPassantCaptureEligible;
    void addPromotionMoves(std::vector<Move>& moves, int fromRow, int fromCol, int toRow, int toCol, Piece* target) const;
};

bool is_back_rank(int row, Color color);

#endif // PAWN_H
