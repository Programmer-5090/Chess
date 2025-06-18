#include "headers/pieces/king.h"
#include "headers/board.h" // Include board.h for Board definition
#include "headers/enums.h"   // Include enums.h for Color, PieceType

King::King(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    castlingEligible = true;

    if(color == BLACK){
        pieceImg = IMG_Load("/Users/jethroaiyesan/Programming/Chess C++/images/B_King.png");
    }
    else{
        pieceImg = IMG_Load("/Users/jethroaiyesan/Programming/Chess C++/images/W_King.png");
    }

    pieceText = SDL_CreateTextureFromSurface(renderer, pieceImg);
}

std::vector<Move> King::getPseudoLegalMoves(const Board& board, bool generateCastlingMoves) const {
    int row = position.first, col = position.second;
    std::vector<Move> moves;

    // Standard king moves (one step in all directions)
    static constexpr std::pair<int,int> dirs[] = {
        {+1,  0}, {-1,  0}, { 0, +1}, { 0, -1},
        {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1}
    };

    for (auto [dr, dc] : dirs){
        int r = row + dr;
        int c = col + dc;

        if (in_bounds(r, c)){ // Check bounds FIRST
            const Piece* targetPiece = board.getPieceAt(r, c);
            
            if (!targetPiece){ // Square is empty
                moves.push_back(Move(
                    std::make_pair(row, col),
                    std::make_pair(r, c),
                    this,
                    nullptr // No captured piece
                ));
            } else { // Square is occupied
                if (targetPiece->getColor() != this->color) { // Enemy piece
                    moves.push_back(Move(
                        std::make_pair(row, col),
                        std::make_pair(r, c),
                        this,
                        targetPiece // Captured piece
                    ));
                }
            }
        }
    }

    if (generateCastlingMoves) { // Only generate castling if flag is true
        // King side castling
        if (this->castlingEligible && !this->hasMoved) {
            const Piece* rookCandidatePiece = board.getPieceAt(position.first, 7);
            if (rookCandidatePiece != nullptr && rookCandidatePiece->getType() == ROOK) {
                const Rook* kingSideRook = static_cast<const Rook*>(rookCandidatePiece);
                if (kingSideRook->getIsCastlingEligible() && !kingSideRook->getHasMoved() &&
                    kingSideRook->getPosition().first == position.first &&
                    kingSideRook->getPosition().second == 7) {
                    bool squaresBetweenAreEmpty =
                        (board.getPieceAt(position.first, position.second + 1) == nullptr &&
                         board.getPieceAt(position.first, position.second + 2) == nullptr);

                    if (squaresBetweenAreEmpty) {
                        Color oppositeColor = (this->color == BLACK) ? WHITE : BLACK;
                        // Pass 'false' for generateCastlingMoves to break possible recursion
                        std::vector<Move> opponentMoves = board.getAllLegalMoves(oppositeColor, false);

                        auto isSquareAttacked = [&](int r_check, int c_check) {
                            for (const auto& oppMove : opponentMoves) {
                                if (oppMove.endPos.first == r_check && oppMove.endPos.second == c_check) {
                                    return true;
                                }
                            }
                            return false;
                        };

                        bool kingIsSafeToCastle = true;
                        if (isSquareAttacked(position.first, position.second) ||
                            isSquareAttacked(position.first, position.second + 1) ||
                            isSquareAttacked(position.first, position.second + 2)) {
                            kingIsSafeToCastle = false;
                        }

                        if (kingIsSafeToCastle) {
                            moves.push_back(Move({row, col}, {row, col + 2}, this, nullptr, true, true)); // isCastling=true, isKingSide=true
                        }
                    }
                }
            }
        }
        // Queen side castling (similar logic to King side)
        if (this->castlingEligible && !this->hasMoved) {
            const Piece* rookCandidatePiece = board.getPieceAt(position.first, 0); // Queen-side rook at col 0
            if (rookCandidatePiece != nullptr && rookCandidatePiece->getType() == ROOK) {
                const Rook* queenSideRook = static_cast<const Rook*>(rookCandidatePiece);
                if (queenSideRook->getIsCastlingEligible() && !queenSideRook->getHasMoved() &&
                    queenSideRook->getPosition().first == position.first &&
                    queenSideRook->getPosition().second == 0) {
                    
                    // Squares (row, col-1), (row, col-2), (row, col-3) must be empty
                    bool squaresBetweenAreEmpty =
                        (board.getPieceAt(position.first, position.second - 1) == nullptr &&
                         board.getPieceAt(position.first, position.second - 2) == nullptr &&
                         board.getPieceAt(position.first, position.second - 3) == nullptr);

                    if (squaresBetweenAreEmpty) {
                        Color oppositeColor = (this->color == BLACK) ? WHITE : BLACK;
                        std::vector<Move> opponentMoves = board.getAllLegalMoves(oppositeColor, false); // Pass false

                        auto isSquareAttacked = [&](int r_check, int c_check) {
                            for (const auto& oppMove : opponentMoves) {
                                if (oppMove.endPos.first == r_check && oppMove.endPos.second == c_check) {
                                    return true;
                                }
                            }
                            return false;
                        };
                        
                        // King must not be in check, pass through (col-1), or land on (col-2) attacked squares
                        bool kingIsSafeToCastle = true;
                        if (isSquareAttacked(position.first, position.second) ||      // Current square
                            isSquareAttacked(position.first, position.second - 1) ||  // Pass through
                            isSquareAttacked(position.first, position.second - 2)) {  // Land on
                            kingIsSafeToCastle = false;
                        }

                        if (kingIsSafeToCastle) {
                            moves.push_back(Move({row, col}, {row, col - 2}, this, nullptr, true, false, true)); // isCastling=true, isKingSide=false
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


