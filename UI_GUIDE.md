# Chess Game UI System Guide

This document explains how to use the UI system in the Chess game project.

## Basic Usage

The UI system provides a set of components for building user interfaces in the Chess game. Here's how to get started:

```cpp
#include "ui/uiManager.h"
#include "ui/uiButton.h"
#include "ui/uiLabel.h"
#include "ui/uiPanel.h"
#include "ui/uiCheckbox.h"
#include "ui/uiDropdown.h"
#include "ui/uiTextInput.h"
#include "ui/uiSlider.h"
#include "ui/uiDialog.h"

// Create a UI manager
UI::UIManager uiManager;

// Font path for UI elements
std::string fontPath = "path/to/font.ttf";

void initUI(SDL_Renderer* renderer) {
    // Create a panel
    auto panel = uiManager.createPanel({100, 100, 400, 300}, "mainPanel");
    panel->setBackgroundColor({40, 40, 40, 220});
    panel->setDrawBorder(true);
    
    // Add a label to the panel
    auto label = uiManager.createLabel({120, 120, 360, 30}, "Chess Game", fontPath, 24, UI::TextAlignment::CENTER, "titleLabel");
    
    // Add a button
    auto button = uiManager.createButton({150, 180, 300, 40}, "Start Game", fontPath, 18, []() {
        // Button click handler
        std::cout << "Start Game clicked!" << std::endl;
    }, "startButton");
    
    // Add a checkbox
    auto checkbox = uiManager.createCheckbox({150, 240, 300, 30}, "Sound Enabled", fontPath, 16, [](bool checked) {
        // Checkbox value changed handler
        std::cout << "Sound " << (checked ? "enabled" : "disabled") << std::endl;
    }, "soundCheckbox");
    
    // Add a dropdown
    std::vector<std::string> difficulties = {"Easy", "Medium", "Hard"};
    auto dropdown = uiManager.createDropdown({150, 290, 300, 30}, difficulties, fontPath, 16, [](int index) {
        // Dropdown selection handler
        std::cout << "Difficulty changed to index: " << index << std::endl;
    }, "difficultyDropdown");
    
    // Add a slider for volume
    auto slider = std::make_shared<UI::Slider>(SDL_FRect{150, 340, 300, 30}, 0.0f, 100.0f, 50.0f);
    slider->setOnValueChanged([](float value) {
        std::cout << "Volume: " << value << std::endl;
    });
    slider->setValueFont(UI::FontManager::getInstance().getFont(fontPath, 14));
    uiManager.addElement(slider, "volumeSlider");
    
    // Add a text input
    auto textInput = uiManager.createTextInput({150, 390, 300, 30}, fontPath, 16, "Enter your name...", [](const std::string& text) {
        std::cout << "Name: " << text << std::endl;
    }, "nameInput");
}

void gameLoop() {
    // In your game loop:
    
    // Update UI with current input state
    uiManager.update(inputManager);
    
    // Render UI
    uiManager.render(renderer);
    
    // Handle text input events
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        uiManager.handleKeyEvent(event);
    }
}

// Show a dialog
void showDialog() {
    auto dialog = UI::createMessageDialog("Information", "Your move has been made.", fontPath);
    dialog->setOnResult([](UI::Dialog::DialogResult result) {
        std::cout << "Dialog closed with result: " << static_cast<int>(result) << std::endl;
    });
    uiManager.addElement(dialog, "messageDialog");
    dialog->show();
}
```

## UI Components

### UIElement (Base Class)

All UI components inherit from `UIElement` which provides:

- Basic position and size management
- Event handling (click, hover, leave)
- Visibility control
- State management (normal, hover, active, disabled)

### Button

Buttons trigger actions when clicked.

```cpp
auto button = uiManager.createButton({x, y, width, height}, "Label", fontPath, fontSize, clickCallback);
button->setEnabled(true); // Enable or disable the button
```

### Label

Labels display text without interaction.

```cpp
auto label = uiManager.createLabel({x, y, width, height}, "Text", fontPath, fontSize, alignment);
label->setText("New Text", renderer); // Update the text
```

### Panel

Panels serve as containers for other UI elements.

```cpp
auto panel = uiManager.createPanel({x, y, width, height});
panel->setBackgroundColor({40, 40, 40, 220});
panel->setDrawBorder(true);
```

### Checkbox

Checkboxes allow toggling boolean options.

```cpp
auto checkbox = uiManager.createCheckbox({x, y, width, height}, "Label", fontPath, fontSize, valueChangedCallback);
checkbox->setChecked(true); // Set the initial state
```

### Dropdown

Dropdowns provide selection from a list of options.

```cpp
std::vector<std::string> options = {"Option 1", "Option 2", "Option 3"};
auto dropdown = uiManager.createDropdown({x, y, width, height}, options, fontPath, fontSize, selectionChangedCallback);
dropdown->setSelectedIndex(1); // Select "Option 2"
```

### TextInput

Text inputs allow users to enter text.

```cpp
auto textInput = uiManager.createTextInput({x, y, width, height}, fontPath, fontSize, "Placeholder", textChangedCallback);
textInput->setText("Initial Value");
textInput->setMaxLength(50);
```

### Slider

Sliders allow selection of a value within a range.

```cpp
auto slider = std::make_shared<UI::Slider>(SDL_FRect{x, y, width, height}, minValue, maxValue, initialValue);
slider->setOnValueChanged(valueChangedCallback);
slider->setPrecision(1); // Set number of decimal places to display
uiManager.addElement(slider, "slider");
```

### Dialog

Dialogs display modal messages and prompts.

```cpp
// Message dialog (OK button)
auto messageDialog = UI::createMessageDialog("Title", "Message", fontPath);
uiManager.addElement(messageDialog, "messageDialog");
messageDialog->show();

// Confirmation dialog (OK/Cancel buttons)
auto confirmDialog = UI::createConfirmDialog("Confirm", "Are you sure?", fontPath);
confirmDialog->setOnResult([](UI::Dialog::DialogResult result) {
    if (result == UI::Dialog::DialogResult::OK) {
        // User confirmed
    }
});
uiManager.addElement(confirmDialog, "confirmDialog");
confirmDialog->show();

// Yes/No dialog
auto yesNoDialog = UI::createYesNoDialog("Question", "Do you want to save?", fontPath);
yesNoDialog->setOnResult([](UI::Dialog::DialogResult result) {
    if (result == UI::Dialog::DialogResult::YES) {
        // User selected "Yes"
    }
});
uiManager.addElement(yesNoDialog, "yesNoDialog");
yesNoDialog->show();
```

## Best Practices

1. **Resource Management**: The `FontManager` class manages fonts to prevent duplicate loading. Use it for all font operations.

2. **Memory Management**: Use the UI manager to manage UI elements. It will ensure proper cleanup.

3. **Event Flow**: The update method in each component handles mouse interaction, while text input requires explicit event handling.

4. **Layering**: Elements are rendered in the order they are added to the UI manager.

5. **Organization**: Group related UI elements into panels for easier management.

6. **Callbacks**: Use lambdas or std::function for UI event callbacks to keep code organized.

7. **Cleanup**: Call the cleanup method when you're done with UI elements to free resources.

## Example Screens

### Main Menu
```cpp
void createMainMenu() {
    auto panel = uiManager.createPanel({100, 100, 600, 400}, "mainMenuPanel");
    uiManager.createLabel({350, 130, 100, 40}, "CHESS", fontPath, 32, UI::TextAlignment::CENTER, "titleLabel");
    
    uiManager.createButton({300, 200, 200, 40}, "New Game", fontPath, 18, []() {
        // Start new game
    }, "newGameButton");
    
    uiManager.createButton({300, 250, 200, 40}, "Load Game", fontPath, 18, []() {
        // Load game
    }, "loadGameButton");
    
    uiManager.createButton({300, 300, 200, 40}, "Settings", fontPath, 18, []() {
        // Show settings
    }, "settingsButton");
    
    uiManager.createButton({300, 350, 200, 40}, "Exit", fontPath, 18, []() {
        // Exit game
    }, "exitButton");
}
```

### Settings Screen
```cpp
void createSettingsScreen() {
    auto panel = uiManager.createPanel({100, 100, 600, 400}, "settingsPanel");
    uiManager.createLabel({350, 130, 100, 40}, "Settings", fontPath, 24, UI::TextAlignment::CENTER, "settingsLabel");
    
    uiManager.createCheckbox({150, 180, 300, 30}, "Sound Effects", fontPath, 16, nullptr, "sfxCheckbox");
    uiManager.createCheckbox({150, 220, 300, 30}, "Music", fontPath, 16, nullptr, "musicCheckbox");
    
    auto slider = std::make_shared<UI::Slider>(SDL_FRect{150, 260, 300, 30}, 0.0f, 100.0f, 50.0f);
    slider->setValueFont(UI::FontManager::getInstance().getFont(fontPath, 14));
    uiManager.addElement(slider, "volumeSlider");
    
    std::vector<std::string> difficulties = {"Easy", "Medium", "Hard"};
    uiManager.createDropdown({150, 300, 300, 30}, difficulties, fontPath, 16, nullptr, "difficultyDropdown");
    
    uiManager.createButton({350, 380, 100, 30}, "Save", fontPath, 16, []() {
        // Save settings
    }, "saveButton");
    
    uiManager.createButton({460, 380, 100, 30}, "Cancel", fontPath, 16, []() {
        // Cancel settings
    }, "cancelButton");
}
```

## Conclusion

This UI system provides a complete set of components for building interfaces in the Chess game. By using these components, you can create consistent and interactive user interfaces that integrate well with the game's visual style.
