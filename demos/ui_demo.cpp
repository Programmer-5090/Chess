// UI Demo (refactored) — lightweight showcase of the UI components and layouts
//
// What this does
// - Initializes SDL (window + renderer)
// - Creates a UIManager and builds a simple screen made of a UIPanel and controls
// - Demonstrates: Button, Label, Checkbox, Modal Dialog, Dropdown (with overlay),
//                 TextInput, Slider, and Panel layouts (Vertical/Grid/Custom)
// - Includes a global "callbacks enabled" toggle to pause user callbacks
//
// Notes
// - Edit mode: enable it via the bottom-left checkbox to freely drag panel children.
//   Dragging is clamped to the panel and visuals update continuously during movement.
// - Dropdown list renders in an overlay pass so it stays on top.

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "input.h"
#include "ui/ui.h"

// Forward declarations
static bool initSDL(SDL_Window** window, SDL_Renderer** renderer);
static void cleanup(SDL_Window* window, SDL_Renderer* renderer, UIManager* uiManager);
static void createMainMenu(UIManager* uiManager, std::string& pendingScreen, bool& running, const std::string& fontPath);
static void createSecondMenu(UIManager* uiManager, std::string& pendingScreen, const std::string& fontPath);

int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!initSDL(&window, &renderer)) {
        std::cout << "Failed to initialize!" << std::endl;
        return 1;
    }

    // Build UI manager and state
    UIManager* uiManager = new UIManager(renderer, 800, 600);
    Input input;
    bool running = true;
    std::string fontPath = "assets/fonts/OpenSans-Regular.ttf";
    std::string currentScreen = "main";
    std::string pendingScreen;

    createMainMenu(uiManager, pendingScreen, running, fontPath);

    // Event loop
    while (running) {
        input.update();
        if (input.shouldQuit()) {
            running = false;
        }

        uiManager->update(input);

        // Late-apply requested screen changes
        if (!pendingScreen.empty()) {
            currentScreen = pendingScreen;
            if (currentScreen == "main") {
                createMainMenu(uiManager, pendingScreen, running, fontPath);
            } else if (currentScreen == "second") {
                createSecondMenu(uiManager, pendingScreen, fontPath);
            }
            pendingScreen.clear();
        }

        SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
        SDL_RenderClear(renderer);
        uiManager->render();
        SDL_RenderPresent(renderer);
    }

    cleanup(window, renderer, uiManager);
    return 0;
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
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }
    *window = SDL_CreateWindow("Refactored UI Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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

static void cleanup(SDL_Window* window, SDL_Renderer* renderer, UIManager* uiManager) {
    delete uiManager;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

static void createMainMenu(UIManager* uiManager, std::string& pendingScreen, bool& running, const std::string& fontPath) {
    uiManager->clearElements();

    // Main container panel
    UIPanel* panel = uiManager->addElement<UIPanel>(150, 50, 500, 500, tupleToColor(30,30,40,220), tupleToColor(60,60,80,255), 2);
    panel->setLayoutVertical(20, 20, 12);
    const int childW = 500 - 2 * 20; // panel width minus horizontal padding

    // Title inside panel
    panel->addChild<Label>(0, 0, "UI DEMO", tupleToColor(255,255,255,255), 36, fontPath);

    // Buttons inside panel
    panel->addChild<Button>(0, 0, childW, 50,
        "Go to Second Screen",
        [&pendingScreen]() { std::cout << "Clicked first button - going to second screen" << std::endl; pendingScreen = "second"; },
        tupleToColor(100, 100, 200, 255),
        tupleToColor(130, 130, 230, 255),
        fontPath,
        tupleToColor(0,0,0,255),
        6,
        24);

    // Show dialog button (dialog itself is modal on UIManager)
    panel->addChild<Button>(0, 0, childW, 50,
        "Show Dialog",
        [uiManager, fontPath](){
            auto* dlg = uiManager->addElement<UIDialog>(200, 160, 400, 220,
                std::string("Confirm Action"),
                std::string("Proceed with this action?"),
                std::string("OK"), std::string("Cancel"), fontPath);
            dlg->setOnOk([dlg](){ std::cout << "OK pressed" << std::endl; dlg->visible = false; });
            dlg->setOnCancel([dlg](){ std::cout << "Cancel pressed" << std::endl; dlg->visible = false; });
        },
        tupleToColor(100, 200, 100, 255),
        tupleToColor(130, 230, 130, 255),
        fontPath,
        tupleToColor(0,0,0,255),
        6,
        24);

    // Dropdown inside panel
    std::vector<std::string> items = {"New Game", "Load", "Settings", "About"};
    UIDropdown* dd = panel->addChild<UIDropdown>(0, 0, childW, 36, items, 0, fontPath, 18,
        tupleToColor(235,235,235,255), tupleToColor(80,80,80,255), tupleToColor(30,30,30,255), tupleToColor(210,210,210,255), tupleToColor(250,250,250,255));
    dd->setOnChange([](int idx, const std::string& value){ std::cout << "Dropdown selected: " << idx << " - " << value << std::endl; });

    // Text input inside panel
    UITextInput* ti = panel->addChild<UITextInput>(0, 0, childW, 36, "Type here...", fontPath, 18);
    ti->setOnChange([](const std::string& /*t*/){ /* live change, keep quiet */ });
    ti->setOnSubmit([](const std::string& t){ std::cout << "Submitted text: " << t << std::endl; });

    // Slider + value label inside panel
    Label* sliderLabel = panel->addChild<Label>(0, 0, "Volume: 50", tupleToColor(230,230,230,255), 18, fontPath);
    UISlider* slider = panel->addChild<UISlider>(0, 0, childW, 24, 0.0, 100.0, 50.0);
    slider->setOnChange([sliderLabel](double v){
        int vi = static_cast<int>(v + 0.5);
        sliderLabel->setText(std::string("Volume: ") + std::to_string(vi));
        std::cout << "Slider value: " << vi << std::endl;
    });

    // Exit inside panel
    panel->addChild<Button>(0, 0, childW, 50,
        "Exit",
        [&running]() { std::cout << "Exiting application" << std::endl; running = false; },
        tupleToColor(200, 100, 100, 255),
        tupleToColor(230, 130, 130, 255),
        fontPath,
        tupleToColor(0,0,0,255),
        6,
        24);

    // Outside controls: toggle panel visibility and layout/editing controls
    UICheckbox* showPanelCb = uiManager->addElement<UICheckbox>(170, 560, 24,
        "Show panel", true, tupleToColor(230,230,230,255), tupleToColor(100,200,120,255), tupleToColor(80,80,80,255), tupleToColor(255,255,255,255), 18, fontPath);
    showPanelCb->setOnChange([panel](bool isOn){ panel->visible = isOn; std::cout << "Checkbox changed: " << (isOn?"on":"off") << std::endl; });

    UICheckbox* editLayoutCb = uiManager->addElement<UICheckbox>(340, 560, 24,
        "Edit layout", false, tupleToColor(230,230,230,255), tupleToColor(120,180,220,255), tupleToColor(80,80,80,255), tupleToColor(255,255,255,255), 18, fontPath);
    editLayoutCb->setOnChange([panel](bool on){ panel->setEditable(on); std::cout << (on?"Edit ON":"Edit OFF") << std::endl; });
    // Make edit checkbox bypass callbacks so it always works
    editLayoutCb->setBypassCallbacks(true);

    auto* verticalBtn = uiManager->addElement<Button>(500, 560, 100, 32,
        "Vertical",
        [panel]() { panel->setCustomLayout(nullptr); panel->setLayoutVertical(20,20,12); },
        tupleToColor(90,120,190,255), tupleToColor(120,150,220,255), fontPath, tupleToColor(0,0,0,255), 4, 16);
    (void)verticalBtn;
    auto* gridBtn = uiManager->addElement<Button>(610, 560, 80, 32,
        "Grid",
        [panel]() { panel->setCustomLayout(nullptr); panel->setLayoutGrid(2, 20, 20, 12, 12); },
        tupleToColor(90,120,190,255), tupleToColor(120,150,220,255), fontPath, tupleToColor(0,0,0,255), 4, 16);
    (void)gridBtn;
    auto* customBtn = uiManager->addElement<Button>(700, 560, 90, 32,
        "Custom",
        [panel]() {
            panel->setCustomLayout([](UIPanel& p){
                // Simple custom layout: two columns with staggered heights
                const int x0 = p.rect.x + 20;
                const int y0 = p.rect.y + 20;
                const int colW = (p.rect.w - 40 - 12) / 2; // padding and gap
                int yLeft = y0;
                int yRight = y0;
                bool left = true;
                for (auto* c : p._debugGetChildren()) {
                    if (!c) continue;
                    // For controls with intrinsic width (e.g., labels), keep their natural size.
                    // For interactive controls, clamp width to column.
                    c->rect.w = std::min(c->rect.w, colW);
                    c->rect.h = std::min(c->rect.h, 60);
                    if (left) {
                        c->rect.x = x0;
                        c->rect.y = yLeft;
                        yLeft += c->rect.h + 12;
                    } else {
                        c->rect.x = x0 + colW + 12;
                        c->rect.y = yRight;
                        yRight += c->rect.h + 12;
                    }
                    left = !left;
                    c->onRectChanged();
                }
            });
        },
        tupleToColor(90,120,190,255), tupleToColor(120,150,220,255), fontPath, tupleToColor(0,0,0,255), 4, 16);
    (void)customBtn;

    // Global callbacks toggle — allows pausing UI callbacks for testing
    bool userOn = UIConfig::callbacksEnabledFlag();
    bool effectiveOn = UIConfig::areCallbacksEnabled();
    Label* cbState = uiManager->addElement<Label>(170, 530, std::string("Callbacks: ") + (effectiveOn ? "ON" : "OFF"),
        tupleToColor(180,180,180,255), 14, fontPath);
    Button* toggleCallbacks = uiManager->addElement<Button>(290, 526, 180, 28,
        userOn ? "Disable callbacks" : "Enable callbacks",
        [](){} /* set below */,
        tupleToColor(150,150,150,255), tupleToColor(180,180,180,255), fontPath, tupleToColor(0,0,0,255), 4, 14);
    toggleCallbacks->setBypassCallbackGate(true);
    toggleCallbacks->setCallback([cbState, toggleCallbacks]() {
        bool userNow = !UIConfig::callbacksEnabledFlag();
        UIConfig::setCallbacksEnabled(userNow);
        bool effective = UIConfig::areCallbacksEnabled();
        cbState->setText(std::string("Callbacks: ") + (effective ? "ON" : "OFF"));
        toggleCallbacks->setText(userNow ? "Disable callbacks" : "Enable callbacks");
    });

    uiManager->addElement<Label>(400 - 100, 570, "Refactored UI v1.1", tupleToColor(180,180,180,255), 14, fontPath);
}

static void createSecondMenu(UIManager* uiManager, std::string& pendingScreen, const std::string& fontPath) {
    uiManager->clearElements();
    // Background panel for the second screen
    uiManager->addElement<UIPanel>(150, 50, 500, 500, tupleToColor(40,30,30,220), tupleToColor(80,60,60,255), 2);
    uiManager->addElement<Label>(400 - 150, 80, "SECOND SCREEN", tupleToColor(255,255,255,255), 36, fontPath);

    const int buttonWidth = 300;
    const int buttonHeight = 50;
    const int startY = 180;
    const int spacing = 70;

    uiManager->addElement<Button>(
        400 - buttonWidth/2, startY,
        buttonWidth, buttonHeight,
        "Back to Main Menu",
        [&pendingScreen]() { std::cout << "Going back to main menu" << std::endl; pendingScreen = "main"; },
        tupleToColor(100, 100, 200, 255),
        tupleToColor(130, 130, 230, 255),
        fontPath,
        tupleToColor(0,0,0,255),
        6,
        24
    );

    uiManager->addElement<Button>(
        400 - buttonWidth/2, startY + spacing,
        buttonWidth, buttonHeight,
        "Another Test Button",
        [](){ std::cout << "Clicked test button on second screen" << std::endl; },
        tupleToColor(100, 200, 100, 255),
        tupleToColor(130, 230, 130, 255),
        fontPath,
        tupleToColor(0,0,0,255),
        6,
        24
    );
}
