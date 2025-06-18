#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include "input.h"
#include "board.h"
#include "gameLogic.h"
#include <iostream>


class Screen {
    SDL_Window* window;
    protected: SDL_Renderer* renderer;
    SDL_Surface* icon = nullptr;
    SDL_Surface* chessBoard = nullptr;
    SDL_Texture* boardTexture = nullptr;
    SDL_Rect boardRect{0, 0, 600, 600};
    Input input;
    Board gameBoard;
    GameLogic gameLogic;
    bool running = true;
    double deltaTime;

    public:
        Screen(int width=800, int height=800);


        virtual void show();


        virtual void update();

        void run();

        void destroy();

};