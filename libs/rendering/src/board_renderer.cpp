// BoardRenderer.cpp
#include "chess/rendering/board_renderer.h"
#include "chess/board/board.h" // For checkIfMoveRemovesCheck
#include "chess/board/move_executor.h"
#include "chess/board/pieces/piece.h"

BoardRenderer::BoardRenderer(SDL_Renderer* renderer) : renderer(renderer) {
    // Default colors already set in RenderColors struct
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
    
    // Draw chess board background (optional - you might handle this elsewhere)
    drawBackground();
    
    // Draw highlights first (so pieces render on top)
    setBlendModeAlpha();
    
    // Highlight selected piece's square
    if (context.selectedSquare) {
        drawSelectedSquareHighlight(*context.selectedSquare);
    }
    
    // Highlight last move (if enabled)
    if (context.highlightLastMove && context.lastMove) {
        drawLastMoveHighlight(*context.lastMove);
    }
    
    // Highlight possible moves
    if (context.possibleMoves) {
        drawPossibleMoveHighlights(*context.possibleMoves, board);
    }
    
    resetBlendMode();
    
    // Draw pieces
    drawPieces(pieces);
    
    // Draw coordinates (if enabled)
    if (context.showCoordinates) {
        drawCoordinates();
    }
}

void BoardRenderer::drawBackground() {
    // Draw alternating light/dark squares
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
    if (!board) return; // Can't check move validity without board reference
    
    for (const auto& move : moves) {
        if (!isValidSquare(move.endPos.first, move.endPos.second)) continue;
        
        SDL_FRect rect = getSquareRect(move.endPos.first, move.endPos.second);
        
        // Choose color based on whether move removes check
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

void BoardRenderer::drawLastMoveHighlight(const Move& move) {
    // Draw subtle highlight for the last move made
    SDL_Color highlightColor = {255, 255, 0, 100}; // Semi-transparent yellow
    
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
    // TODO: Implement coordinate drawing (A-H, 1-8)
    // This would require font rendering which might need additional setup
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
    // Note: The actual grid recalculation should be done by Board and passed via updateLayout
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