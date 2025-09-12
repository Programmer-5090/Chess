// Improved UI Demo using UIBuilder
// Shows how the new immediate-mode-style API makes UI creation cleaner
// while keeping all the benefits of retained mode (state persistence, complex interactions)

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>

#include "input.h"
#include "ui/ui.h"
#include "ui/uiBuilder.h"

static bool initSDL(SDL_Window** window, SDL_Renderer** renderer);
static void cleanup(SDL_Window* window, SDL_Renderer* renderer);
static void createMainScreen(UIBuilder& ui, bool& running, std::string& screen);
static void createSecondScreen(UIBuilder& ui, std::string& screen);

int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!initSDL(&window, &renderer)) {
        std::cout << "Failed to initialize!" << std::endl;
        return 1;
    }

    UIManager uiManager(renderer, 800, 600);
    UIBuilder ui(&uiManager, "assets/fonts/OpenSans-Regular.ttf");
    Input input;
    
    bool running = true;
    std::string currentScreen = "";
    std::string nextScreen = "main";  // Start with main screen

    // Create initial screen
    createMainScreen(ui, running, nextScreen);
    currentScreen = "main";

    // Main loop
    while (running) {
        input.update();
        if (input.shouldQuit()) {
            running = false;
        }

        // Handle screen changes
        if (nextScreen != currentScreen) {
            currentScreen = nextScreen;
            ui.clear();
            
            if (currentScreen == "main") {
                createMainScreen(ui, running, nextScreen);
            } else if (currentScreen == "second") {
                createSecondScreen(ui, nextScreen);
            }
        }

        uiManager.update(input);

        SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
        SDL_RenderClear(renderer);
        uiManager.render();
        SDL_RenderPresent(renderer);
    }

    cleanup(window, renderer);
    return 0;
}

static void createMainScreen(UIBuilder& ui, bool& running, std::string& screen) {
    // Create main panel with vertical layout
    ui.beginVerticalLayout({150, 50, 500, 500}, 15);
    
    // Title
    ui.label("IMPROVED UI DEMO", {255, 255, 255, 255}, 36);
    ui.spacing(10);
    
    // Navigation buttons
    ui.button("Go to Second Screen", [&screen]() {
        std::cout << "Going to second screen" << std::endl;
        screen = "second";
    }, 460, 50);
    
    // Dialog demo
    ui.button("Show Dialog", [&ui]() {
        ui.dialog("Confirm Action", 
                 "This demonstrates the improved dialog API!",
                 []() { std::cout << "Dialog OK clicked" << std::endl; },
                 []() { std::cout << "Dialog cancelled" << std::endl; });
    }, 460, 50);
    
    ui.spacing(5);
    
    // Interactive controls
    ui.dropdown({"New Game", "Load Game", "Settings", "About"}, 0, 460,
                [](int idx, const std::string& text) {
                    std::cout << "Selected: " << text << " (index " << idx << ")" << std::endl;
                });
    
    ui.textInput("Type something here...", 460,
                [](const std::string& text) {
                    std::cout << "Submitted: " << text << std::endl;
                });
    
    // Slider with live label update
    static double volume = 50.0;
    static Label* volumeLabel = nullptr;
    
    if (!volumeLabel) {
        volumeLabel = ui.label("Volume: 50", {230, 230, 230, 255}, 18);
    }
    
    ui.slider(0.0, 100.0, volume, 460, [](double v) {
        volume = v;
        if (volumeLabel) {
            volumeLabel->setText("Volume: " + std::to_string(static_cast<int>(v)));
        }
        std::cout << "Volume: " << static_cast<int>(v) << std::endl;
    });
    
    ui.spacing(10);
    
    // Checkbox controls
    ui.checkbox("Show advanced options", false, [](bool checked) {
        std::cout << "Advanced options: " << (checked ? "ON" : "OFF") << std::endl;
    });
    
    ui.spacing(20);
    
    // Exit button
    ui.button("Exit Application", [&running]() {
        std::cout << "Exiting..." << std::endl;
        running = false;
    }, 460, 50);
    
    ui.endLayout();
    
    // Footer outside the panel
    ui.label("Built with improved UIBuilder API", {150, 150, 150, 255}, 12);
}

static void createSecondScreen(UIBuilder& ui, std::string& screen) {
    // Centered layout for second screen
    ui.beginVerticalLayout({200, 100, 400, 400}, 20);
    
    ui.label("SECOND SCREEN", {255, 255, 255, 255}, 36);
    ui.spacing(30);
    
    ui.button("Back to Main Menu", [&screen]() {
        std::cout << "Returning to main menu" << std::endl;
        screen = "main";
    }, 300, 50);
    
    ui.button("Test Button", []() {
        std::cout << "Test button clicked on second screen" << std::endl;
    }, 300, 50);
    
    // Horizontal layout demo
    ui.spacing(20);
    ui.endLayout();
    
    // Show horizontal layout
    ui.beginHorizontalLayout({200, 350, 400, 60}, 10);
    ui.button("Left", []() { std::cout << "Left clicked" << std::endl; }, 90, 40);
    ui.button("Center", []() { std::cout << "Center clicked" << std::endl; }, 90, 40);
    ui.button("Right", []() { std::cout << "Right clicked" << std::endl; }, 90, 40);
    ui.endLayout();
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
    
    *window = SDL_CreateWindow("Improved UI Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
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
