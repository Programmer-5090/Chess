#ifndef UI_ENHANCED_BUILDER_H
#define UI_ENHANCED_BUILDER_H

#include "../manager.h"
#include "../controls/ui/uiPanel.h"
#include "../controls/button.h"
#include "../controls/label.h"
#include "../controls/checkbox.h"
#include "../controls/slider.h"
#include "../controls/text_input.h"
#include "../controls/dropdown.h"
#include "../controls/dialog.h"
#include <functional>
#include <string>
#include <stack>

// Enhanced UIBuilder that integrates with UIPanel's layout system.
// Provides:
// - Panel-aware begin/end for vertical, horizontal (with wrapping), and grid layouts
// - Wrapped labels using TTF_SizeText measurement
// - Precise available-width calculation based on panel padding
// - Simple spacing/separator elements without abusing text labels

class UIEnhancedBuilder {
private:
    UIManager* manager;
    std::string defaultFontPath;
    std::stack<UIPanel*> panelStack;  // Track nested panels
    
public:
    UIEnhancedBuilder(UIManager* mgr, const std::string& fontPath = "assets/fonts/OpenSans-Regular.ttf");
    
    // Panel-based layout management (integrates with existing UIPanel)
    UIPanel* beginPanel(SDL_Rect rect, SDL_Color bg = {30,30,40,220});
    UIPanel* beginVerticalPanel(SDL_Rect rect, int padding = 15, int spacing = 8, SDL_Color bg = {30,30,40,220});
    UIPanel* beginHorizontalPanel(SDL_Rect rect, int padding = 15, int spacing = 8, SDL_Color bg = {30,30,40,220});
    UIPanel* beginGridPanel(SDL_Rect rect, int columns, int padding = 15, int spacing = 8, SDL_Color bg = {30,30,40,220});
    void endPanel();
    
    // Current panel access
    UIPanel* getCurrentPanel();
    bool hasActivePanel() const { return !panelStack.empty(); }
    
    // Enhanced element creation with automatic sizing and text wrapping
    Button* button(const std::string& text, std::function<void()> callback, 
                   int width = -1, int height = 40);  // -1 = auto-width
    
    Label* label(const std::string& text, SDL_Color color = {255,255,255,255}, 
                 int fontSize = 16, int maxWidth = -1);  // -1 = no wrap
    
    Label* wrappedLabel(const std::string& text, int maxWidth, SDL_Color color = {255,255,255,255}, 
                       int fontSize = 16);
    
    UICheckbox* checkbox(const std::string& text, bool checked = false,
                        std::function<void(bool)> onChange = nullptr);
    
    UISlider* slider(double minVal, double maxVal, double value,
                    int width = -1, std::function<void(double)> onChange = nullptr);  // -1 = fill width
    
    UITextInput* textInput(const std::string& placeholder = "", int width = -1,
                          std::function<void(const std::string&)> onSubmit = nullptr);
    
    UIDropdown* dropdown(const std::vector<std::string>& options, int selectedIndex = 0,
                        int width = -1, std::function<void(int, const std::string&)> onChange = nullptr);
    
    UIDialog* dialog(const std::string& title, const std::string& message,
                    std::function<void()> onOk = nullptr, std::function<void()> onCancel = nullptr);
    
    // Layout helpers
    void spacing(int pixels = 8);
    void separator(int height = 1, SDL_Color color = {100,100,100,255});
    
    // Responsive sizing helpers
    int getAvailableWidth();  // Width available in current panel
    int getDefaultButtonWidth();  // Smart button width based on available space
    
    // Clear all panels and elements
    void clear();
    
private:
    // Helper for auto-sizing elements within panels
    SDL_Rect getElementRect(int preferredWidth, int height);
    
    // Text wrapping helper
    std::vector<std::string> wrapText(const std::string& text, int maxWidth, TTF_Font* font);
};

#endif // UI_ENHANCED_BUILDER_H
