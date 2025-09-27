#include "pieces/king.h"
#include "board.h" // Include board.h for Board definition
#include "enums.h"   // Include enums.h for Color, PieceType
#include "pieces/rook.h"

King::King(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    castlingEligible = true;

    if(color == BLACK){
        pieceImg = IMG_Load("images/B_King.png");
    }
    else{
        pieceImg = IMG_Load("images/W_King.png");
    }

    pieceText = SDL_CreateTextureFromSurface(renderer, pieceImg);
}

std::vector<Move> King::getPseudoLegalMoves(const Board& board, bool generateCastlingMoves) const {
    int row = position.first, col = position.second;
    std::vector<Move> moves;

    // Standard king moves (one step in all directions) - UNCHANGED
    static constexpr std::pair<int,int> dirs[] = {
        {+1,  0}, {-1,  0}, { 0, +1}, { 0, -1},
        {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1}
    };

    for (auto [dr, dc] : dirs){
        int r = row + dr;
        int c = col + dc;

        if (in_bounds(r, c)){
            const Piece* targetPiece = board.getPieceAt(r, c);
            
            if (!targetPiece){
                moves.push_back(Move({row, col}, {r, c}, this, nullptr));
            } else {
                if (targetPiece->getColor() != this->color) {
                    moves.push_back(Move({row, col}, {r, c}, this, targetPiece));
                }
            }
        }
    }

    if (generateCastlingMoves) {
        Color oppositeColor = (this->color == BLACK) ? WHITE : BLACK;

        // King side castling
        if (this->castlingEligible && !this->hasMoved) {
            const Piece* rookCandidatePiece = board.getPieceAt(row, 7);
            if (rookCandidatePiece && rookCandidatePiece->getType() == ROOK) {
                const Rook* kingSideRook = static_cast<const Rook*>(rookCandidatePiece);
                if (kingSideRook->getIsCastlingEligible() && !kingSideRook->getHasMoved()) {
                    
                    bool squaresBetweenAreEmpty =
                        (board.getPieceAt(row, col + 1) == nullptr &&
                         board.getPieceAt(row, col + 2) == nullptr);

                    if (squaresBetweenAreEmpty) {
                        // NEW, EFFICIENT CHECK: Use the board's optimized isSquareAttacked function.
                        if (!board.isSquareAttacked(row, col, oppositeColor) &&
                            !board.isSquareAttacked(row, col + 1, oppositeColor) &&
                            !board.isSquareAttacked(row, col + 2, oppositeColor)) {
                            moves.push_back(Move({row, col}, {row, col + 2}, this, nullptr, true, true));
                        }
                    }
                }
            }
        }
        
        // Queen side castling
        if (this->castlingEligible && !this->hasMoved) {
            const Piece* rookCandidatePiece = board.getPieceAt(row, 0);
            if (rookCandidatePiece && rookCandidatePiece->getType() == ROOK) {
                const Rook* queenSideRook = static_cast<const Rook*>(rookCandidatePiece);
                if (queenSideRook->getIsCastlingEligible() && !queenSideRook->getHasMoved()) {
                    
                    bool squaresBetweenAreEmpty =
                        (board.getPieceAt(row, col - 1) == nullptr &&
                         board.getPieceAt(row, col - 2) == nullptr &&
                         board.getPieceAt(row, col - 3) == nullptr);

                    if (squaresBetweenAreEmpty) {
                        // NEW, EFFICIENT CHECK: Use the board's optimized isSquareAttacked function.
                        if (!board.isSquareAttacked(row, col, oppositeColor) &&
                            !board.isSquareAttacked(row, col - 1, oppositeColor) &&
                            !board.isSquareAttacked(row, col - 2, oppositeColor)) {
                            moves.push_back(Move({row, col}, {row, col - 2}, this, nullptr, true, false, true));
                        }
                    }
                }
            }
        }
    }
    return moves;
}

bool King::getIsCastlingEligible() const{
    return castlingEligible;
}

void King::setIsCastlingEligible(bool eligible){
    castlingEligible = eligible;
}

bool King::getIsKingInCheck(const Board& board) const{
    int row = position.first, col = position.second;
    
    Color oppositeColor = (this->color == BLACK) ? WHITE : BLACK;

    std::vector<Move> opponentMoves = board.getAllLegalMoves(oppositeColor, false);

    for (const auto& oppMove : opponentMoves) {
        if (oppMove.endPos.first == row && oppMove.endPos.second == col) {
            return true;
        }
    }
    return false;
}


