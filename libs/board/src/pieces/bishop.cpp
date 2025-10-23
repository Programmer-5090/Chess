#include <chess/board/pieces/bishop.h>
#include <chess/board/board.h>
#include <chess/enums.h>
#include <chess/rendering/texture_cache.h>
#include <chess/utils/profiler.h>

Bishop::Bishop(Color color, PieceType type, SDL_Renderer *renderer) : Piece(color, type, renderer)
{
    g_profiler.startTimer("piece_ctor_Bishop_internal");
    if(renderer){
            if (color == BLACK) {
            pieceText = TextureCache::getTexture("resources/B_Bishop.png");
        } else {
            pieceText = TextureCache::getTexture("resources/W_Bishop.png");
        }
    }
    g_profiler.endTimer("piece_ctor_Bishop_internal");
}

std::unique_ptr<Piece> Bishop::clone() const {
    return std::make_unique<Bishop>(*this); // Calls Bishop's copy constructor
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
            {
                if (target->getColor() != color)
                {
                    moves.push_back(Move(
                        std::make_pair(row, col),
                        std::make_pair(r, c),
                        this,
                        target));
                }
                break;
            }
            r += dr;
            c += dc;
        }
    }
    return moves;
}
