// Menu System Demo - Demonstrates the complete menu navigation system
// with chess board background as requested by the user

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>

#include <chess/ui/input.h>
#include <chess/ui/controls/ui/ui.h>
#include <chess/menus/manager.h>

static bool initSDL(SDL_Window** window, SDL_Renderer** renderer);
static void cleanup(SDL_Window* window, SDL_Renderer* renderer);

int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!initSDL(&window, &renderer)) {
        std::cout << "Failed to initialize!" << std::endl;
        return 1;
    }

    {
        // Scope UI so it is destroyed before renderer/window cleanup
        UIManager uiManager(renderer, 800, 600);
        MenuManager menuManager(renderer, 800, 600);
        Input input;

        bool running = true;
        bool gameStartRequested = false;

        // Start with the main menu
        menuManager.setState(MenuState::MAIN_MENU);
        
        // Set up callback for when game should start
        menuManager.setStartGameCallback([&gameStartRequested]() {
            gameStartRequested = true;
        });

        // Main loop
        while (running) {
            input.update();
            if (input.shouldQuit()) {
                running = false;
            }

            // Let the menu manager handle the input and state transitions
            menuManager.update(input);
            
            // Check if we should start the actual game
            if (gameStartRequested) {
                std::cout << "Starting game..." << std::endl;
                // Here you would transition to the actual chess game
                // For now, just exit the demo
                running = false;
            }

            // Render
            SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
            SDL_RenderClear(renderer);
            
            // Render the chess board background and current menu
            menuManager.render();
            
            SDL_RenderPresent(renderer);
        }
        // menuManager and UI elements destroyed here
    }

    cleanup(window, renderer);
    return 0;
}

static bool initSDL(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (TTF_Init() < 0) {
        std::cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return false;
    }
    
    *window = SDL_CreateWindow("Chess Game - Menu Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                              800, 600, SDL_WINDOW_SHOWN);
    if (!*window) {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer) {
        std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

static void cleanup(SDL_Window* window, SDL_Renderer* renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}
