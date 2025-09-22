#include "pieces/piece.h"
#include "board.h"

Piece::Piece(Color color, PieceType type, SDL_Renderer* renderer)
    :pieceImg(nullptr), pieceText(nullptr), renderer(renderer), color(color), type(type), value(0), points(0), hasMoved(false) {
    // Initialize position
    position = {-1, -1}; 
    // Value and points specific initializations
    switch (type) {
        case PAWN: points = 1;  name = "Pawn"; break;
        case KNIGHT: points = 3; name = "Knight"; break;
        case BISHOP: points = 3; name = "Bishop"; break;
        case ROOK: points = 5; name = "Rook"; break;
        case QUEEN: points = 9; name = "Queen"; break;
        case KING: points = 100; name = "King"; break;
        default: points = 0; name = ""; break;
    }
}

Piece::~Piece() {
    if (pieceText) {
        SDL_DestroyTexture(pieceText);
        pieceText = nullptr;
    }
    if (pieceImg) { // freed in derived classes after texture creation
        SDL_FreeSurface(pieceImg);
        pieceImg = nullptr;
    }
}

void Piece::setHasMoved(bool moved) {
    this->hasMoved = moved;
}

std::string Piece::stringPieceType() const{
    return name;
}

bool Piece::canCapture(int targetRow, int targetCol, const Board& board) const {
    auto targetPos = std::make_pair(targetRow, targetCol);
    for (const auto& move : getPseudoLegalMoves(board)) {
        if (move.endPos == targetPos && board.boardState[move.endPos.first][move.endPos.second] != nullptr){
            return true;
        }
    }
    return false;
}

void Piece::draw(SDL_FRect& boardSquareRect) {
    if (this->renderer && this->pieceText) {
        int texW = 0, texH = 0;
        SDL_QueryTexture(this->pieceText, NULL, NULL, &texW, &texH);
        if (texW == 0 || texH == 0) return;

        float textureAspectRatio = static_cast<float>(texW) / static_cast<float>(texH);
        
        SDL_FRect fittedRect;

        if (boardSquareRect.w / textureAspectRatio <= boardSquareRect.h) {
            fittedRect.w = boardSquareRect.w;
            fittedRect.h = boardSquareRect.w / textureAspectRatio;
        } else {
            fittedRect.h = boardSquareRect.h;
            fittedRect.w = boardSquareRect.h * textureAspectRatio;
        }

        const float pieceScaleFactor = 1.3f; // Current scale factor
        SDL_FRect destRect;
        destRect.w = fittedRect.w * pieceScaleFactor;
        destRect.h = fittedRect.h * pieceScaleFactor;

        destRect.x = boardSquareRect.x + (boardSquareRect.w - destRect.w) / 2.0f;
        
        const float visualVerticalOffset = -15.0f;
        destRect.y = boardSquareRect.y + (boardSquareRect.h - destRect.h) / 2.0f + visualVerticalOffset;

        SDL_RenderCopyF(this->renderer, this->pieceText, NULL, &destRect);
    }
}

bool Piece::in_bounds(int r,int c) const {
    return r >= 0 && r < 8 && c >= 0 && c < 8;
}

void Piece::setPosition(int r, int c) {
    position.first = r;
    position.second = c;
}
