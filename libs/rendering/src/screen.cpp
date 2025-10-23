#include <chess/rendering/screen.h>
#include <chess/utils/logger.h>
#include <chess/ui/input.h>
#include <chess/board/board.h>
#include <chess/board/game_logic.h>
#include <chess/board/boardBB.h>
#include <chess/board/game_logicBB.h>
#include <chess/menus/manager.h>
#include <chess/AI/ai.h>
#include <chess/AI/ai_bb.h>
#include <future>
#include <thread>
#include <atomic>

const float CHESS_BOARD_OFFSET = 30.0f;

Screen::Screen(int width, int height, bool useBitboardFlag) : useBitboard(useBitboardFlag) {
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
    if (useBitboard) {
        gameBoardBB = std::make_unique<BoardBB>(width, height, CHESS_BOARD_OFFSET);
        gameLogicBB = std::make_unique<GameLogicBB>();
        gameBoardBB->initializeBoard(renderer);
    } else {
        gameBoard = std::make_unique<Board>(width, height, CHESS_BOARD_OFFSET);
        gameLogic = std::make_unique<GameLogic>();
        gameBoard->initializeBoard(renderer);
    }
    
    menuManager = std::make_unique<MenuManager>(renderer, width, height);
    
    menuManager->setStartGameCallback([this]() {
        initializeGame();
    });
    
    menuManager->setAIConfigCallback([this](bool enabled, Color humanColor) {
        setupAI(enabled, humanColor);
    });
}

// Helper to start async search if needed
static std::pair<std::pair<chess::BBMove, int>, std::string> runAIFullSearchCopy(const BoardBB& liveBoard, int depth, unsigned threadCount) {
    BoardBB localBoard(100, 100, CHESS_BOARD_OFFSET);
    std::string fen = liveBoard.getCurrentFEN();
    localBoard.loadFEN(fen, nullptr);
    AI_BB localAI(threadCount == 0 ? 1u : threadCount);
    auto res = localAI.getSearchResult(localBoard, depth);
    return {res, fen};
}

void Screen::show() {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    if (menuManager->isInMenu()) {
        menuManager->render();
    } else {
        SDL_RenderCopy(renderer, boardTexture, NULL, &boardRect);
        if (useBitboard) {
            const std::vector<chess::BBMove>& bbMoves = gameLogicBB->getPossibleMoves();
            gameBoardBB->draw(renderer, gameLogicBB->getSelectedPieceSquare(), &bbMoves);
            gameBoardBB->renderPromotionDialog(renderer);
        } else {
            const std::vector<Move>& legacyMoves = gameLogic->getPossibleMoves();
            gameBoard->draw(renderer, gameLogic->getSelectedPieceSquare(), &legacyMoves);
            gameBoard->renderPromotionDialog(renderer);
        }
        
    }

    SDL_RenderPresent(renderer);
}

void Screen::update() {
    if (menuManager->isInMenu()) {
        menuManager->update(*input);
    } else {
        if (useBitboard) {
            if (gameBoardBB->isPromotionDialogActive()) {
                gameBoardBB->updatePromotionDialog(*input);
                return;
            }
        } else {
            if (gameBoard->isPromotionDialogActive()) {
                gameBoard->updatePromotionDialog(*input);
                return; 
            }
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
            if (useBitboard) {
                gameLogicBB->handleMouseClick(mousePos.first, mousePos.second, *gameBoardBB, true);
            } else {
                gameLogic->handleMouseClick(mousePos.first, mousePos.second, *gameBoard, true);
            }
        }

        if (input->keyDown("R")) {
            resetGame();
        }

        if (useBitboard) {
            if (gameLogicBB) gameLogicBB->update(*gameBoardBB);
        } else {
            gameLogic->update(*gameBoard);
        }
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
    if (useBitboard) {
        if (gameBoardBB) {
            gameBoardBB->setFlipped(playerColor == BLACK);
            gameBoardBB->resetBoard(renderer);
            gameBoardBB->initializeBoard(renderer);
        }
        gameLogicBB = std::make_unique<GameLogicBB>();
        if (gameLogicBB && gameBoardBB) gameLogicBB->setCurrentPlayer(gameBoardBB->getCurrentPlayer());
        
        // Re-attach AI to the new GameLogicBB instance
        if (aiEnabled && aiInstanceBB) {
            Color aiColor = (playerColor == WHITE) ? BLACK : WHITE;
            gameLogicBB->setAI(aiInstanceBB, aiColor);
            gameLogicBB->setAISettings(aiSearchDepth, aiThreadCount == 0 ? 1u : aiThreadCount);
        }
    } else {
        gameBoard->setFlipped(playerColor == BLACK);
        gameBoard->resetBoard(renderer);
        gameBoard->initializeBoard(renderer); 
        gameLogic = std::make_unique<GameLogic>();
        
        // Re-attach AI to the new GameLogic instance
        if (aiEnabled && aiInstance) {
            Color aiColor = (playerColor == WHITE) ? BLACK : WHITE;
            gameLogic->setAI(aiInstance, aiColor);
        }
    }
}

void Screen::setupAI(bool enabled, Color humanColor) {
    aiEnabled = enabled;
    playerColor = humanColor;
    
    if (enabled) {
        if (!aiInstance) {
            if (!useBitboard) aiInstance = std::make_shared<AI>(*gameBoard);
        }
        Color aiColor = (humanColor == WHITE) ? BLACK : WHITE;
        if (useBitboard) {
            if (!aiInstanceBB) {
                    unsigned maxThreads = 8; // max threads for AI, avoid over-allocation
                    unsigned tc = (aiThreadCount == 0 ? std::thread::hardware_concurrency() : aiThreadCount);
                    if (tc == 0) tc = 1;
                    if (tc > maxThreads) tc = maxThreads;
                    try {
                        aiInstanceBB = std::make_shared<AI_BB>(tc);
                    } catch (const std::bad_alloc& e) {
                        LOG_ERROR(std::string("Failed to allocate AI_BB with ") + std::to_string(tc) + " threads: " + e.what());
                        // Fallback to single-threaded AI instance
                        try {
                            aiInstanceBB = std::make_shared<AI_BB>(1u);
                        } catch (const std::bad_alloc& e2) {
                            LOG_ERROR(std::string("Failed to allocate fallback single-thread AI_BB: ") + e2.what());
                            aiInstanceBB = nullptr;
                        }
                    }
            }
                if (gameLogicBB && aiInstanceBB) {
                    LOG_INFO("Screen: Attaching AI_BB to GameLogicBB for color " + std::string(aiColor == WHITE ? "WHITE" : "BLACK"));
                    gameLogicBB->setAI(aiInstanceBB, aiColor);
                    gameLogicBB->setAISettings(aiSearchDepth, aiThreadCount == 0 ? 1u : aiThreadCount);
                    aiBBColor = aiColor;
                } else {
                    LOG_ERROR("Screen: Failed to attach AI - gameLogicBB=" + std::string(gameLogicBB ? "valid" : "null") + ", aiInstanceBB=" + std::string(aiInstanceBB ? "valid" : "null"));
                }
        } else {
            gameLogic->setAI(aiInstance, aiColor);
        }
    } else {
        if (useBitboard) gameLogicBB->setAI(nullptr, NO_COLOR);
        else gameLogic->setAI(nullptr, NO_COLOR);
        aiInstance.reset();
        aiInstanceBB.reset();
        aiBBColor = NO_COLOR;
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
