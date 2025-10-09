// Updated include paths to use correct locations
#include <chess/menus/manager.h>
#include <chess/menus/mainMenu.h>
#include <chess/menus/playMenu.h>
#include <chess/menus/startGameMenu.h>
#include <chess/menus/vsCompMenu.h>
#include <chess/menus/vsPlayerMenu.h>
#include <chess/menus/settingsMenu.h>
#include <chess/board/board.h>
#include <chess/utils/logger.h>
#include <SDL_image.h>
#include <iostream>

const float MENU_BOARD_OFFSET = 30.0f;

MenuManager::MenuManager(SDL_Renderer* renderer, int screenWidth, int screenHeight)
    : renderer(renderer), screenWidth(screenWidth), screenHeight(screenHeight),
      currentState(MenuState::MAIN_MENU), previousState(MenuState::MAIN_MENU),
      boardTexture(nullptr), chessBoardSurface(nullptr) {
    
    setupBoardBackground();
    initializeMenus();
    setupMenuCallbacks();
}

MenuManager::~MenuManager() {
    if (chessBoardSurface) {
        SDL_FreeSurface(chessBoardSurface);
    }
    if (boardTexture) {
        SDL_DestroyTexture(boardTexture);
    }
}

void MenuManager::setupBoardBackground() {
    // Create background board for visual effect
    backgroundBoard = std::make_unique<Board>(screenWidth, screenHeight, MENU_BOARD_OFFSET);
    backgroundBoard->initializeBoard(renderer);
    
    // Load chess board texture
    chessBoardSurface = IMG_Load("resources/board_plain_05.png");
    if (!chessBoardSurface) {
        LOG_ERROR(std::string("Failed to load chess board for menu background: ") + IMG_GetError());
    } else {
        boardTexture = SDL_CreateTextureFromSurface(renderer, chessBoardSurface);
        if (!boardTexture) {
            LOG_ERROR(std::string("Failed to create board texture for menu: ") + SDL_GetError());
        }
    }
    
    // Setup board rectangle to cover the screen
    boardRect = {0, 0, screenWidth, screenHeight};
}

void MenuManager::initializeMenus() {
    mainMenu = std::make_unique<MainMenu>(renderer, screenWidth, screenHeight);
    playMenu = std::make_unique<PlayMenu>(renderer, screenWidth, screenHeight);
    startGameMenu = std::make_unique<StartGameMenu>(renderer, screenWidth, screenHeight);
    vsCompMenu = std::make_unique<VSCompMenu>(renderer, screenWidth, screenHeight);
    vsPlayerMenu = std::make_unique<VSPlayerMenu>(renderer, screenWidth, screenHeight);
    settingsMenuInstance = std::make_unique<SettingsMenu>(renderer, screenWidth, screenHeight);
}

void MenuManager::setupMenuCallbacks() {
    // Main Menu callbacks
    mainMenu->addPlayCallback([this]() {
        setState(MenuState::PLAY_MENU);
    });
    
    mainMenu->addSettingsCallback([this]() {
        setState(MenuState::SETTINGS_MENU);
    });
    
    // Play Menu callbacks
    playMenu->addPlayMenuCallback([this]() {
        setState(MenuState::VS_COMP_MENU);
    });
    
    playMenu->addVsPlayerCallback([this]() {
        setState(MenuState::VS_PLAYER_MENU);
    });
    
    playMenu->addBackCallback([this]() {
        setState(MenuState::MAIN_MENU);
    });
    
    // VS Computer Menu callbacks
    vsCompMenu->addStartGameCallback([this]() {
        setState(MenuState::START_GAME_MENU);
    });
    
    vsCompMenu->addBackCallback([this]() {
        setState(MenuState::PLAY_MENU);
    });
    
    // VS Player Menu callbacks (no networking yet, just go to start game menu)
    vsPlayerMenu->addStartGameCallback([this]() {
        setState(MenuState::START_GAME_MENU);
    });
    
    vsPlayerMenu->addBackCallback([this]() {
        setState(MenuState::PLAY_MENU);
    });
    
    // Settings Menu callbacks
    settingsMenuInstance->addBackCallback([this]() {
        setState(MenuState::MAIN_MENU);
    });
}

void MenuManager::renderBackground() {
    // Clear screen with dark background
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_RenderClear(renderer);
    
    if (boardTexture) {
        // Render chess board with transparency
        SDL_SetTextureAlphaMod(boardTexture, 128); // Semi-transparent
        SDL_RenderCopy(renderer, boardTexture, nullptr, &boardRect);
        SDL_SetTextureAlphaMod(boardTexture, 255); // Reset alpha
    }
    
    // Render pieces with lower opacity for background effect
    if (backgroundBoard) {
        // Set blend mode for semi-transparent pieces
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // Render board pieces with reduced opacity
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (backgroundBoard->getPieceGrid()[i][j]) {
                    SDL_FRect rect = backgroundBoard->getSquareRect(i, j);
                    // Render pieces normally (we'll handle transparency with overlay)
                    backgroundBoard->getPieceGrid()[i][j]->draw(rect);
                }
            }
        }
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    
    // Add dark overlay to make menus more visible
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100); // Semi-transparent black overlay
    SDL_RenderFillRect(renderer, nullptr);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void MenuManager::update(Input& input) {
    switch (currentState) {
        case MenuState::MAIN_MENU:
            mainMenu->update(input);
            break;
        case MenuState::PLAY_MENU:
            playMenu->update(input);
            break;
        case MenuState::SETTINGS_MENU:
            settingsMenuInstance->update(input);
            break;
        case MenuState::VS_COMP_MENU:
            vsCompMenu->update(input);
            break;
        case MenuState::VS_PLAYER_MENU:
            vsPlayerMenu->update(input);
            break;
        case MenuState::START_GAME_MENU:
            startGameMenu->update(input);
            break;
        case MenuState::IN_GAME:
            // Game update handled elsewhere
            break;
    }
}

void MenuManager::render() {
    renderBackground();
    
    switch (currentState) {
        case MenuState::MAIN_MENU:
            mainMenu->render();
            break;
        case MenuState::PLAY_MENU:
            playMenu->render();
            break;
        case MenuState::SETTINGS_MENU:
            settingsMenuInstance->render();
            break;
        case MenuState::VS_COMP_MENU:
            vsCompMenu->render();
            break;
        case MenuState::VS_PLAYER_MENU:
            vsPlayerMenu->render();
            break;
        case MenuState::START_GAME_MENU:
            startGameMenu->render();
            break;
        case MenuState::IN_GAME:
            // Game rendering handled elsewhere
            break;
    }
}

void MenuManager::setState(MenuState newState) {
    previousState = currentState;
    currentState = newState;
    
    // Handle special transitions
    if (newState == MenuState::IN_GAME && startGameCallback) {
        startGameCallback();
    }
}

void MenuManager::setStartGameCallback(std::function<void()> callback) {
    startGameCallback = std::move(callback);
    
    // Set up color-specific start game callbacks
    startGameMenu->addWhiteCallback([this]() {
        chosenBottomColor = WHITE; // White at bottom
        setState(MenuState::IN_GAME);
    });

    startGameMenu->addBlackCallback([this]() {
        chosenBottomColor = BLACK; // Black at bottom
        setState(MenuState::IN_GAME);
    });

    startGameMenu->addBackCallback([this]() {
        setState(MenuState::PLAY_MENU);
    });
}

void MenuManager::goToPreviousMenu() {
    setState(previousState);
}
