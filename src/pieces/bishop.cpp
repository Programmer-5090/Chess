#include "pieces/bishop.h"
#include "board.h"
#include "enums.h" // Include enums.h for Color, PieceType
#include "perfProfiler.h"
#include "textureCache.h"

Bishop::Bishop(Color color, PieceType type, SDL_Renderer *renderer) : Piece(color, type, renderer)
{
    g_profiler.startTimer("piece_ctor_Bishop_internal");
    if (color == BLACK) {
        pieceText = TextureCache::getTexture("images/B_Bishop.png");
    } else {
        pieceText = TextureCache::getTexture("images/W_Bishop.png");
    }
    g_profiler.endTimer("piece_ctor_Bishop_internal");
}

std::vector<Move> Bishop::getPseudoLegalMoves(const Board &board, bool generateCastlingMoves) const
{
    const int row = position.first, col = position.second;
    std::vector<Move> moves;

    static constexpr std::pair<int, int> dirs[] = {
        {+1, +1}, {-1, -1}, {-1, +1}, {+1, -1}};

    for (auto [dr, dc] : dirs)
    {
        int r = row + dr, c = col + dc;
        while (in_bounds(r, c))
        {
            const Piece *target = board.getPieceGrid()[r][c];
            if (!target)
            {
                moves.push_back(Move(
                    std::make_pair(row, col),
                    std::make_pair(r, c),
                    this,
                    target));
            }
            else
            { // Found a piece
                if (target->getColor() != color)
                { // Enemy piece
                    moves.push_back(Move(
                        std::make_pair(row, col),
                        std::make_pair(r, c),
                        this,
                        target));
                }
                break; // Stop ray in this direction
            }
            r += dr;
            c += dc;
        }
    }
    return moves;
}
