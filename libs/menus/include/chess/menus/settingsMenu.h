#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include <SDL.h>
#include <chess/ui/controls/ui/ui.h>  
#include <vector>
#include <functional>
#include <string>

// Forward declaration
class Input;

class SettingsMenu {
    private:
        SDL_Renderer* renderer;
        int screenWidth;
        int screenHeight;
        UIManager uiManager;
        UIEnhancedBuilder uiBuilder;

        // UI Elements
        Label* titleLabel = nullptr;
        Button* audioButton = nullptr;
        Button* videoButton = nullptr;
        Button* controlsButton = nullptr;
        Button* backButton = nullptr;

        std::vector<std::function<void()>> settingsMenuCallbacks;

    public:
        SettingsMenu(SDL_Renderer* renderer, int screenWidth, int screenHeight)
            : renderer(renderer), screenWidth(screenWidth), screenHeight(screenHeight),
              uiManager(renderer, screenWidth, screenHeight), uiBuilder(&uiManager) {
            setupUI();
        }

        ~SettingsMenu() {
            uiManager.clearElements();
            settingsMenuCallbacks.clear();
        }

        void setupUI() {
            uiManager.clearElements();
            settingsMenuCallbacks.clear();

            uiBuilder.beginVerticalPanel({screenWidth/2 - 150, screenHeight/2 - 100, 300, 250}, 20, 15);

            titleLabel = uiBuilder.label("Settings", {255, 255, 255, 255}, 32);
            uiBuilder.spacing(10);

            audioButton = uiBuilder.button("Audio", []() {
            }, -1, 40);

            videoButton = uiBuilder.button("Video", []() {
            }, -1, 40);

            controlsButton = uiBuilder.button("Controls", []() {
            }, -1, 40);

            backButton = uiBuilder.button("Back", [this]() {
                for (const auto& cb : settingsMenuCallbacks) cb();
            }, -1, 40);

            uiBuilder.endPanel();
        }

        void addBackCallback(std::function<void()> cb) {
            settingsMenuCallbacks.push_back(std::move(cb));
        }

        void update(Input& input) {
            uiManager.update(input);
        }

        void render() {
            uiManager.render();
        }
};

#endif // SETTINGS_MENU_H