#ifndef VS_COMP_MENU_H
#define VS_COMP_MENU_H
#include <SDL.h>
#include <chess/ui/controls/ui/ui.h>  
#include <vector>
#include <functional>
#include <string>
#include <chess/enums.h>

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

        std::function<void(bool, Color)> aiConfigCallback;
        bool aiEnabled = true;
        Color chosenBottomColor = WHITE;

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
            uiManager.clearElements();
            vsCompMenuCallbacks.clear();
            backCallbacks.clear();

            uiBuilder.beginVerticalPanel({screenWidth/2 - 150, screenHeight/2 - 100, 300, 200}, 20, 15);

            titleLabel = uiBuilder.label("Play vs Computer", {255, 255, 255, 255}, 32);
            uiBuilder.spacing(10);

            startGameButton = uiBuilder.button("Start Game", [this]() {
                if (aiConfigCallback) {
                    aiConfigCallback(aiEnabled, chosenBottomColor);
                }
                for (const auto& cb : vsCompMenuCallbacks) cb();
            }, -1, 40);

            loadFENButton = uiBuilder.button("Load FEN", []() {
            }, -1, 40);

            loadSavedGameButton = uiBuilder.button("Load Saved Game", []() {
            }, -1, 40);

            backButton = uiBuilder.button("Back", [this]() {
                for (const auto& cb : backCallbacks) cb();
            }, -1, 40);

            uiBuilder.endPanel();
        }

    public:
        void render();
        void update(Input& input);

        void addStartGameCallback(std::function<void()> cb) {
            vsCompMenuCallbacks.push_back(std::move(cb));
        }

        void addBackCallback(std::function<void()> cb) {
            backCallbacks.push_back(std::move(cb));
        }

        void setAIConfigCallback(std::function<void(bool, Color)> cb) {
            aiConfigCallback = std::move(cb);
        }

        void setChosenBottomColor(Color color) {
            chosenBottomColor = color;
        }
};

#endif // VS_COMP_MENU_H