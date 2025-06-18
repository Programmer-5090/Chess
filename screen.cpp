#include "headers/screen.h"
#include <iostream>


const float CHESS_BOARD_OFFSET = 30.0f;

Screen::Screen(int width, int height) : gameBoard(width, height, CHESS_BOARD_OFFSET), gameLogic() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init failed: " << IMG_GetError() << "\n";
    }

    if (SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_SHOWN, &window, &renderer) < 0) {
            std::cerr << "Window and Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
    }

    icon = IMG_Load("/Users/jethroaiyesan/Programming/Chess C++/images/chess.png");
    chessBoard = IMG_Load("/Users/jethroaiyesan/Programming/Chess C++/images/board_plain_05.png");

    boardTexture = SDL_CreateTextureFromSurface(renderer, chessBoard);

    SDL_FreeSurface(chessBoard);
    chessBoard = nullptr;


    if (!icon) {
        std::cerr << "Failed to load icon: " << IMG_GetError() << "\n";
    } else {
        SDL_SetWindowIcon(window, icon);
    }


                
    SDL_SetWindowTitle(window, "Chess");
                                            

    input = Input();
    gameBoard.initializeBoard(renderer);
}

void Screen::show(){
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, boardTexture, NULL, &boardRect);
    // Pass selected square and possible moves to gameBoard.draw for highlighting
    gameBoard.draw(renderer, gameLogic.getSelectedPieceSquare(), &gameLogic.getPossibleMoves());

    SDL_RenderPresent(renderer);
}

void Screen::update(){
    // Handle input for game logic
    // Check for a new mouse click (rising edge)
    static bool wasLeftMouseButtonPressed = false;
    bool currentLeftMouseButtonState = input.getMouseStates()["left"];
    bool leftMouseButtonClicked = false;

    if (currentLeftMouseButtonState && !wasLeftMouseButtonPressed) {
        leftMouseButtonClicked = true;
    }
    wasLeftMouseButtonPressed = currentLeftMouseButtonState;

    if (leftMouseButtonClicked) {
        std::pair<int, int> mousePos = input.getMousePos();
        gameLogic.handleMouseClick(mousePos.first, mousePos.second, gameBoard, true);
    }
}

void Screen::run(){
    const double FIXED_DELTA = 1.0/60.0;
    double accumulator    = 0.0;
    Uint64 previousTime   = SDL_GetTicks64();

    while(running) {
        input.update();

        Uint64 currentTime = SDL_GetTicks64();
        double frameTime   = (currentTime - previousTime) / 1000.0;
        previousTime       = currentTime;

        if(frameTime > 0.25) frameTime = 0.25;
        accumulator += frameTime;

        while(accumulator >= FIXED_DELTA){
            accumulator -= FIXED_DELTA;
        }
        deltaTime = frameTime;

        update();
        show();

        if(input.shouldQuit())
            running = false;
    }
    destroy();
}

void Screen::destroy(){
    if (icon) {
        SDL_FreeSurface(icon);
        icon = nullptr;
    }
    if (boardTexture){
        SDL_DestroyTexture(boardTexture);
        boardTexture = nullptr;
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    window = nullptr;
    renderer = nullptr;
    IMG_Quit();
    SDL_Quit();
}
