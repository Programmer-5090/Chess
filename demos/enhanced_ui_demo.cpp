// Enhanced UI Demo - Shows improved panel integration and text wrapping
// Demonstrates how the enhanced builder works with UIPanel's layout system
// and provides automatic text wrapping for long content

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <string>

#include "input.h"
#include "ui/ui.h"

static bool initSDL(SDL_Window** window, SDL_Renderer** renderer);
static void cleanup(SDL_Window* window, SDL_Renderer* renderer);
static void createEnhancedDemo(UIEnhancedBuilder& ui, bool& running);

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
        UIEnhancedBuilder ui(&uiManager, "assets/fonts/OpenSans-Regular.ttf");
        Input input;

        bool running = true;

        // Create enhanced demo UI
        createEnhancedDemo(ui, running);

        // Main loop
        while (running) {
            input.update();
            if (input.shouldQuit()) {
                running = false;
            }

            uiManager.update(input);

            SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
            SDL_RenderClear(renderer);
            uiManager.render();
            SDL_RenderPresent(renderer);
        }
        // uiManager and UI elements destroyed here
    }

    cleanup(window, renderer);
    return 0;
}

static void createEnhancedDemo(UIEnhancedBuilder& ui, bool& running) {
    // Main container panel with vertical layout
    // Start with a reasonable height, panel will auto-grow as needed
    ui.beginVerticalPanel({40, 30, 700, 400}, 20, 12);
    
    ui.label("ENHANCED UI BUILDER DEMO", {255, 255, 255, 255}, 28);
    ui.spacing(10);
    
    // Text wrapping demo
    ui.label("Text Wrapping Demo:", {200, 200, 255, 255}, 18);
    ui.wrappedLabel("This is a long text that should automatically wrap to multiple lines when it exceeds the specified maximum width. The enhanced builder handles this automatically by creating multiple label elements.", 
                    600, {180, 180, 180, 255}, 14);
    ui.spacing(15);
    
    // Nested panel demo - Settings section
    ui.beginVerticalPanel({0, 0, 660, 150}, 15, 8, {40, 40, 50, 200});
    ui.label("Settings Panel", {255, 200, 100, 255}, 20);
    ui.checkbox("Enable notifications", true, [](bool checked) {
        std::cout << "Notifications: " << (checked ? "ON" : "OFF") << std::endl;
    });
    ui.checkbox("Auto-save", false, [](bool checked) {
        std::cout << "Auto-save: " << (checked ? "ON" : "OFF") << std::endl;
    });
    ui.endPanel();
    
    ui.spacing(10);
    
    // Interactive controls with auto-sizing
    ui.label("Interactive Controls (Auto-sized):", {200, 200, 255, 255}, 18);
    
    ui.dropdown({"Option 1", "Option 2", "Option 3", "A very long option name that tests wrapping"}, 0, -1,
                [](int idx, const std::string& text) {
                    std::cout << "Selected: " << text << " (index " << idx << ")" << std::endl;
                });
    
    ui.textInput("Enter some text here...", -1, [](const std::string& text) {
        std::cout << "Text submitted: " << text << std::endl;
    });
    
    // Removed side-by-side alignment demo; will demonstrate alignment with action buttons below.

    static double volume = 75.0;
    static Label* volumeLabel = nullptr;
    
    if (!volumeLabel) {
        volumeLabel = ui.label("Volume: 75%", {230, 230, 230, 255}, 16);
    }
    
    ui.slider(0.0, 100.0, volume, -1, [](double v) {
        volume = v;
        if (volumeLabel) {
            volumeLabel->setText("Volume: " + std::to_string(static_cast<int>(v)) + "%");
        }
    });
    
    ui.spacing(20);
    
    // Action buttons with proper left/center/right alignment using grid layout
    // Create a 3-column grid for precise alignment
    // Parameters: rect, columns, padding, spacing, background
    ui.beginGridPanel({0, 0, 1000, 50}, 3, 10, 20);
    
    // Left column - left-aligned button
    if (auto* showDlg = ui.button("Show Dialog", []() {
        std::cout << "Dialog would show here" << std::endl;
    }, 150, 40)) {
        showDlg->setHorizontalAlign(UIElement::HorizontalAlign::Left);
    }
    
    // Center column - center-aligned button  
    if (auto* resetBtn = ui.button("Reset Settings", []() {
        std::cout << "Settings reset" << std::endl;
    }, 150, 40)) {
        resetBtn->setHorizontalAlign(UIElement::HorizontalAlign::Center);
    }
    
    // Right column - right-aligned button
    if (auto* exitBtn = ui.button("Exit", [&running]() {
        std::cout << "Exiting..." << std::endl;
        running = false;
    }, 100, 40)) {
        exitBtn->setHorizontalAlign(UIElement::HorizontalAlign::Right);
    }
    
    ui.endPanel(); // End button grid
    
    ui.endPanel(); // End main panel
    
    // Footer outside main panel
    ui.label("Enhanced Builder with Panel Integration & Text Wrapping", {120, 120, 120, 255}, 12);
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
    
    *window = SDL_CreateWindow("Enhanced UI Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
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
