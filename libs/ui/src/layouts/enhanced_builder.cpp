#include "chess/ui/layouts/enhanced_builder.h"
#include "chess/ui/controls/ui/uiCommon.h"
#include <iostream>
#include <sstream>
#include <SDL_ttf.h>

UIEnhancedBuilder::UIEnhancedBuilder(UIManager* mgr, const std::string& fontPath)
    : manager(mgr), defaultFontPath(fontPath) {
}

UIPanel* UIEnhancedBuilder::beginPanel(SDL_Rect rect, SDL_Color bg) {
    UIPanel* panel = manager->addElement<UIPanel>(rect.x, rect.y, rect.w, rect.h, bg);
    panelStack.push(panel);
    return panel;
}

UIPanel* UIEnhancedBuilder::beginVerticalPanel(SDL_Rect rect, int padding, int spacing, SDL_Color bg) {
    UIPanel* panel = beginPanel(rect, bg);
    panel->setLayoutVertical(padding, padding, spacing);
    return panel;
}

UIPanel* UIEnhancedBuilder::beginHorizontalPanel(SDL_Rect rect, int padding, int spacing, SDL_Color bg) {
    UIPanel* panel = beginPanel(rect, bg);
    // Use true horizontal layout (with wrapping)
    panel->setLayoutHorizontal(padding, padding, spacing, spacing);
    return panel;
}

UIPanel* UIEnhancedBuilder::beginGridPanel(SDL_Rect rect, int columns, int padding, int spacing, SDL_Color bg) {
    UIPanel* panel = beginPanel(rect, bg);
    panel->setLayoutGrid(columns, padding, padding, spacing, spacing);
    return panel;
}

void UIEnhancedBuilder::endPanel() {
    if (!panelStack.empty()) {
        panelStack.pop();
    }
}

UIPanel* UIEnhancedBuilder::getCurrentPanel() {
    return panelStack.empty() ? nullptr : panelStack.top();
}

Button* UIEnhancedBuilder::button(const std::string& text, std::function<void()> callback, 
                                 int width, int height) {
    UIPanel* panel = getCurrentPanel();
    if (!panel) {
        // No active panel, create at origin
        return manager->addElement<Button>(0, 0, width > 0 ? width : 200, height, text, callback);
    }
    
    // Auto-size width if not specified
    if (width <= 0) {
        width = getDefaultButtonWidth();
    }
    
    Button* btn = panel->addChild<Button>(0, 0, width, height, text, callback,
        tupleToColor(100, 150, 200, 255), // default blue
        tupleToColor(130, 180, 230, 255), // hover
        defaultFontPath,
        tupleToColor(255, 255, 255, 255), // white text
        4, 20);
    
    return btn;
}

Label* UIEnhancedBuilder::label(const std::string& text, SDL_Color color, int fontSize, int maxWidth) {
    UIPanel* panel = getCurrentPanel();
    if (!panel) {
        // No active panel, create at origin
        return manager->addElement<Label>(0, 0, text, color, fontSize, defaultFontPath);
    }
    
    if (maxWidth > 0) {
        // Use wrapped label for width-constrained text
        return wrappedLabel(text, maxWidth, color, fontSize);
    }
    
    Label* lbl = panel->addChild<Label>(0, 0, text, color, fontSize, defaultFontPath);
    return lbl;
}

Label* UIEnhancedBuilder::wrappedLabel(const std::string& text, int maxWidth, SDL_Color color, int fontSize) {
    UIPanel* panel = getCurrentPanel();
    
    // Load font to measure text
    TTF_Font* font = TTF_OpenFont(defaultFontPath.c_str(), fontSize);
    if (!font) {
        // Fallback to regular label if font loading fails
        return label(text, color, fontSize);
    }
    
    std::vector<std::string> lines = wrapText(text, maxWidth, font);
    TTF_CloseFont(font);
    
    // Create multiple labels for wrapped text
    Label* firstLabel = nullptr;
    for (const std::string& line : lines) {
        Label* lbl;
        if (panel) {
            lbl = panel->addChild<Label>(0, 0, line, color, fontSize, defaultFontPath);
        } else {
            lbl = manager->addElement<Label>(0, 0, line, color, fontSize, defaultFontPath);
        }
        
        if (!firstLabel) {
            firstLabel = lbl;
        }
    }
    
    return firstLabel;  // Return first label as reference
}

UICheckbox* UIEnhancedBuilder::checkbox(const std::string& text, bool checked,
                                       std::function<void(bool)> onChange) {
    UIPanel* panel = getCurrentPanel();
    UICheckbox* cb;
    
    if (panel) {
        cb = panel->addChild<UICheckbox>(0, 0, 24, text, checked,
            tupleToColor(220, 220, 220, 255), // box
            tupleToColor(60, 180, 75, 255),   // check
            tupleToColor(80, 80, 80, 255),    // border
            tupleToColor(255, 255, 255, 255), // text
            18, defaultFontPath);
    } else {
        cb = manager->addElement<UICheckbox>(0, 0, 24, text, checked,
            tupleToColor(220, 220, 220, 255),
            tupleToColor(60, 180, 75, 255),
            tupleToColor(80, 80, 80, 255),
            tupleToColor(255, 255, 255, 255), 18, defaultFontPath);
    }
    
    if (onChange) {
        cb->setOnChange(onChange);
    }
    
    return cb;
}

UISlider* UIEnhancedBuilder::slider(double minVal, double maxVal, double value,
                                   int width, std::function<void(double)> onChange) {
    UIPanel* panel = getCurrentPanel();
    
    if (width <= 0) {
        width = getAvailableWidth();
    }
    
    UISlider* s;
    if (panel) {
        s = panel->addChild<UISlider>(0, 0, width, 24, minVal, maxVal, value);
    } else {
        s = manager->addElement<UISlider>(0, 0, width, 24, minVal, maxVal, value);
    }
    
    if (onChange) {
        s->setOnChange(onChange);
    }
    
    return s;
}

UITextInput* UIEnhancedBuilder::textInput(const std::string& placeholder, int width,
                                         std::function<void(const std::string&)> onSubmit) {
    UIPanel* panel = getCurrentPanel();
    
    if (width <= 0) {
        width = getAvailableWidth();
    }
    
    UITextInput* ti;
    if (panel) {
        ti = panel->addChild<UITextInput>(0, 0, width, 32, placeholder, defaultFontPath);
    } else {
        ti = manager->addElement<UITextInput>(0, 0, width, 32, placeholder, defaultFontPath);
    }
    
    if (onSubmit) {
        ti->setOnSubmit(onSubmit);
    }
    
    return ti;
}

UIDropdown* UIEnhancedBuilder::dropdown(const std::vector<std::string>& options, int selectedIndex,
                                       int width, std::function<void(int, const std::string&)> onChange) {
    UIPanel* panel = getCurrentPanel();
    
    if (width <= 0) {
        width = getAvailableWidth();
    }
    
    UIDropdown* dd;
    if (panel) {
        dd = panel->addChild<UIDropdown>(0, 0, width, 32, options, selectedIndex, defaultFontPath);
    } else {
        dd = manager->addElement<UIDropdown>(0, 0, width, 32, options, selectedIndex, defaultFontPath);
    }
    
    if (onChange) {
        dd->setOnChange(onChange);
    }
    
    return dd;
}

UIDialog* UIEnhancedBuilder::dialog(const std::string& title, const std::string& message,
                                   std::function<void()> onOk, std::function<void()> onCancel) {
    UIDialog* dlg = manager->addElement<UIDialog>(
        200, 160, 400, 220, title, message,
        "OK", "Cancel", defaultFontPath
    );
    
    if (onOk) {
        dlg->setOnOk([dlg, onOk](){ onOk(); dlg->visible = false; });
    } else {
        dlg->setOnOk([dlg](){ dlg->visible = false; });
    }
    
    if (onCancel) {
        dlg->setOnCancel([dlg, onCancel](){ onCancel(); dlg->visible = false; });
    } else {
        dlg->setOnCancel([dlg](){ dlg->visible = false; });
    }
    
    return dlg;
}

void UIEnhancedBuilder::spacing(int pixels) {
    // Add invisible spacer element to current panel without rendering text
    struct Spacer : public UIElement {
        explicit Spacer(int h) : UIElement(0, 0, 1, h) {}
        void render(SDL_Renderer* /*renderer*/) override {}
    };
    UIPanel* panel = getCurrentPanel();
    if (panel) {
        panel->addChild<Spacer>(pixels);
    } else {
        manager->addElement<Spacer>(pixels);
    }
}

void UIEnhancedBuilder::separator(int height, SDL_Color color) {
    // Draw a simple colored rectangle as a separator without using text
    struct Separator : public UIElement {
        SDL_Color color;
        explicit Separator(int w, int h, SDL_Color c) : UIElement(0, 0, w, h), color(c) {}
        void render(SDL_Renderer* renderer) override {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(renderer, &rect);
        }
    };
    UIPanel* panel = getCurrentPanel();
    int w = getAvailableWidth();
    if (panel) {
        panel->addChild<Separator>(w, height, color);
    } else {
        manager->addElement<Separator>(w, height, color);
    }
}

int UIEnhancedBuilder::getAvailableWidth() {
    UIPanel* panel = getCurrentPanel();
    if (!panel) return 400;

    const int inner = panel->rect.w - 2 * panel->getPaddingX();
    return std::max(0, inner);
}

int UIEnhancedBuilder::getDefaultButtonWidth() {
    int availableWidth = getAvailableWidth();
    return std::min(availableWidth, 300);
}

void UIEnhancedBuilder::clear() {
    while (!panelStack.empty()) {
        panelStack.pop();
    }
    manager->clearElements();
}

SDL_Rect UIEnhancedBuilder::getElementRect(int preferredWidth, int height) {
    UIPanel* panel = getCurrentPanel();
    if (!panel) {
        return {0, 0, preferredWidth, height};
    }
    
    return {0, 0, preferredWidth, height};
}

std::vector<std::string> UIEnhancedBuilder::wrapText(const std::string& text, int maxWidth, TTF_Font* font) {
    std::vector<std::string> lines;
    
    if (text.empty() || !font) {
        lines.push_back(" ");
        return lines;
    }
    
    std::istringstream stream(text);
    std::string word;
    std::string currentLine;
    
    while (stream >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        
        int textWidth;
        if (TTF_SizeText(font, testLine.c_str(), &textWidth, nullptr) == 0) {
            if (textWidth <= maxWidth) {
                currentLine = testLine;
            } else {
                if (!currentLine.empty()) {
                    lines.push_back(currentLine);
                    currentLine = word;
                } else {
                    lines.push_back(word);
                    currentLine.clear();
                }
            }
        }
    }
    
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    if (lines.empty()) {
        lines.push_back(" ");
    }
    
    return lines;
}
