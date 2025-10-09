#include "pieces/queen.h"
#include "board.h"
#include "enums.h"   // Include enums.h for Color, PieceType
#include "perfProfiler.h"
#include "textureCache.h"

Queen::Queen(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    g_profiler.startTimer("piece_ctor_Queen_internal");
    // Use global texture cache (loads on first request)
    if(renderer){
        if (color == BLACK) {
            pieceText = TextureCache::getTexture("images/B_Queen.png");
        } else {
            pieceText = TextureCache::getTexture("images/W_Queen.png");
        }
    }
    g_profiler.endTimer("piece_ctor_Queen_internal");
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
            } else { // Found a piece
                if (target->getColor() != color) { // Enemy piece
                    moves.push_back(Move(
                        std::make_pair(row, col),
                        std::make_pair(r,   c),
                        this,
                        target
                    ));
                }
                break; // Stop ray in this direction
            }
            r += dr; c += dc;
        }
    }
    
    return moves;
}


