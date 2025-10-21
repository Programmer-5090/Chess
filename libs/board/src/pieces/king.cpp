#include <chess/board/pieces/king.h>
#include <chess/board/board.h>
#include <chess/enums.h>
#include <chess/board/pieces/rook.h>
#include <chess/rendering/texture_cache.h>
#include <chess/utils/profiler.h>

King::King(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    castlingEligible = true;
    if (renderer) {
        if (color == BLACK) {
            pieceText = TextureCache::getTexture("resources/B_King.png");
        } else {
            pieceText = TextureCache::getTexture("resources/W_King.png");
        }
    }
}

std::vector<Move> King::getPseudoLegalMoves(const Board& board, bool generateCastlingMoves) const {
    int row = position.first, col = position.second;
    std::vector<Move> moves;

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
                        if (!board.isSquareAttacked(row, col, oppositeColor) &&
                            !board.isSquareAttacked(row, col + 1, oppositeColor) &&
                            !board.isSquareAttacked(row, col + 2, oppositeColor)) {
                            moves.push_back(Move({row, col}, {row, col + 2}, this, nullptr, CastlingType::KING_SIDE));
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
                        if (!board.isSquareAttacked(row, col, oppositeColor) &&
                            !board.isSquareAttacked(row, col - 1, oppositeColor) &&
                            !board.isSquareAttacked(row, col - 2, oppositeColor)) {
                            moves.push_back(Move({row, col}, {row, col - 2}, this, nullptr, CastlingType::QUEEN_SIDE));
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


