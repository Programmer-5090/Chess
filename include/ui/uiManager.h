#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "uiElement.h"
#include "uiButton.h"
#include "uiLabel.h"
#include "uiPanel.h"
#include "uiDropdown.h"
#include "uiCheckbox.h"
#include "uiTextInput.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

namespace UI {

/**
 * UIManager class to manage all UI elements
 */
class UIManager {
public:
    UIManager() = default;
    ~UIManager() {
        cleanup();
    }
    
    // Singleton access
    static UIManager& getInstance() {
        static UIManager instance;
        return instance;
    }
    
    // Add a UI element with an optional ID
    void addElement(std::shared_ptr<UIElement> element, const std::string& id = "") {
        if (!element) return;
        
        elements.push_back(element);
        
        // Store element with ID for later lookup if ID is provided
        if (!id.empty()) {
            elementMap[id] = element;
        }
    }
    
    // Remove a UI element
    void removeElement(std::shared_ptr<UIElement> element) {
        auto it = std::find(elements.begin(), elements.end(), element);
        if (it != elements.end()) {
            // Remove from ID map if present
            for (auto mapIt = elementMap.begin(); mapIt != elementMap.end(); ++mapIt) {
                if (mapIt->second == element) {
                    elementMap.erase(mapIt);
                    break;
                }
            }
            
            (*it)->cleanup();
            elements.erase(it);
        }
    }
    
    // Remove a UI element by ID
    void removeElementById(const std::string& id) {
        auto it = elementMap.find(id);
        if (it != elementMap.end()) {
            auto element = it->second;
            elementMap.erase(it);
            
            auto vecIt = std::find(elements.begin(), elements.end(), element);
            if (vecIt != elements.end()) {
                (*vecIt)->cleanup();
                elements.erase(vecIt);
            }
        }
    }
    
    // Get a UI element by ID
    std::shared_ptr<UIElement> getElementById(const std::string& id) {
        auto it = elementMap.find(id);
        if (it != elementMap.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // Template function to get a UI element by ID with specific type
    template<typename T>
    std::shared_ptr<T> getElementByIdAs(const std::string& id) {
        auto element = getElementById(id);
        if (element) {
            return std::dynamic_pointer_cast<T>(element);
        }
        return nullptr;
    }
    
    // Clear all UI elements
    void clear() {
        cleanup();
        elements.clear();
        elementMap.clear();
    }
    
    // Render all UI elements
    void render(SDL_Renderer* renderer) {
        for (auto& element : elements) {
            if (element) {
                element->render(renderer);
            }
        }
    }
    
    // Update all UI elements
    void update(Input& input) {
        for (auto& element : elements) {
            if (element) {
                element->update(input);
            }
        }
    }
    
    // Handle key events for text inputs and other elements that need key events
    void handleKeyEvent(const SDL_Event& event) {
        for (auto& element : elements) {
            if (element) {
                auto textInput = std::dynamic_pointer_cast<TextInput>(element);
                if (textInput && textInput->isFocused()) {
                    textInput->handleKeyEvent(event);
                }
            }
        }
    }
    
    // Cleanup all UI elements
    void cleanup() {
        for (auto& element : elements) {
            if (element) {
                element->cleanup();
            }
        }
    }
    
    // Helper functions to create common UI elements
    
    // Create a button
    std::shared_ptr<Button> createButton(const SDL_FRect& rect, const std::string& label,
                                        const std::string& fontPath, int fontSize,
                                        std::function<void()> onClick = nullptr,
                                        const std::string& id = "") {
        auto button = std::make_shared<Button>(rect, label, fontPath, fontSize);
        if (onClick) {
            button->onClick = onClick;
        }
        addElement(button, id);
        return button;
    }
    
    // Create a label
    std::shared_ptr<Label> createLabel(const SDL_FRect& rect, const std::string& text,
                                     const std::string& fontPath, int fontSize,
                                     TextAlignment alignment = TextAlignment::CENTER,
                                     const std::string& id = "") {
        auto label = std::make_shared<Label>(rect, text, fontPath, fontSize);
        label->setAlignment(alignment);
        addElement(label, id);
        return label;
    }
    
    // Create a panel
    std::shared_ptr<Panel> createPanel(const SDL_FRect& rect, const std::string& id = "") {
        auto panel = std::make_shared<Panel>(rect);
        addElement(panel, id);
        return panel;
    }
    
    // Create a dropdown
    std::shared_ptr<Dropdown> createDropdown(const SDL_FRect& rect, 
                                          const std::vector<std::string>& items,
                                          const std::string& fontPath, int fontSize,
                                          std::function<void(int)> onSelectionChanged = nullptr,
                                          const std::string& id = "") {
        auto dropdown = std::make_shared<Dropdown>(rect, items, fontPath, fontSize);
        if (onSelectionChanged) {
            dropdown->setOnSelectionChanged(onSelectionChanged);
        }
        addElement(dropdown, id);
        return dropdown;
    }
    
    // Create a checkbox
    std::shared_ptr<Checkbox> createCheckbox(const SDL_FRect& rect, const std::string& label,
                                          const std::string& fontPath, int fontSize,
                                          std::function<void(bool)> onValueChanged = nullptr,
                                          const std::string& id = "") {
        auto checkbox = std::make_shared<Checkbox>(rect, label, fontPath, fontSize);
        if (onValueChanged) {
            checkbox->setOnValueChanged(onValueChanged);
        }
        addElement(checkbox, id);
        return checkbox;
    }
    
    // Create a text input
    std::shared_ptr<TextInput> createTextInput(const SDL_FRect& rect,
                                            const std::string& fontPath, int fontSize,
                                            const std::string& placeholder = "Enter text...",
                                            std::function<void(const std::string&)> onTextChanged = nullptr,
                                            const std::string& id = "") {
        auto textInput = std::make_shared<TextInput>(rect, fontPath, fontSize);
        textInput->setPlaceholder(placeholder);
        if (onTextChanged) {
            textInput->setOnTextChanged(onTextChanged);
        }
        addElement(textInput, id);
        return textInput;
    }

private:
    std::vector<std::shared_ptr<UIElement>> elements;
    std::unordered_map<std::string, std::shared_ptr<UIElement>> elementMap;
};

} // namespace UI

#endif // UI_MANAGER_H
