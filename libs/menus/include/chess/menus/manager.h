#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <SDL.h>
#include <memory>
#include <functional>
// Updated to fully-qualified public headers
#include <chess/board/board.h>
#include <chess/ui/input.h>

// Forward declarations
class MainMenu;
class PlayMenu;
class StartGameMenu;
class VSCompMenu;
class VSPlayerMenu;
class SettingsMenu;

enum class MenuState {
    MAIN_MENU,
    PLAY_MENU,
    SETTINGS_MENU,
    VS_COMP_MENU,
    VS_PLAYER_MENU,
    START_GAME_MENU,
    IN_GAME
};

class MenuManager {
private:
    SDL_Renderer* renderer;
    int screenWidth;
    int screenHeight;
    
    MenuState currentState;
    MenuState previousState;
    
    // Menu instances
    std::unique_ptr<MainMenu> mainMenu;
    std::unique_ptr<PlayMenu> playMenu;
    std::unique_ptr<StartGameMenu> startGameMenu;
    std::unique_ptr<VSCompMenu> vsCompMenu;
    std::unique_ptr<VSPlayerMenu> vsPlayerMenu;
    std::unique_ptr<SettingsMenu> settingsMenuInstance;
    
    // Chess board background
    std::unique_ptr<Board> backgroundBoard;
    SDL_Texture* boardTexture;
    SDL_Surface* chessBoardSurface;
    SDL_Rect boardRect;
    
    // Game state callback
    std::function<void()> startGameCallback;
    Color chosenBottomColor = WHITE;
    
    void setupBoardBackground();
    void renderBackground();
    void initializeMenus();
    void setupMenuCallbacks();
    
public:
    MenuManager(SDL_Renderer* renderer, int screenWidth, int screenHeight);
    ~MenuManager();
    
    void update(Input& input);
    void render();
    
    void setState(MenuState newState);
    MenuState getCurrentState() const { return currentState; }
    
    void setStartGameCallback(std::function<void()> callback);
    Color getChosenBottomColor() const { return chosenBottomColor; }
    
    bool isInMenu() const { return currentState != MenuState::IN_GAME; }
    void goToPreviousMenu();
};

#endif // MENU_MANAGER_H
