#ifndef VS_PLAYER_MENU_H
#define VS_PLAYER_MENU_H
#include <SDL.h>
#include <chess/ui/controls/ui/ui.h>  
#include <vector>
#include <functional>
#include <string>

// Forward declaration
class Input;

class VSPlayerMenu {
    private:
        SDL_Renderer* renderer;
        int screenWidth;
        int screenHeight;
        UIManager uiManager;
        UIEnhancedBuilder uiBuilder;

        // UI Elements
        Label* titleLabel = nullptr;
        UITextInput* ipInputField = nullptr;
        Button* connectIP_StartButton = nullptr;
        Button* backButton = nullptr;

        std::vector<std::function<void()>> vsPlayerMenuCallbacks;
        std::vector<std::function<void()>> backCallbacks;

    public:
        VSPlayerMenu(SDL_Renderer* renderer, int screenWidth, int screenHeight)
            : renderer(renderer), screenWidth(screenWidth), screenHeight(screenHeight),
              uiManager(renderer, screenWidth, screenHeight), uiBuilder(&uiManager) {
            setupUI();
        }

        ~VSPlayerMenu() {
            uiManager.clearElements();
            vsPlayerMenuCallbacks.clear();
            backCallbacks.clear();
        }

        void setupUI() {
            uiManager.clearElements();
            vsPlayerMenuCallbacks.clear();
            backCallbacks.clear();
            
            uiBuilder.beginVerticalPanel({screenWidth/2 - 150, screenHeight/2 - 100, 300, 200}, 20, 15);

            titleLabel = uiBuilder.label("Play vs Player", {255, 255, 255, 255}, 32);
            uiBuilder.spacing(10);

            ipInputField = uiBuilder.textInput("Enter opponent IP", -1);
            uiBuilder.spacing(10);

            connectIP_StartButton = uiBuilder.button("Connect & Start", [this]() {
                for (const auto& cb : vsPlayerMenuCallbacks) cb();
            }, -1, 40);


            backButton = uiBuilder.button("Back", [this]() {
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

        void addStartGameCallback(std::function<void()> cb) {
            vsPlayerMenuCallbacks.push_back(std::move(cb));
        }

        void addBackCallback(std::function<void()> cb) {
            backCallbacks.push_back(std::move(cb));
        }
};

#endif // VS_PLAYER_MENU_H