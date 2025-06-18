#include "headers/pieces/rook.h"
#include "headers/board.h"
#include "headers/enums.h"   // Include enums.h for Color, PieceType

Rook::Rook(Color color, PieceType type, SDL_Renderer* renderer) : Piece(color, type, renderer) {
    castlingEligible = true;

    if(color == BLACK){
        pieceImg = IMG_Load("/Users/jethroaiyesan/Programming/Chess C++/images/B_Rook.png");
    }
    else{
        pieceImg = IMG_Load("/Users/jethroaiyesan/Programming/Chess C++/images/W_Rook.png");
    }

    pieceText = SDL_CreateTextureFromSurface(renderer, pieceImg);
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
            const Piece* target = board.boardState[r][c].get();
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