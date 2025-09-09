#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "include/input.h"
#include "ui.h"

// Forward declarations
bool initSDL();
void cleanup();
void createMainMenu();
void createSecondMenu();
void handleEvents();
void update();
void render();
void setScreen(const std::string& screenName);

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool running = true;
Input input;
UIManager* uiManager = nullptr;
std::string fontPath = "assets/fonts/OpenSans-Regular.ttf";
std::string currentScreen = "main";
std::string pendingScreen = ""; // Used for deferred screen changes

int main(int argc, char* argv[]) {
    // Initialize SDL and create window/renderer
    if (!initSDL()) {
        std::cout << "Failed to initialize!" << std::endl;
        return 1;
    }

    // Create the UI Manager
    uiManager = new UIManager(renderer, 800, 600);

    // Set up the initial UI
    createMainMenu();

    // Main game loop
    while (running) {
        handleEvents();
        update();
        render();
    }

    // Clean up resources
    cleanup();
    return 0;
}

bool initSDL() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    // Create window
    window = SDL_CreateWindow(
        "Old UI Demo - Chess Game",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cout << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cout << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

void cleanup() {
    delete uiManager;
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void handleEvents() {
    input.update();
    if (input.shouldQuit()) {
        running = false;
    }
}

void update() {
    uiManager->update(input);
    
    // Handle any pending screen changes after the update cycle is complete
    if (!pendingScreen.empty()) {
        currentScreen = pendingScreen;
        
        if (currentScreen == "main") {
            createMainMenu();
        } else if (currentScreen == "second") {
            createSecondMenu();
        }
        
        pendingScreen = "";
    }
}

// Safe way to request a screen change
void setScreen(const std::string& screenName) {
    pendingScreen = screenName;
}

void render() {
    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);
    
    // Render UI elements
    uiManager->render();
    
    // Present the renderer
    SDL_RenderPresent(renderer);
}

void createMainMenu() {
    // Clear any existing UI
    uiManager->clearElements();
    
    // Create a title label
    Label* titleLabel = uiManager->addElement<Label>(
        400 - 150, 80, 
        "OLD UI SYSTEM DEMO", 
        tupleToColor(255, 255, 255, 255), 
        36, 
        fontPath
    );
    
    // Create main menu buttons
    int buttonWidth = 300;
    int buttonHeight = 50;
    int startY = 180;
    int spacing = 70;
    
    // First button - Go to Second Screen
    Button* firstButton = uiManager->addElement<Button>(
        400 - buttonWidth/2, startY, 
        buttonWidth, buttonHeight,
        "Go to Second Screen", 
        []() {
            std::cout << "Clicked first button - going to second screen" << std::endl;
            setScreen("second");
        },
        tupleToColor(100, 100, 200, 255),  // base color
        tupleToColor(130, 130, 230, 255),  // hover color
        fontPath,
        tupleToColor(0, 0, 0, 255),  // text color
        6,  // elevation
        24   // font size
    );
    
    // Second button
    Button* secondButton = uiManager->addElement<Button>(
        400 - buttonWidth/2, startY + spacing, 
        buttonWidth, buttonHeight,
        "Test Button 2", 
        []() {
            std::cout << "Clicked second button" << std::endl;
        },
        tupleToColor(100, 200, 100, 255),  // base color
        tupleToColor(130, 230, 130, 255),  // hover color
        fontPath,
        tupleToColor(0, 0, 0, 255),  // text color
        6,  // elevation
        24   // font size
    );
    
    // Exit button
    Button* exitButton = uiManager->addElement<Button>(
        400 - buttonWidth/2, startY + spacing*2, 
        buttonWidth, buttonHeight,
        "Exit", 
        [running_ptr = &running]() {
            std::cout << "Exiting application" << std::endl;
            *running_ptr = false;
        },
        tupleToColor(200, 100, 100, 255),  // base color
        tupleToColor(230, 130, 130, 255),  // hover color
        fontPath,
        tupleToColor(0, 0, 0, 255),  // text color
        6,  // elevation
        24   // font size
    );
    
    // Create a version label
    Label* versionLabel = uiManager->addElement<Label>(
        400 - 100, 570, 
        "Old UI System v1.0", 
        tupleToColor(180, 180, 180, 255), 
        14, 
        fontPath
    );
}

void createSecondMenu() {
    // Clear any existing UI
    uiManager->clearElements();
    
    // Create a title label
    Label* titleLabel = uiManager->addElement<Label>(
        400 - 150, 80, 
        "SECOND SCREEN", 
        tupleToColor(255, 255, 255, 255), 
        36, 
        fontPath
    );
    
    // Create buttons
    int buttonWidth = 300;
    int buttonHeight = 50;
    int startY = 180;
    int spacing = 70;
    
    // Back button
    Button* backButton = uiManager->addElement<Button>(
        400 - buttonWidth/2, startY, 
        buttonWidth, buttonHeight,
        "Back to Main Menu", 
        []() {
            std::cout << "Going back to main menu" << std::endl;
            setScreen("main");
        },
        tupleToColor(100, 100, 200, 255),  // base color
        tupleToColor(130, 130, 230, 255),  // hover color
        fontPath,
        tupleToColor(0, 0, 0, 255),  // text color
        6,  // elevation
        24   // font size
    );
    
    // Test button
    Button* testButton = uiManager->addElement<Button>(
        400 - buttonWidth/2, startY + spacing, 
        buttonWidth, buttonHeight,
        "Another Test Button", 
        []() {
            std::cout << "Clicked test button on second screen" << std::endl;
        },
        tupleToColor(100, 200, 100, 255),  // base color
        tupleToColor(130, 230, 130, 255),  // hover color
        fontPath,
        tupleToColor(0, 0, 0, 255),  // text color
        6,  // elevation
        24   // font size
    );
}
