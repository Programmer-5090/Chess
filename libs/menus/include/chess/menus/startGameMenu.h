#ifndef STARTGAMEMENU_H
#define STARTGAMEMENU_H
#include <SDL.h>
#include <chess/ui/controls/ui/ui.h>  
#include <vector>
#include <functional>
#include <string>

// Forward declaration
class Input;

class StartGameMenu {
    private:
        SDL_Renderer* renderer;
        int screenWidth;
        int screenHeight;
        UIManager uiManager;
        UIEnhancedBuilder uiBuilder;

        // UI Elements
        Label* titleLabel = nullptr;
        Label* pickColorLabel = nullptr;
        Button* blackButton = nullptr;
        Button* whiteButton = nullptr;
        Button* backtoPreviousMenuButton = nullptr;

        std::vector<std::function<void()>> blackCallbacks;
        std::vector<std::function<void()>> whiteCallbacks;
        std::vector<std::function<void()>> backCallbacks;

    public:
        StartGameMenu(SDL_Renderer* renderer, int screenWidth, int screenHeight)
            : renderer(renderer), screenWidth(screenWidth), screenHeight(screenHeight),
              uiManager(renderer, screenWidth, screenHeight), uiBuilder(&uiManager) {
            setupUI();
        }
        ~StartGameMenu() {
            uiManager.clearElements();
            blackCallbacks.clear();
            whiteCallbacks.clear();
            backCallbacks.clear();
        }

        void setupUI() {
            // Clear existing elements
            uiManager.clearElements();
            blackCallbacks.clear();
            whiteCallbacks.clear();
            backCallbacks.clear();

            // Create start game menu UI using UIEnhancedBuilder
            uiBuilder.beginVerticalPanel({screenWidth/2 - 150, screenHeight/2 - 100, 300, 200}, 20, 15);

            titleLabel = uiBuilder.label("Start Game", {255, 255, 255, 255}, 32);
            uiBuilder.spacing(10);

            pickColorLabel = uiBuilder.label("Pick Your Color", {255, 255, 255, 255}, 24);
            uiBuilder.spacing(10);

            blackButton = uiBuilder.button("Black", [this]() {
                for (const auto& cb : blackCallbacks) cb();
            }, -1, 40);

            whiteButton = uiBuilder.button("White", [this]() {
                for (const auto& cb : whiteCallbacks) cb();
            }, -1, 40);

            backtoPreviousMenuButton = uiBuilder.button("Back", [this]() {
                for (const auto& cb : backCallbacks) cb();
            }, -1, 40);

            uiBuilder.endPanel();
        }

        void update(Input& input) {
            uiManager.update(input);
        }

        void render() {
            uiManager.render();
        }

        void addBlackCallback(std::function<void()> callback) {
            blackCallbacks.push_back(std::move(callback));
        }

        void addWhiteCallback(std::function<void()> callback) {
            whiteCallbacks.push_back(std::move(callback));
        }

        void addBackCallback(std::function<void()> callback) {
            backCallbacks.push_back(std::move(callback));
        }
};

#endif // START_GAME_MENU_H