#ifndef SCREEN_H
#define SCREEN_H

#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <iostream>
#include "input.h"
#include "board.h"
#include "gameLogic.h"
#include "menus/menuManager.h"

class Screen {
public:
    Screen(int width = 800, int height = 800);
    
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
    Input input;
    Board gameBoard;
    GameLogic gameLogic;
    std::unique_ptr<MenuManager> menuManager;
    bool running = true;
    double deltaTime;
    
    void initializeGame();
    void resetGame();
    void applyBoardOrientation();
};

#endif // SCREEN_H
