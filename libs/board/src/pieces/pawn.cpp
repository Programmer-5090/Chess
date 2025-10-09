#include <chess/board/pieces/pawn.h>
#include <chess/board/board.h>
#include <chess/enums.h>
#include <chess/rendering/texture_cache.h>
#include <chess/utils/profiler.h>

Pawn::Pawn(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    enPassantCaptureEligible = false;
    if(renderer){
        if (color == BLACK) {
            pieceText = TextureCache::getTexture("resources/B_Pawn.png");
        } else {
            pieceText = TextureCache::getTexture("resources/W_Pawn.png");
        }
    }
}

// Replace your current Pawn::getPseudoLegalMoves() with this corrected version:

std::vector<Move> Pawn::getPseudoLegalMoves(const Board& board, bool generateCastlingMoves) const {
    int row = position.first, col = position.second;
    int dir = (color == BLACK ? +1 : -1);
    std::vector<Move> moves;

    // 1. Single step forward
    if (in_bounds(row+dir, col) && board.getPieceAt(row+dir, col) == nullptr) {
        auto target = board.getPieceAt(row+dir, col);
        
        // Check if this move results in promotion
        if (is_back_rank(row+dir, color)) {
            // Generate all 4 promotion moves
            addPromotionMoves(moves, row, col, row+dir, col, target);
        } else {
            moves.push_back(Move({row,col}, {row+dir,col}, this, target));
        }

        // 2. Two-step on first move
        int startRow = (color==BLACK ? 1:6);
        if (row==startRow && in_bounds(row+dir*2, col) && board.getPieceAt(row+dir*2, col) == nullptr) {
            target = board.getPieceAt(row+dir*2, col);
            moves.push_back(Move({row,col}, {row+dir*2,col}, this, target));
        }
    }

    // 3. Diagonal captures
    for (int dc : {-1, +1}) {
        int captureRow = row + dir;
        int captureCol = col + dc;
        if (!in_bounds(captureRow, captureCol)) 
            continue;
        auto target = board.getPieceAt(captureRow, captureCol);

        // Regular diagonal captures
        if (target != nullptr && target->getColor() != this->color) {
            if (is_back_rank(captureRow, color)) {
                // Generate all 4 promotion capture moves
                addPromotionMoves(moves, row, col, captureRow, captureCol, target);
            } else {
                moves.push_back(Move({row,col}, {captureRow, captureCol}, this, target));
            }
        }
        
        // 4. En passant captures
        auto sidePiece = board.getPieceAt(row, captureCol); 
        if (target == nullptr && // Destination square must be empty
            sidePiece != nullptr && 
            sidePiece->getColor() != this->color &&
            sidePiece->getType() == PieceType::PAWN) {
            
            const Pawn* enemyPawn = static_cast<const Pawn*>(sidePiece);
            if (enemyPawn->getEnPassantCaptureEligible()) {
                moves.push_back(Move({row, col}, {captureRow, captureCol}, this, enemyPawn));
            }
        }
    }

    return moves;
}

void Pawn::addPromotionMoves(std::vector<Move>& moves, int fromRow, int fromCol, 
                           int toRow, int toCol, Piece* capturedPiece) const {
    // Create 4 separate moves for each promotion piece type
    std::vector<PieceType> promotionTypes = {QUEEN, ROOK, BISHOP, KNIGHT};
    
    for (PieceType promoteType : promotionTypes) {
        moves.push_back(Move(
            {fromRow, fromCol},
            {toRow, toCol},
            this,
            capturedPiece,
            CastlingType::NONE,
            true,
            promoteType
        ));
    }
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
