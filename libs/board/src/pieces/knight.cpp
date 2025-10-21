#include <chess/board/pieces/knight.h>
#include <chess/board/board.h>
#include <chess/enums.h>
#include <chess/rendering/texture_cache.h>
#include <chess/utils/profiler.h>

Knight::Knight(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    g_profiler.startTimer("piece_ctor_Knight_internal");
    if(renderer){
        if (color == BLACK) {
            pieceText = TextureCache::getTexture("resources/B_Knight.png");
        } else {
            pieceText = TextureCache::getTexture("resources/W_Knight.png");
        }
    }
    g_profiler.endTimer("piece_ctor_Knight_internal");
}

std::vector<Move> Knight::getPseudoLegalMoves(const Board& board, bool generateCastlingMoves) const {
    int row = position.first, col = position.second;
    std::vector<Move> moves;

    static constexpr std::pair<int,int> dirs[] = {
        {+2, +1}, {-2, +1}, {+2, -1}, {-2, -1},
        {+1, +2}, {-1, +2}, {+1, -2}, {-1, -2}
    };

    for (auto [dr, dc] : dirs){
        int r = row + dr;
        int c = col + dc;
        
        if (in_bounds(r, c)){
            const Piece* target = board.getPieceGrid()[r][c];
            if (!target){
                moves.push_back(Move(
                    std::make_pair(row, col),
                    std::make_pair(r  , c  ),
                    this,
                    target
                ));
            }
            else if (target->getColor() != color) {
                moves.push_back(Move(
                    std::make_pair(row, col),
                    std::make_pair(r,   c),
                    this,
                    target
                ));
            }
        }
    }
    
    return moves;
}
