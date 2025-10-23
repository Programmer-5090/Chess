#include <chess/board/pieces/rook.h>
#include <chess/board/board.h>
#include <chess/enums.h>
#include <chess/rendering/texture_cache.h>
#include <chess/utils/profiler.h>

Rook::Rook(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    g_profiler.startTimer("piece_ctor_Rook_internal");
    castlingEligible = true;
    if(renderer){
        if(color == BLACK){
           pieceText = TextureCache::getTexture("resources/B_Rook.png");
        }
        else{
            pieceText = TextureCache::getTexture("resources/W_Rook.png");
        }
    }

    g_profiler.endTimer("piece_ctor_Rook_internal");
}

std::unique_ptr<Piece> Rook::clone() const {
    return std::make_unique<Rook>(*this); // Calls Rook's copy constructor
}

std::vector<Move> Rook::getPseudoLegalMoves(const Board& board, bool generateCastlingMoves) const {
    const int row = position.first, col = position.second;
    std::vector<Move> moves;

    static constexpr std::pair<int,int> dirs[] = {
        {+1, 0}, {-1, 0}, {0, +1}, {0, -1}
    };

    for (auto [dr, dc] : dirs) {
        int r = row + dr, c = col + dc;
        while (in_bounds(r, c)) {
            const Piece* target = board.getPieceGrid()[r][c];
            if (!target) {
                moves.push_back(Move(
                    std::make_pair(row, col),
                    std::make_pair(r,   c),
                    this,
                    target)
                );
            }
            else {
                if (target->getColor() != color) {
                    moves.push_back(Move(
                        std::make_pair(row, col),
                        std::make_pair(r,   c),
                        this,
                        target)
                    );
                }
                break;
            }
            r += dr; c += dc;
        }
    }
    return moves;
}

bool Rook::getIsCastlingEligible() const{
    return castlingEligible;
}

void Rook::setIsCastlingEligible(bool eligible){
    castlingEligible = eligible;
}
