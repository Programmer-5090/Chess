#ifndef VS_COMP_MENU_H
#define VS_COMP_MENU_H
#include <SDL.h>
#include "ui/ui.h"  
#include <vector>
#include <functional>
#include <string>

// Forward declaration
class Input;

class VSCompMenu {
    private:
        SDL_Renderer* renderer;
        int screenWidth;
        int screenHeight;
        UIManager uiManager;
        UIEnhancedBuilder uiBuilder;

        // UI Elements
        Label* titleLabel = nullptr;
        Button* startGameButton = nullptr;
        Button* loadFENButton = nullptr;
        Button* loadSavedGameButton = nullptr;
        Button* backButton = nullptr;

        std::vector<std::function<void()>> vsCompMenuCallbacks;
        std::vector<std::function<void()>> backCallbacks;

    public:
        VSCompMenu(SDL_Renderer* renderer, int screenWidth, int screenHeight)
            : renderer(renderer), screenWidth(screenWidth), screenHeight(screenHeight),
              uiManager(renderer, screenWidth, screenHeight), uiBuilder(&uiManager) {
            setupUI();
        }

        ~VSCompMenu() {
            uiManager.clearElements();
            vsCompMenuCallbacks.clear();
            backCallbacks.clear();
        }

        void setupUI() {
            // Clear existing elements
            uiManager.clearElements();
            vsCompMenuCallbacks.clear();
            backCallbacks.clear();
            backCallbacks.clear();

            // Create VS Computer menu UI using UIEnhancedBuilder
            uiBuilder.beginVerticalPanel({screenWidth/2 - 150, screenHeight/2 - 100, 300, 200}, 20, 15);

            titleLabel = uiBuilder.label("Play vs Computer", {255, 255, 255, 255}, 32);
            uiBuilder.spacing(10);

            startGameButton = uiBuilder.button("Start Game", [this]() {
                for (const auto& cb : vsCompMenuCallbacks) cb();
            }, -1, 40);

            loadFENButton = uiBuilder.button("Load FEN", []() {
                // Load FEN functionality (to be implemented)
            }, -1, 40);

            loadSavedGameButton = uiBuilder.button("Load Saved Game", []() {
                // Load saved game functionality (to be implemented)
            }, -1, 40);

            backButton = uiBuilder.button("Back", [this]() {
                for (const auto& cb : backCallbacks) cb();
            }, -1, 40);

            uiBuilder.endPanel();
        }

        void render() {
            uiManager.render();
        }

        void update(Input& input) {
            uiManager.update(input);
        }

        void addStartGameCallback(std::function<void()> cb) {
            vsCompMenuCallbacks.push_back(std::move(cb));
        }

        void addBackCallback(std::function<void()> cb) {
            backCallbacks.push_back(std::move(cb));
        }
};

#endif // VS_COMP_MENU_H