#include "pieces/rook.h"
#include "board.h"
#include "enums.h"
#include "perfProfiler.h"
#include "textureCache.h"

Rook::Rook(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    g_profiler.startTimer("piece_ctor_Rook_internal");
    castlingEligible = true;

    if(color == BLACK){
           pieceText = TextureCache::getTexture("images/B_Rook.png");
    }
    else{
           pieceText = TextureCache::getTexture("images/W_Rook.png");
    }

    // Texture is obtained from TextureCache (already an SDL_Texture*).
    // No need to call SDL_CreateTextureFromSurface here.
    g_profiler.endTimer("piece_ctor_Rook_internal");
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
