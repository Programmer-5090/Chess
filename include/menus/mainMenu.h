#ifndef MAIN_MENU_H
#define MAIN_MENU_H
#include <SDL.h>
#include "ui/ui.h"  
#include <vector>
#include <functional>
#include <string>

using namespace std;

// Forward declaration
class Input;

class MainMenu {
    private:
        SDL_Renderer* renderer;
        int screenWidth;
        int screenHeight;
        UIManager uiManager;
        UIEnhancedBuilder uiBuilder;

        // UI Elements
        Label* titleLabel = nullptr;
        Button* playButton = nullptr;
        Button* settingsButton = nullptr;
        Button* exitButton = nullptr;

        vector<function<void()>> mainMenuCallbacks;
        vector<function<void()>> settingsCallbacks;


    public:
        MainMenu(SDL_Renderer* renderer, int screenWidth, int screenHeight)
            : renderer(renderer), screenWidth(screenWidth), screenHeight(screenHeight),
              uiManager(renderer, screenWidth, screenHeight), uiBuilder(&uiManager) {
            setupUI();
        }

        ~MainMenu() {
            uiManager.clearElements();
            mainMenuCallbacks.clear();
        }

        void setupUI() {
            // Clear existing elements
            uiManager.clearElements();
            mainMenuCallbacks.clear();
            settingsCallbacks.clear();

            // Create main menu UI using UIEnhancedBuilder
            uiBuilder.beginVerticalPanel({screenWidth/2 - 150, screenHeight/2 - 100, 300, 200}, 20, 15);

            titleLabel = uiBuilder.label("Chess Game", {255, 255, 255, 255}, 32);
            uiBuilder.spacing(10);

            playButton = uiBuilder.button("Play", [this]() {
                for (const auto& cb : mainMenuCallbacks) cb();
            }, -1, 40);

            settingsButton = uiBuilder.button("Settings", [this]() {
                for (const auto& cb : settingsCallbacks) cb();
            }, -1, 40);

            exitButton = uiBuilder.button("Exit", []() {
                SDL_Event quitEvent;
                quitEvent.type = SDL_QUIT;
                SDL_PushEvent(&quitEvent);
            }, -1, 40);

            uiBuilder.endPanel();
        }

        void addPlayCallback(function<void()> cb) {
            mainMenuCallbacks.push_back(std::move(cb));
        }

        void addSettingsCallback(function<void()> cb) {
            settingsCallbacks.push_back(std::move(cb));
        }

        void update(Input& input) {
            uiManager.update(input);
        }
        void render() {
            uiManager.render();
        }
        void clearCallbacks() {
            mainMenuCallbacks.clear();
            settingsCallbacks.clear();
        }
};
#endif // MAIN_MENU_H