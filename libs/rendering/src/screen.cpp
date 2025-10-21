#include <chess/rendering/screen.h>
#include <chess/utils/logger.h>
#include <chess/ui/input.h>
#include <chess/board/board.h>
#include <chess/board/game_logic.h>
#include <chess/menus/manager.h>
#include <chess/AI/ai.h>

const float CHESS_BOARD_OFFSET = 30.0f;

Screen::Screen(int width, int height) {
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

    icon = IMG_Load("resources/chess.png");
    chessBoard = IMG_Load("resources/board_plain_05.png");

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

    input = std::make_unique<Input>();
    gameBoard = std::make_unique<Board>(width, height, CHESS_BOARD_OFFSET);
    gameLogic = std::make_unique<GameLogic>();
    gameBoard->initializeBoard(renderer);
    
    menuManager = std::make_unique<MenuManager>(renderer, width, height);
    
    menuManager->setStartGameCallback([this]() {
        initializeGame();
    });
    
    menuManager->setAIConfigCallback([this](bool enabled, Color humanColor) {
        setupAI(enabled, humanColor);
    });
}

void Screen::show() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    if (menuManager->isInMenu()) {
        menuManager->render();
    } else {
        SDL_RenderCopy(renderer, boardTexture, NULL, &boardRect);
        // Use moves directly from GameLogic (UI Move type)
        const std::vector<Move>& legacyMoves = gameLogic->getPossibleMoves();
        gameBoard->draw(renderer, gameLogic->getSelectedPieceSquare(), &legacyMoves);

        gameBoard->renderPromotionDialog(renderer);
        
    }

    SDL_RenderPresent(renderer);
}

void Screen::update() {
    if (menuManager->isInMenu()) {
        menuManager->update(*input);
    } else {
        if (gameBoard->isPromotionDialogActive()) {
            gameBoard->updatePromotionDialog(*input);
            return; 
        }
        
        static bool wasLeftMouseButtonPressed = false;
        bool currentLeftMouseButtonState = input->getMouseStates()["left"];
        bool leftMouseButtonClicked = false;

        if (currentLeftMouseButtonState && !wasLeftMouseButtonPressed) {
            leftMouseButtonClicked = true;
        }
        wasLeftMouseButtonPressed = currentLeftMouseButtonState;

        if (leftMouseButtonClicked) {
            std::pair<int, int> mousePos = input->getMousePos();
            gameLogic->handleMouseClick(mousePos.first, mousePos.second, *gameBoard, true);
        }

        if (input->keyDown("R")) {
            resetGame();
        }

        gameLogic->update(*gameBoard);
    }
}

void Screen::run() {
    const double FIXED_DELTA = 1.0 / 60.0;
    double accumulator = 0.0;
    Uint64 previousTime = SDL_GetTicks64();

    while (running) {
        input->update();

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

        if (input->shouldQuit())
            running = false;
    }
    destroy();
}

void Screen::initializeGame() {
    resetGame();
}

void Screen::resetGame() {
    gameBoard->resetBoard(renderer);
    gameBoard->initializeBoard(renderer); 
    gameLogic = std::make_unique<GameLogic>(); 
    
    aiInstance.reset();
    
    setupAI(aiEnabled, playerColor);
    
    gameBoard->setFlipped(playerColor == BLACK);
}

void Screen::setupAI(bool enabled, Color humanColor) {
    aiEnabled = enabled;
    playerColor = humanColor;
    
    if (enabled) {
        if (!aiInstance) {
            aiInstance = std::make_shared<AI>(*gameBoard);
        }
        Color aiColor = (humanColor == WHITE) ? BLACK : WHITE;
        gameLogic->setAI(aiInstance, aiColor);
    } else {
        gameLogic->setAI(nullptr, NO_COLOR);
        aiInstance.reset();
    }
    
    std::cout << "AI " << (enabled ? "enabled" : "disabled") << 
                 ", Human plays as " << (humanColor == WHITE ? "WHITE" : "BLACK") << std::endl;
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
