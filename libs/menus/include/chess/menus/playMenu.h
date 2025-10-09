#ifndef PLAY_MENU_H
#define PLAY_MENU_H
#include <SDL.h>
#include <chess/ui/controls/ui/ui.h>  
#include <vector>
#include <functional>
#include <string>

// Forward declaration
class Input;

class PlayMenu {
    private:
        SDL_Renderer* renderer;
        int screenWidth;
        int screenHeight;
        UIManager uiManager;
        UIEnhancedBuilder uiBuilder;

        // UI Elements
        Label* titleLabel = nullptr;
        Button* VSCompButton = nullptr;
        Button* VSPlayerButton = nullptr;
        Button* exitToMainMenuButton = nullptr;

        std::vector<std::function<void()>> playMenuCallbacks;
        std::vector<std::function<void()>> vsPlayerCallbacks;
        std::vector<std::function<void()>> backCallbacks;

    public:
        PlayMenu(SDL_Renderer* renderer, int screenWidth, int screenHeight)
            : renderer(renderer), screenWidth(screenWidth), screenHeight(screenHeight),
              uiManager(renderer, screenWidth, screenHeight), uiBuilder(&uiManager) {
            setupUI();
        }
        ~PlayMenu() {
            uiManager.clearElements();
            playMenuCallbacks.clear();
            vsPlayerCallbacks.clear();
            backCallbacks.clear();
        }

        void setupUI() {
            // Clear existing elements
            uiManager.clearElements();
            playMenuCallbacks.clear();
            vsPlayerCallbacks.clear();
            backCallbacks.clear();

            // Create play menu UI using UIEnhancedBuilder
            uiBuilder.beginVerticalPanel({screenWidth/2 - 150, screenHeight/2 - 100, 300, 200}, 20, 15);

            titleLabel = uiBuilder.label("Play Game", {255, 255, 255, 255}, 32);
            uiBuilder.spacing(10);

            VSCompButton = uiBuilder.button("Play vs Computer", [this]() {
                for (const auto& cb : playMenuCallbacks) cb();
            }, -1, 40);

            VSPlayerButton = uiBuilder.button("Play vs Player", [this]() {
                for (const auto& cb : vsPlayerCallbacks) cb();
            }, -1, 40);

            exitToMainMenuButton = uiBuilder.button("Back to Main Menu", [this]() {
                for (const auto& cb : backCallbacks) cb();
            }, -1, 40);

            uiBuilder.endPanel();
        }

        void addPlayMenuCallback(std::function<void()> callback) {
            playMenuCallbacks.push_back(std::move(callback));
        }

        void addVsPlayerCallback(std::function<void()> callback) {
            vsPlayerCallbacks.push_back(std::move(callback));
        }

        void addBackCallback(std::function<void()> callback) {
            backCallbacks.push_back(std::move(callback));
        }

        void update(Input& input) {
            uiManager.update(input);
        }

        void render() {
            uiManager.render();
        }
};

#endif // PLAY_MENU_H