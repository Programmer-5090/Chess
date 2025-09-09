#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "include/input.h"
#include "include/ui/uiManager.h"
#include "include/ui/uiButton.h"
#include "include/ui/uiLabel.h"
#include "include/ui/uiPanel.h"
#include "include/ui/uiCheckbox.h"
#include "include/ui/uiDropdown.h"
#include "include/ui/uiTextInput.h"
#include "include/ui/uiSlider.h"
#include "include/ui/uiDialog.h"

// Forward declarations
bool initSDL();
void cleanup();
void createMainMenu();
void createSettingsScreen();
void createStyleDemo();
void createFormDemo();
void handleEvents();
void update();
void render();

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool running = true;
Input input;
UI::UIManager uiManager;
std::string fontPath = "assets/fonts/OpenSans-Regular.ttf";  // You'll need to add this font
std::string currentScreen = "main";

int main(int argc, char* argv[]) {
    // Initialize SDL and create window/renderer
    if (!initSDL()) {
        std::cerr << "Failed to initialize!" << std::endl;
        return 1;
    }

    // Set up the initial UI
    createMainMenu();

    // Main game loop
    while (running) {
        handleEvents();
        update();
        render();

        // Cap to 60 FPS
        SDL_Delay(16);
    }

    // Clean up resources
    cleanup();
    return 0;
}

bool initSDL() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        return false;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
        return false;
    }

    // Create window
    window = SDL_CreateWindow(
        "UI Demo - Chess Game",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        800, 600,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

void cleanup() {
    uiManager.cleanup();
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Update input manager
        input.update();
        
        // Handle text input events
        uiManager.handleKeyEvent(event);
        
        if (event.type == SDL_QUIT) {
            running = false;
        }
    }
}

void update() {
    // Update the UI elements
    uiManager.update(input);
}

void render() {
    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderClear(renderer);
    
    // Render UI elements
    uiManager.render(renderer);
    
    // Present the renderer
    SDL_RenderPresent(renderer);
}

void createMainMenu() {
    // Clear any existing UI
    uiManager.clear();
    
    // Create a background panel
    auto panel = uiManager.createPanel({0, 0, 800, 600}, "backgroundPanel");
    panel->setBackgroundColor({20, 20, 30, 255});
    
    // Create a title label
    uiManager.createLabel({400, 80, 400, 60}, "UI ELEMENTS DEMO", fontPath, 36, UI::TextAlignment::CENTER, "titleLabel");
    
    // Create main menu buttons
    float buttonWidth = 300;
    float buttonHeight = 50;
    float startY = 180;
    float spacing = 70;
    
    auto demoButton = uiManager.createButton({400 - buttonWidth/2, startY, buttonWidth, buttonHeight}, 
                                           "UI Components Demo", fontPath, 24, []() {
        currentScreen = "style";
        createStyleDemo();
    }, "demoButton");
    
    auto formButton = uiManager.createButton({400 - buttonWidth/2, startY + spacing, buttonWidth, buttonHeight}, 
                                          "Form Example", fontPath, 24, []() {
        currentScreen = "form";
        createFormDemo();
    }, "formButton");
    
    auto settingsButton = uiManager.createButton({400 - buttonWidth/2, startY + spacing*2, buttonWidth, buttonHeight}, 
                                              "Settings Screen", fontPath, 24, []() {
        currentScreen = "settings";
        createSettingsScreen();
    }, "settingsButton");
    
    auto exitButton = uiManager.createButton({400 - buttonWidth/2, startY + spacing*3, buttonWidth, buttonHeight}, 
                                          "Exit", fontPath, 24, []() {
        running = false;
    }, "exitButton");
    
    // Create a version label
    uiManager.createLabel({400, 570, 800, 20}, "Chess Game UI Demo v1.0", fontPath, 14, 
                        UI::TextAlignment::CENTER, "versionLabel");
}

void createSettingsScreen() {
    // Clear any existing UI
    uiManager.clear();
    
    // Create a background panel
    auto panel = uiManager.createPanel({0, 0, 800, 600}, "backgroundPanel");
    panel->setBackgroundColor({20, 20, 30, 255});
    
    // Create a title
    uiManager.createLabel({400, 40, 400, 40}, "SETTINGS", fontPath, 32, 
                        UI::TextAlignment::CENTER, "titleLabel");
    
    // Create a settings panel
    auto settingsPanel = uiManager.createPanel({150, 100, 500, 400}, "settingsPanel");
    settingsPanel->setBackgroundColor({40, 40, 50, 230});
    settingsPanel->setDrawBorder(true);
    settingsPanel->setBorderColor({100, 100, 120, 255});
    
    // Sound settings
    uiManager.createLabel({200, 130, 200, 30}, "SOUND", fontPath, 20, 
                        UI::TextAlignment::LEFT, "soundLabel");
    
    auto sfxCheckbox = uiManager.createCheckbox({200, 170, 400, 30}, "Sound Effects", fontPath, 18, 
                                              [](bool checked) {
        std::cout << "Sound effects " << (checked ? "enabled" : "disabled") << std::endl;
    }, "sfxCheckbox");
    sfxCheckbox->setChecked(true);
    
    auto musicCheckbox = uiManager.createCheckbox({200, 210, 400, 30}, "Music", fontPath, 18, 
                                               [](bool checked) {
        std::cout << "Music " << (checked ? "enabled" : "disabled") << std::endl;
    }, "musicCheckbox");
    musicCheckbox->setChecked(true);
    
    // Volume slider
    uiManager.createLabel({200, 250, 200, 30}, "Volume", fontPath, 18, 
                        UI::TextAlignment::LEFT, "volumeLabel");
    
    auto volumeSlider = std::make_shared<UI::Slider>(SDL_FRect{200, 280, 400, 30}, 0.0f, 100.0f, 75.0f);
    volumeSlider->setValueFont(UI::FontManager::getInstance().getFont(fontPath, 16));
    volumeSlider->setOnValueChanged([](float value) {
        std::cout << "Volume set to: " << value << std::endl;
    });
    uiManager.addElement(volumeSlider, "volumeSlider");
    
    // Difficulty dropdown
    uiManager.createLabel({200, 330, 200, 30}, "Difficulty", fontPath, 18, 
                        UI::TextAlignment::LEFT, "difficultyLabel");
    
    std::vector<std::string> difficulties = {"Easy", "Medium", "Hard", "Grandmaster"};
    auto difficultyDropdown = uiManager.createDropdown({200, 360, 400, 30}, difficulties, fontPath, 16, 
                                                    [](int index) {
        std::cout << "Difficulty set to index: " << index << std::endl;
    }, "difficultyDropdown");
    difficultyDropdown->setSelectedIndex(1); // Medium by default
    
    // Buttons
    auto saveButton = uiManager.createButton({350, 440, 120, 40}, "Save", fontPath, 18, []() {
        std::cout << "Settings saved!" << std::endl;
        
        auto dialog = UI::createMessageDialog("Settings", "Your settings have been saved!", fontPath);
        dialog->setOnResult([](UI::Dialog::DialogResult result) {
            // Return to main menu after dialog is closed
            currentScreen = "main";
            createMainMenu();
        });
        uiManager.addElement(dialog, "messageDialog");
        dialog->show();
        
    }, "saveButton");
    
    auto cancelButton = uiManager.createButton({480, 440, 120, 40}, "Cancel", fontPath, 18, []() {
        // Go back to main menu
        currentScreen = "main";
        createMainMenu();
    }, "cancelButton");
    
    // Back button
    auto backButton = uiManager.createButton({50, 40, 100, 40}, "Back", fontPath, 18, []() {
        // Go back to main menu
        currentScreen = "main";
        createMainMenu();
    }, "backButton");
}

void createStyleDemo() {
    // Clear any existing UI
    uiManager.clear();
    
    // Create a background panel
    auto panel = uiManager.createPanel({0, 0, 800, 600}, "backgroundPanel");
    panel->setBackgroundColor({20, 20, 30, 255});
    
    // Create a title
    uiManager.createLabel({400, 40, 400, 40}, "UI COMPONENTS", fontPath, 32, 
                        UI::TextAlignment::CENTER, "titleLabel");
    
    // Section: Labels
    float sectionWidth = 360;
    float sectionHeight = 250;
    float margin = 20;
    
    // Top-left panel: Labels
    auto labelsPanel = uiManager.createPanel({margin, 100, sectionWidth, sectionHeight}, "labelsPanel");
    labelsPanel->setBackgroundColor({40, 40, 50, 230});
    labelsPanel->setDrawBorder(true);
    
    uiManager.createLabel({margin + sectionWidth/2, 110, 200, 30}, "Labels", fontPath, 20, 
                        UI::TextAlignment::CENTER, "labelTitle");
    
    uiManager.createLabel({margin + 20, 150, 320, 30}, "Left aligned label", fontPath, 16, 
                        UI::TextAlignment::LEFT, "leftLabel");
    
    uiManager.createLabel({margin + 20, 180, 320, 30}, "Center aligned label", fontPath, 16, 
                        UI::TextAlignment::CENTER, "centerLabel");
    
    uiManager.createLabel({margin + 20, 210, 320, 30}, "Right aligned label", fontPath, 16, 
                        UI::TextAlignment::RIGHT, "rightLabel");
    
    // Styled label with background
    auto styledLabel = uiManager.createLabel({margin + 20, 250, 320, 40}, "Styled Label with Background", fontPath, 18, 
                                          UI::TextAlignment::CENTER, "styledLabel");
    styledLabel->setDrawBackground(true);
    styledLabel->setBackgroundColor({60, 60, 100, 255});
    
    // Top-right panel: Buttons
    auto buttonsPanel = uiManager.createPanel({margin*2 + sectionWidth, 100, sectionWidth, sectionHeight}, "buttonsPanel");
    buttonsPanel->setBackgroundColor({40, 40, 50, 230});
    buttonsPanel->setDrawBorder(true);
    
    uiManager.createLabel({margin*2 + sectionWidth + sectionWidth/2, 110, 200, 30}, "Buttons", fontPath, 20, 
                        UI::TextAlignment::CENTER, "buttonTitle");
    
    // Regular button
    auto normalButton = uiManager.createButton({margin*2 + sectionWidth + 80, 150, 200, 40}, 
                                            "Normal Button", fontPath, 16, []() {
        std::cout << "Normal button clicked" << std::endl;
    }, "normalButton");
    
    // Styled button
    auto buttonStyle = UI::Button_Style();
    buttonStyle.bgColor = {60, 120, 60, 255};
    buttonStyle.hoverColor = {80, 140, 80, 255};
    buttonStyle.activeColor = {40, 100, 40, 255};
    
    // Disabled button
    auto disabledButton = uiManager.createButton({margin*2 + sectionWidth + 80, 250, 200, 40}, 
                                              "Disabled Button", fontPath, 16, []() {
        std::cout << "This should not be called" << std::endl;
    }, "disabledButton");
    disabledButton->setState(UI::UIState::DISABLED);
    
    // Bottom-left panel: Input controls
    auto inputPanel = uiManager.createPanel({margin, margin + 100 + sectionHeight, sectionWidth, sectionHeight}, "inputPanel");
    inputPanel->setBackgroundColor({40, 40, 50, 230});
    inputPanel->setDrawBorder(true);
    
    uiManager.createLabel({margin + sectionWidth/2, margin + 110 + sectionHeight, 200, 30}, "Input Controls", fontPath, 20, 
                        UI::TextAlignment::CENTER, "inputTitle");
    
    // Checkbox
    auto checkbox = uiManager.createCheckbox({margin + 20, margin + 150 + sectionHeight, 320, 30}, 
                                          "Sample Checkbox", fontPath, 16, [](bool checked) {
        std::cout << "Checkbox value: " << (checked ? "checked" : "unchecked") << std::endl;
    }, "sampleCheckbox");
    
    // Text input
    auto textInput = uiManager.createTextInput({margin + 20, margin + 190 + sectionHeight, 320, 30}, 
                                            fontPath, 16, "Enter text here...", [](const std::string& text) {
        std::cout << "Text input: " << text << std::endl;
    }, "sampleTextInput");
    
    // Slider
    auto slider = std::make_shared<UI::Slider>(
        SDL_FRect{margin + 20, margin + 240 + sectionHeight, 320, 30}, 
        0.0f, 10.0f, 5.0f
    );
    slider->setOnValueChanged([](float value) {
        std::cout << "Slider value: " << value << std::endl;
    });
    slider->setValueFont(UI::FontManager::getInstance().getFont(fontPath, 14));
    slider->setStep(0.5f);
    uiManager.addElement(slider, "sampleSlider");
    
    // Bottom-right panel: Complex controls
    auto complexPanel = uiManager.createPanel({margin*2 + sectionWidth, margin + 100 + sectionHeight, sectionWidth, sectionHeight}, "complexPanel");
    complexPanel->setBackgroundColor({40, 40, 50, 230});
    complexPanel->setDrawBorder(true);
    
    uiManager.createLabel({margin*2 + sectionWidth + sectionWidth/2, margin + 110 + sectionHeight, 200, 30}, 
                        "Complex Controls", fontPath, 20, UI::TextAlignment::CENTER, "complexTitle");
    
    // Dropdown
    std::vector<std::string> options = {"Option 1", "Option 2", "Option 3", "Option 4"};
    auto dropdown = uiManager.createDropdown({margin*2 + sectionWidth + 30, margin + 150 + sectionHeight, 300, 30}, 
                                          options, fontPath, 16, [](int index) {
        std::cout << "Selected dropdown index: " << index << std::endl;
    }, "sampleDropdown");
    
    // Dialog button
    auto dialogButton = uiManager.createButton({margin*2 + sectionWidth + 30, margin + 200 + sectionHeight, 300, 40}, 
                                            "Open Dialog", fontPath, 16, []() {
        auto dialog = UI::createYesNoDialog(
            "Confirmation", 
            "Do you want to see how dialogs work?", 
            fontPath
        );
        dialog->setOnResult([](UI::Dialog::DialogResult result) {
            if (result == UI::Dialog::DialogResult::YES) {
                std::cout << "User selected Yes" << std::endl;
                
                // Show a follow-up message dialog
                auto msgDialog = UI::createMessageDialog(
                    "Information", 
                    "This is a sample message dialog.\nYou can display multiple lines of text here.", 
                    fontPath
                );
                UI::UIManager::getInstance().addElement(msgDialog, "messageDialog");
                msgDialog->show();
            } else {
                std::cout << "User selected No" << std::endl;
            }
        });
        UI::UIManager::getInstance().addElement(dialog, "confirmDialog");
        dialog->show();
    }, "dialogButton");
    
    // Back button
    auto backButton = uiManager.createButton({50, 40, 100, 40}, "Back", fontPath, 18, []() {
        // Go back to main menu
        currentScreen = "main";
        createMainMenu();
    }, "backButton");
}

void createFormDemo() {
    // Clear any existing UI
    uiManager.clear();
    
    // Create a background panel
    auto panel = uiManager.createPanel({0, 0, 800, 600}, "backgroundPanel");
    panel->setBackgroundColor({20, 20, 30, 255});
    
    // Create a title
    uiManager.createLabel({400, 40, 400, 40}, "PLAYER PROFILE", fontPath, 32, 
                        UI::TextAlignment::CENTER, "titleLabel");
    
    // Create a form panel
    auto formPanel = uiManager.createPanel({150, 100, 500, 400}, "formPanel");
    formPanel->setBackgroundColor({40, 40, 50, 230});
    formPanel->setDrawBorder(true);
    formPanel->setBorderColor({100, 100, 120, 255});
    
    // Form fields
    float startY = 120;
    float spacing = 60;
    float labelWidth = 150;
    float inputWidth = 300;
    float fieldHeight = 30;
    
    // Name field
    uiManager.createLabel({200, startY, labelWidth, fieldHeight}, "Player Name:", fontPath, 18, 
                        UI::TextAlignment::LEFT, "nameLabel");
    
    auto nameInput = uiManager.createTextInput({200 + labelWidth, startY, inputWidth, fieldHeight}, 
                                            fontPath, 18, "Enter your name", [](const std::string& text) {
        std::cout << "Name: " << text << std::endl;
    }, "nameInput");
    
    // Email field
    uiManager.createLabel({200, startY + spacing, labelWidth, fieldHeight}, "Email:", fontPath, 18, 
                        UI::TextAlignment::LEFT, "emailLabel");
    
    auto emailInput = uiManager.createTextInput({200 + labelWidth, startY + spacing, inputWidth, fieldHeight}, 
                                             fontPath, 18, "Enter your email", [](const std::string& text) {
        std::cout << "Email: " << text << std::endl;
    }, "emailInput");
    
    // Age field
    uiManager.createLabel({200, startY + spacing*2, labelWidth, fieldHeight}, "Age:", fontPath, 18, 
                        UI::TextAlignment::LEFT, "ageLabel");
    
    auto ageSlider = std::make_shared<UI::Slider>(
        SDL_FRect{200 + labelWidth, startY + spacing*2, inputWidth, fieldHeight}, 
        8, 100, 25
    );
    ageSlider->setOnValueChanged([](float value) {
        std::cout << "Age: " << static_cast<int>(value) << std::endl;
    });
    ageSlider->setValueFont(UI::FontManager::getInstance().getFont(fontPath, 16));
    ageSlider->setStep(1.0f);
    ageSlider->setPrecision(0);
    uiManager.addElement(ageSlider, "ageSlider");
    
    // Experience level
    uiManager.createLabel({200, startY + spacing*3, labelWidth, fieldHeight}, "Experience:", fontPath, 18, 
                        UI::TextAlignment::LEFT, "expLabel");
    
    std::vector<std::string> expLevels = {"Beginner", "Intermediate", "Advanced", "Expert", "Grandmaster"};
    auto expDropdown = uiManager.createDropdown({200 + labelWidth, startY + spacing*3, inputWidth, fieldHeight}, 
                                             expLevels, fontPath, 16, [](int index) {
        std::cout << "Experience level index: " << index << std::endl;
    }, "expDropdown");
    
    // Newsletter subscription
    auto newsletterCheckbox = uiManager.createCheckbox({200, startY + spacing*4, inputWidth + labelWidth, fieldHeight}, 
                                                    "Subscribe to our newsletter", fontPath, 18, [](bool checked) {
        std::cout << "Newsletter subscription: " << (checked ? "yes" : "no") << std::endl;
    }, "newsletterCheckbox");
    
    // Form buttons
    auto submitButton = uiManager.createButton({350, startY + spacing*5, 120, 40}, "Submit", fontPath, 18, []() {
        std::cout << "Form submitted!" << std::endl;
        
        auto dialog = UI::createMessageDialog("Profile Saved", "Your player profile has been saved!", fontPath);
        dialog->setOnResult([](UI::Dialog::DialogResult result) {
            // Return to main menu after dialog is closed
            currentScreen = "main";
            createMainMenu();
        });
        uiManager.addElement(dialog, "messageDialog");
        dialog->show();
        
    }, "submitButton");
    
    auto clearButton = uiManager.createButton({480, startY + spacing*5, 120, 40}, "Clear", fontPath, 18, []() {
        // Clear all form fields
        auto nameInput = UI::UIManager::getInstance().getElementByIdAs<UI::TextInput>("nameInput");
        if (nameInput) nameInput->setText("");
        
        auto emailInput = UI::UIManager::getInstance().getElementByIdAs<UI::TextInput>("emailInput");
        if (emailInput) emailInput->setText("");
        
        auto ageSlider = UI::UIManager::getInstance().getElementByIdAs<UI::Slider>("ageSlider");
        if (ageSlider) ageSlider->setValue(25);
        
        auto expDropdown = UI::UIManager::getInstance().getElementByIdAs<UI::Dropdown>("expDropdown");
        if (expDropdown) expDropdown->setSelectedIndex(0);
        
        auto newsletterCheckbox = UI::UIManager::getInstance().getElementByIdAs<UI::Checkbox>("newsletterCheckbox");
        if (newsletterCheckbox) newsletterCheckbox->setChecked(false);
        
        std::cout << "Form cleared!" << std::endl;
    }, "clearButton");
    
    // Back button
    auto backButton = uiManager.createButton({50, 40, 100, 40}, "Back", fontPath, 18, []() {
        // Go back to main menu
        currentScreen = "main";
        createMainMenu();
    }, "backButton");
}
