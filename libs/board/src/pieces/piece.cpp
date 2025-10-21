#include <chess/board/pieces/piece.h>
#include <chess/board/board.h>
#include <chess/utils/logger.h>
#include <chess/utils/profiler.h>
#include <chess/board/move_executor.h>
#include <sstream>

unsigned int Piece::next_white_id = 1;
unsigned int Piece::next_black_id = 101;

Piece::Piece(Color color, PieceType type, SDL_Renderer* renderer)
    :pieceImg(nullptr), pieceText(nullptr), renderer(renderer), color(color), type(type), value(0), points(0), hasMoved(false) {
    position = {-1, -1}; 
    if (color == WHITE) {
        id = next_white_id++;
    } else {
        id = next_black_id++;
    }
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
    pieceText = nullptr;
    pieceImg = nullptr;
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
        if (move.endPos == targetPos && board.getPieceGrid()[move.endPos.first][move.endPos.second] != nullptr){
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

        const float pieceScaleFactor = 1.3f;
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

void Piece::getPseudoLegalMoves(const Board& board, std::vector<Move>& out, bool generateCastlingMoves) const {
    std::vector<Move> tmp = getPseudoLegalMoves(board, generateCastlingMoves);
    out.insert(out.end(), tmp.begin(), tmp.end());
}
