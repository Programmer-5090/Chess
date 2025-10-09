// DEPRECATED: Legacy UIBuilder kept only for historical reference.
// Not included in builds; superseded by UIEnhancedBuilder integrated with UIPanel.
// This file will be removed in a future cleanup.
#ifndef UI_BUILDER_H
#define UI_BUILDER_H

#include "../manager.h"
#include "../controls/ui/uiLayoutManager.h"
#include "../controls/button.h"
#include "../controls/label.h"
#include "../controls/checkbox.h"
#include "../controls/slider.h"
#include "../controls/text_input.h"
#include "../controls/dropdown.h"
#include "../controls/dialog.h"
#include "../controls/ui/uiPanel.h"
#include <functional>
#include <string>

// Immediate-mode style builder for retained mode UI
// Provides cleaner, more declarative API while keeping object benefits

class UIBuilder {
private:
    UIManager* manager;
    UILayoutManager layoutManager;
    std::string defaultFontPath;

public:
    UIBuilder(UIManager* mgr, const std::string& fontPath = "assets/fonts/OpenSans-Regular.ttf");
    
    // Layout management
    void beginVerticalLayout(SDL_Rect bounds, int padding = 8);
    void beginHorizontalLayout(SDL_Rect bounds, int padding = 8);
    void endLayout();
    
    // Simplified element creation with automatic positioning
    Button* button(const std::string& text, std::function<void()> callback, 
                   int width = 200, int height = 40);
    
    Label* label(const std::string& text, SDL_Color color = {255,255,255,255}, 
                 int fontSize = 16);
    
    UICheckbox* checkbox(const std::string& text, bool checked = false,
                        std::function<void(bool)> onChange = nullptr);
    
    UISlider* slider(double minVal, double maxVal, double value,
                    int width = 200, std::function<void(double)> onChange = nullptr);
    
    UITextInput* textInput(const std::string& placeholder = "", int width = 200,
                          std::function<void(const std::string&)> onSubmit = nullptr);
    
    UIDropdown* dropdown(const std::vector<std::string>& options, int selectedIndex = 0,
                        int width = 200, std::function<void(int, const std::string&)> onChange = nullptr);
    
    UIDialog* dialog(const std::string& title, const std::string& message,
                    std::function<void()> onOk = nullptr, std::function<void()> onCancel = nullptr);
    
    UIPanel* panel(SDL_Rect rect, SDL_Color bg = {30,30,40,220});
    
    // Spacing and alignment helpers
    void spacing(int pixels = 8);
    void sameLine();  // Next element on same line (horizontal layout for one element)
    
    // Direct positioning (bypass layout)
    void setNextElementPos(int x, int y);
    
    // Clear all layouts and elements
    void clear();
};

#endif // UI_BUILDER_H
