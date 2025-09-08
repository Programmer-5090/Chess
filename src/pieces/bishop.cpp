#include "pieces/bishop.h"
#include "board.h"
#include "enums.h" // Include enums.h for Color, PieceType

Bishop::Bishop(Color color, PieceType type, SDL_Renderer *renderer) : Piece(color, type, renderer)
{
    if (color == BLACK)
    {
        pieceImg = IMG_Load("images/B_Bishop.png");
    }
    else
    {
        pieceImg = IMG_Load("images/W_Bishop.png");
    }

    pieceText = SDL_CreateTextureFromSurface(renderer, pieceImg);
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
            const Piece *target = board.boardState[r][c].get();
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
