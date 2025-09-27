#include "screen.h"
#include "logger.h"

const float CHESS_BOARD_OFFSET = 30.0f;

Screen::Screen(int width, int height) : gameBoard(width, height, CHESS_BOARD_OFFSET), gameLogic() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        LOG_ERROR(std::string("SDL could not initialize! SDL_Error: ") + SDL_GetError());
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        LOG_ERROR(std::string("IMG_Init failed: ") + IMG_GetError());
    }

    if (SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_SHOWN, &window, &renderer) < 0) {
        LOG_ERROR(std::string("Window and Renderer could not be created! SDL_Error: ") + SDL_GetError());
        SDL_Quit();
    }

    icon = IMG_Load("images/chess.png");
    chessBoard = IMG_Load("images/board_plain_05.png");

    if (!icon) {
        LOG_ERROR(std::string("Failed to load icon: ") + IMG_GetError());
    } else {
        LOG_INFO("Successfully loaded icon");
    }

    if (!chessBoard) {
        LOG_ERROR(std::string("Failed to load chessBoard: ") + IMG_GetError());
    } else {
        LOG_INFO("Successfully loaded chessBoard");
    }

    boardTexture = SDL_CreateTextureFromSurface(renderer, chessBoard);

    if (!boardTexture) {
        LOG_ERROR(std::string("Failed to create board texture: ") + SDL_GetError());
    } else {
        LOG_INFO("Successfully created board texture");
    }

    SDL_FreeSurface(chessBoard);
    chessBoard = nullptr;

    if (!icon) {
        LOG_ERROR(std::string("Failed to load icon: ") + IMG_GetError());
    } else {
        SDL_SetWindowIcon(window, icon);
    }

    SDL_SetWindowTitle(window, "Chess");

    input = Input();
    gameBoard.initializeBoard(renderer);
    
    // Initialize MenuManager after renderer is created
    menuManager = std::make_unique<MenuManager>(renderer, width, height);
    
    // Set up menu callbacks
    menuManager->setStartGameCallback([this]() {
        initializeGame();
    });
}

void Screen::show() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    if (menuManager->isInMenu()) {
        // Show menu system
        menuManager->render();
    } else {
        // Show game
        SDL_RenderCopy(renderer, boardTexture, NULL, &boardRect);
        // Pass selected square and possible moves to gameBoard.draw for highlighting
        gameBoard.draw(renderer, gameLogic.getSelectedPieceSquare(), &gameLogic.getPossibleMoves());

        // Render promotion dialog on top of everything
        gameBoard.renderPromotionDialog(renderer);
    }

    SDL_RenderPresent(renderer);
}

void Screen::update() {
    if (menuManager->isInMenu()) {
        // Update menu system
        menuManager->update(input);
    } else {
        // Handle promotion dialog first (if active)
        if (gameBoard.isPromotionDialogActive()) {
            gameBoard.updatePromotionDialog(input);
            return; // Don't process other input while dialog is active
        }
        
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
}

void Screen::run() {
    const double FIXED_DELTA = 1.0 / 60.0;
    double accumulator = 0.0;
    Uint64 previousTime = SDL_GetTicks64();

    while (running) {
        input.update();

        Uint64 currentTime = SDL_GetTicks64();
        double frameTime = (currentTime - previousTime) / 1000.0;
        previousTime = currentTime;

        if (frameTime > 0.25)
            frameTime = 0.25;
        accumulator += frameTime;

        while (accumulator >= FIXED_DELTA) {
            accumulator -= FIXED_DELTA;
        }
        deltaTime = frameTime;

        update();
        show();

        if (input.shouldQuit())
            running = false;
    }
    destroy();
}

void Screen::initializeGame() {
    // Reset the game to initial state
    resetGame();
}

void Screen::resetGame() {
    // Reset the board to starting position
    gameBoard.resetBoard(renderer);
    gameBoard.initializeBoard(renderer); // Reinitialize with renderer
    gameLogic = GameLogic(); // Reset game logic
}

void Screen::destroy() {
    if (icon) {
        SDL_FreeSurface(icon);
        icon = nullptr;
    }
    if (boardTexture) {
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
