#include "chess/rendering/board_renderer.h"
#include "chess/board/board.h" 
#include "chess/board/move_executor.h"
#include "chess/board/pieces/piece.h"
#include <chess/board/bitboard/board_state.h>
#include "chess/board/pieces/piece_const.h"
#include "chess/rendering/texture_cache.h"

BoardRenderer::BoardRenderer(SDL_Renderer* renderer) : renderer(renderer) {
}

void BoardRenderer::initializeLayout(const std::array<std::array<SDL_FRect, 8>, 8>& grid, 
                                    float squareSize, bool flipped) {
    boardGrid = grid;
    squareSide = squareSize;
    isFlipped = flipped;
}

void BoardRenderer::draw(const std::vector<Piece*>& pieces, 
                        const RenderContext& context, Board* board) {
    if (!renderer) return;
    
    drawBackground();
    
    setBlendModeAlpha();
    
    if (context.selectedSquare) {
        drawSelectedSquareHighlight(*context.selectedSquare);
    }
    
    if (context.highlightLastMove && context.lastMove) {
        drawLastMoveHighlight(*context.lastMove);
    }
    
    if (context.possibleMoves) {
        drawPossibleMoveHighlights(*context.possibleMoves, board);
    }
    
    resetBlendMode();
    
    drawPieces(pieces);
    
    if (context.showCoordinates) {
        drawCoordinates();
    }
}

void BoardRenderer::drawBackground() {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            SDL_Color color = ((row + col) % 2 == 0) ? colors.lightSquare : colors.darkSquare;
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRectF(renderer, &boardGrid[row][col]);
        }
    }
}

void BoardRenderer::drawSelectedSquareHighlight(const std::pair<int, int>& square) {
    if (!isValidSquare(square.first, square.second)) return;
    
    SDL_FRect rect = getSquareRect(square.first, square.second);
    drawSquareHighlight(rect, colors.selectedSquare);
}

void BoardRenderer::drawPossibleMoveHighlights(const std::vector<Move>& moves, Board* board) {
    if (!board) return; 
    
    for (const auto& move : moves) {
        if (!isValidSquare(move.endPos.first, move.endPos.second)) continue;
        
        SDL_FRect rect = getSquareRect(move.endPos.first, move.endPos.second);
        
        SDL_Color color = board->checkIfMoveRemovesCheck(move) ? 
                         colors.validMove : colors.invalidMove;
        
        drawSquareHighlight(rect, color);
    }
}

void BoardRenderer::drawPieces(const std::vector<Piece*>& pieces) {
    for (Piece* piece : pieces) {
        if (!piece) continue;
        
        std::pair<int, int> pos = piece->getPosition();
        if (!isValidSquare(pos.first, pos.second)) continue;
        
        SDL_FRect rect = boardGrid[pos.first][pos.second];
        piece->draw(rect);
    }
}

void BoardRenderer::drawPieces(const chess::BitboardState& bbState) {
    if (!renderer) return;

    for (int sq = 0; sq < 64; ++sq) {
        int piece = bbState.square[sq];
        if (piece == chess::PIECE_NONE) continue;

        int type = chess::typeOf(piece);
        int color = chess::colorOf(piece);

        std::string name;
        switch (type) {
            case chess::PIECE_PAWN: name = "Pawn"; break;
            case chess::PIECE_KNIGHT: name = "Knight"; break;
            case chess::PIECE_BISHOP: name = "Bishop"; break;
            case chess::PIECE_ROOK: name = "Rook"; break;
            case chess::PIECE_QUEEN: name = "Queen"; break;
            case chess::PIECE_KING: name = "King"; break;
            default: continue;
        }

        std::string prefix = (color == chess::COLOR_WHITE) ? "W_" : "B_";
        std::string path = std::string("resources/") + prefix + name + ".png";

        SDL_Texture* tex = TextureCache::getTexture(path);
        if (!tex) continue;

        int texW = 0, texH = 0;
        SDL_QueryTexture(tex, NULL, NULL, &texW, &texH);
        if (texW == 0 || texH == 0) continue;

        float textureAspectRatio = static_cast<float>(texW) / static_cast<float>(texH);

        int rank = sq / 8;
        int file = sq % 8;
        int row = 7 - rank;
        int col = file;
        if (!isValidSquare(row, col)) continue;

        SDL_FRect boardSquareRect = boardGrid[row][col];

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

        SDL_RenderCopyF(this->renderer, tex, NULL, &destRect);
    }
}

void BoardRenderer::drawLastMoveHighlight(const Move& move) {
    SDL_Color highlightColor = {255, 255, 0, 100}; 
    
    if (isValidSquare(move.startPos.first, move.startPos.second)) {
        SDL_FRect startRect = getSquareRect(move.startPos.first, move.startPos.second);
        drawSquareHighlight(startRect, highlightColor);
    }
    
    if (isValidSquare(move.endPos.first, move.endPos.second)) {
        SDL_FRect endRect = getSquareRect(move.endPos.first, move.endPos.second);
        drawSquareHighlight(endRect, highlightColor);
    }
}

void BoardRenderer::drawCoordinates() {
}

void BoardRenderer::setBlendModeAlpha() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

void BoardRenderer::resetBlendMode() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void BoardRenderer::drawSquareHighlight(const SDL_FRect& rect, const SDL_Color& color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(renderer, &rect);
}

void BoardRenderer::setFlipped(bool flipped) {
    isFlipped = flipped;
}

void BoardRenderer::setColors(const RenderColors& newColors) {
    colors = newColors;
}

void BoardRenderer::updateLayout(const std::array<std::array<SDL_FRect, 8>, 8>& grid, float squareSize) {
    boardGrid = grid;
    squareSide = squareSize;
}

SDL_FRect BoardRenderer::getSquareRect(int row, int col) const {
    if (isValidSquare(row, col)) {
        return boardGrid[row][col];
    }
    return {0, 0, 0, 0};
}

bool BoardRenderer::isValidSquare(int row, int col) const {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

void BoardRenderer::drawBB(const chess::BitboardState& bbState, const RenderContextBB& context) {
    if (!renderer) return;

    drawBackground();
    setBlendModeAlpha();

    if (context.selectedSquare) {
        drawSelectedSquareHighlight(*context.selectedSquare);
    }

    if (context.highlightLastMove && context.lastMove) {
        const chess::BBMove& m = *context.lastMove;
        int from = m.startSquare();
        int to = m.targetSquare();

        int fromRank = from / 8; int fromFile = from % 8; int fromRow = 7 - fromRank;
        int toRank = to / 8; int toFile = to % 8; int toRow = 7 - toRank;

        drawSquareHighlight(getSquareRect(fromRow, fromFile), SDL_Color{255,0,0,100});
        drawSquareHighlight(getSquareRect(toRow, toFile), SDL_Color{255,255,0,100});
    }

    if (context.possibleMoves) {
        for (const auto &bm : *context.possibleMoves) {
            int to = bm.targetSquare();
            int toRank = to / 8; int toFile = to % 8; int toRow = 7 - toRank;
            if (!isValidSquare(toRow, toFile)) continue;
            SDL_FRect rect = getSquareRect(toRow, toFile);
            drawSquareHighlight(rect, colors.validMove);
        }
    }

    resetBlendMode();

    drawPieces(bbState);

    if (context.showCoordinates) drawCoordinates();
}