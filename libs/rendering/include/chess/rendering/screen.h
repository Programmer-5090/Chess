#ifndef SCREEN_H
#define SCREEN_H

#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <iostream>
#include <memory>
#include <chess/enums.h>

// Forward declarations
class Input;
class Board;
class GameLogic;
class BoardBB;
class GameLogicBB;
class MenuManager;
class AI;

class Screen {
public:
    // set useBitboard=true to use BoardBB/GameLogicBB instead of legacy Board/GameLogic
    Screen(int width = 800, int height = 800, bool useBitboard = false);
    
    virtual void show();
    virtual void update();
    void run();
    void destroy();

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Surface* icon = nullptr;
    SDL_Surface* chessBoard = nullptr;
    SDL_Texture* boardTexture = nullptr;
    SDL_Rect boardRect{0, 0, 600, 600};
    std::unique_ptr<Input> input;
    std::unique_ptr<Board> gameBoard;
    std::unique_ptr<GameLogic> gameLogic;
    std::unique_ptr<BoardBB> gameBoardBB;
    std::unique_ptr<GameLogicBB> gameLogicBB;
    bool useBitboard = false;
    std::unique_ptr<MenuManager> menuManager;
    bool running = true;
    double deltaTime;
    bool aiEnabled = false;
    Color playerColor = WHITE;
    std::shared_ptr<AI> aiInstance;
    
    void initializeGame();
    void resetGame();
    void applyBoardOrientation();
    void setupAI(bool enabled, Color humanColor);
};

#endif // SCREEN_H
