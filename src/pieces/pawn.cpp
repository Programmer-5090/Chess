#include "pieces/pawn.h"
#include "board.h"
#include "enums.h"

Pawn::Pawn(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    enPassantCaptureEligible = false;

    if(color == BLACK){
        pieceImg = IMG_Load("images/B_Pawn.png");
        std::cout << "Loading black pawn image: " << (pieceImg ? "SUCCESS" : "FAILED") << std::endl;
        if (!pieceImg) std::cerr << "Error: " << IMG_GetError() << std::endl;
    }
    else{
        pieceImg = IMG_Load("images/W_Pawn.png");
        std::cout << "Loading white pawn image: " << (pieceImg ? "SUCCESS" : "FAILED") << std::endl;
        if (!pieceImg) std::cerr << "Error: " << IMG_GetError() << std::endl;
    }

    pieceText = SDL_CreateTextureFromSurface(renderer, pieceImg);
    std::cout << "Creating pawn texture: " << (pieceText ? "SUCCESS" : "FAILED") << std::endl;
    if (!pieceText) std::cerr << "Texture Error: " << SDL_GetError() << std::endl;
}

std::vector<Move> Pawn::getPseudoLegalMoves(const Board& board, bool generateCastlingMoves) const {
    int row = position.first, col = position.second;
    int dir = (color == BLACK ? +1 : -1);
    std::vector<Move> moves;

    // 1. Single step
    if (in_bounds(row+dir, col) && board.getPieceAt(row+dir, col) == nullptr) {
        auto target = board.getPieceAt(row+dir, col);
        moves.push_back(
            Move({row,col}, {row+dir,col}, this, target)
        );


        // 2. Two‐step on first move
        int startRow = (color==BLACK ? 1:6);
        // Check if the square in front is also empty for the two-step move
        if (row==startRow && in_bounds(row+dir*2, col) && board.getPieceAt(row+dir*2, col) == nullptr) {
            target = board.getPieceAt(row+dir*2, col);
            moves.push_back(Move({row,col}, {row+dir*2,col}, this, target));

        }
    }

    for (int dc : {-1, +1}) {
        int captureRow = row + dir;
        int captureCol = col + dc;
        if (!in_bounds(captureRow, captureCol)) 
            continue;
        auto target = board.getPieceAt(captureRow, captureCol);

        // 3. Diagonals
        if (target != nullptr && target->getColor() != this->color){
            bool isPromotion = is_back_rank(captureRow, color);
            moves.push_back(
                Move({row,col}, {captureRow, captureCol}, this, target)
            );
        }
        // 4. enPassant
        auto sidePiece = board.getPieceAt(row, captureCol); 
        if (target == nullptr && // Destination square must be empty
            sidePiece != nullptr && 
            sidePiece->getColor() != this->color &&
            sidePiece->getType() == PieceType::PAWN) {
            
            const Pawn* enemyPawn = static_cast<const Pawn*>(sidePiece);
            if (enemyPawn->getEnPassantCaptureEligible()) {
                // En passant capture. The captured piece is 'enemyPawn'.
                moves.push_back(
                    Move({row, col}, {captureRow, captureCol}, this, enemyPawn)
                );
            }
        }
    }

    return moves;
}

void Pawn::setEnPassantCaptureEligible(bool eligible) {
    enPassantCaptureEligible = eligible;
}

bool Pawn::getEnPassantCaptureEligible() const{
    return enPassantCaptureEligible;
}

bool is_back_rank(int row, Color color) {
    return row == (color==BLACK ? 7 : 0);
}
