#include <chess/board/pieces/queen.h>
#include <chess/board/board.h>
#include <chess/enums.h>
#include <chess/rendering/texture_cache.h>
#include <chess/utils/profiler.h>

Queen::Queen(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    g_profiler.startTimer("piece_ctor_Queen_internal");
    if(renderer){
        if (color == BLACK) {
            pieceText = TextureCache::getTexture("resources/B_Queen.png");
        } else {
            pieceText = TextureCache::getTexture("resources/W_Queen.png");
        }
    }
    g_profiler.endTimer("piece_ctor_Queen_internal");
}

std::unique_ptr<Piece> Queen::clone() const {
    return std::make_unique<Queen>(*this); // Calls Queen's copy constructor
}

std::vector<Move> Queen::getPseudoLegalMoves(const Board& board, bool generateCastlingMoves) const {
    int row = position.first, col = position.second;
    std::vector<Move> moves;

    static constexpr std::pair<int,int> dirs[] = {
        {+1,  0}, {-1,  0}, { 0, +1}, { 0, -1}, 
        {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1}
    };

    for (auto [dr, dc] : dirs){
        int r = row + dr;
        int c = col + dc;

        while (in_bounds(r, c)){
            const Piece* target = board.getPieceGrid()[r][c];
            if (!target){
                moves.push_back(Move(
                    std::make_pair(row, col),
                    std::make_pair(r  , c  ),
                    this,
                    target
                ));
            } else {
                if (target->getColor() != color) {
                    moves.push_back(Move(
                        std::make_pair(row, col),
                        std::make_pair(r,   c),
                        this,
                        target
                    ));
                }
                break;
            }
            r += dr; c += dc;
        }
    }
    
    return moves;
}


