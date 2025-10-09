// BoardRenderer.h
#ifndef BOARD_RENDERER_H
#define BOARD_RENDERER_H

#include <SDL.h>
#include <vector>
#include <array>
#include <utility>

// Forward declarations
struct Move;
class Piece;
class Board;

// Forward declarations for types used by-value only as references
class BoardState;
class Piece;
struct Move;

struct RenderColors {
    SDL_Color selectedSquare = {0, 255, 0, 150};      // Semi-transparent green
    SDL_Color validMove = {0, 255, 0, 150};           // Semi-transparent green  
    SDL_Color invalidMove = {255, 0, 0, 150};         // Semi-transparent red
    SDL_Color lightSquare = {240, 217, 181, 255};     // Light chess square
    SDL_Color darkSquare = {181, 136, 99, 255};       // Dark chess square
};

struct RenderContext {
    const std::pair<int, int>* selectedSquare = nullptr;
    const std::vector<Move>* possibleMoves = nullptr;
    bool showCoordinates = false;
    bool highlightLastMove = false;
    const Move* lastMove = nullptr;
};

class BoardRenderer {
private:
    SDL_Renderer* renderer;
    RenderColors colors;
    
    // Board layout data - will be initialized from Board
    std::array<std::array<SDL_FRect, 8>, 8> boardGrid;
    bool isFlipped = false;
    float squareSide = 0;
    
    // Helper methods
    void setBlendModeAlpha();
    void resetBlendMode();
    void drawSquareHighlight(const SDL_FRect& rect, const SDL_Color& color);
    void drawChessBoard();
    
public:
    BoardRenderer(SDL_Renderer* renderer);
    ~BoardRenderer() = default;
    
    // Initialize layout from Board - called once during setup
    void initializeLayout(const std::array<std::array<SDL_FRect, 8>, 8>& grid, 
                         float squareSize, bool flipped);
    
    // Main drawing function - replaces Board::draw()
    void draw(const std::vector<Piece*>& pieces, 
             const RenderContext& context, Board* board); // Need board reference for checkIfMoveRemovesCheck
    
    // Individual drawing components
    void drawBackground();
    void drawSelectedSquareHighlight(const std::pair<int, int>& square);
    void drawPossibleMoveHighlights(const std::vector<Move>& moves, Board* board);
    void drawPieces(const std::vector<Piece*>& pieces);
    void drawLastMoveHighlight(const Move& move);
    void drawCoordinates(); // Optional feature
    
    // Configuration
    void setFlipped(bool flipped);
    void setColors(const RenderColors& newColors);
    void updateLayout(const std::array<std::array<SDL_FRect, 8>, 8>& grid, float squareSize);
    
    // Utility
    SDL_FRect getSquareRect(int row, int col) const;
    bool isValidSquare(int row, int col) const;
};

#endif // BOARD_RENDERER_H
