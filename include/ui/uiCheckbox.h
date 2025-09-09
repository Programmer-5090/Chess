#ifndef UI_CHECKBOX_H
#define UI_CHECKBOX_H

#include "uiElement.h"

namespace UI {

/**
 * Checkbox UI element with label
 */
class Checkbox : public UIElement {
public:
    Checkbox(const SDL_FRect& rect, const std::string& label,
             const std::string& fontPath, int fontSize)
        : UIElement(rect), label(label), fontPath(fontPath), fontSize(fontSize),
          checked(false) {}
    
    ~Checkbox() override {
        cleanup();
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        
        // Lazy initialization of font and label texture
        if (!font) {
            font = FontManager::getInstance().getFont(fontPath, fontSize);
        }
        
        if (!labelTexture && font && !label.empty()) {
            labelTexture = renderTextToTexture(renderer, label, font, textColor);
        }
        
        // Calculate checkbox square dimensions and position
        float checkboxSize = rect.h * 0.8f;
        float checkboxX = rect.x;
        float checkboxY = rect.y + (rect.h - checkboxSize) / 2;
        
        SDL_FRect checkboxRect = {checkboxX, checkboxY, checkboxSize, checkboxSize};
        
        // Draw checkbox background
        SDL_Color bgColor;
        switch (state) {
            case UIState::NORMAL:
                bgColor = normalColor;
                break;
            case UIState::HOVER:
                bgColor = hoverColor;
                break;
            case UIState::ACTIVE:
                bgColor = activeColor;
                break;
            case UIState::DISABLED:
                bgColor = disabledColor;
                break;
        }
        
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRectF(renderer, &checkboxRect);
        
        // Draw checkbox border
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        SDL_RenderDrawRectF(renderer, &checkboxRect);
        
        // Draw checkmark if checked
        if (checked) {
            float padding = checkboxSize * 0.2f;
            
            // Draw a simple X as checkmark
            SDL_SetRenderDrawColor(renderer, checkmarkColor.r, checkmarkColor.g, 
                                  checkmarkColor.b, checkmarkColor.a);
            
            // Draw first diagonal line (top-left to bottom-right)
            SDL_RenderDrawLineF(renderer, 
                               checkboxX + padding, 
                               checkboxY + padding,
                               checkboxX + checkboxSize - padding, 
                               checkboxY + checkboxSize - padding);
            
            // Draw second diagonal line (bottom-left to top-right)
            SDL_RenderDrawLineF(renderer, 
                               checkboxX + padding, 
                               checkboxY + checkboxSize - padding,
                               checkboxX + checkboxSize - padding, 
                               checkboxY + padding);
        }
        
        // Render label text
        if (labelTexture) {
            // Adjust label position to be to the right of the checkbox
            SDL_FRect labelRect = {
                rect.x + checkboxSize + 10, // Add some spacing
                rect.y,
                rect.w - checkboxSize - 10,
                rect.h
            };
            
            renderTextureWithAlignment(renderer, labelTexture, labelRect, TextAlignment::LEFT);
        }
    }
    
    void update(Input& input) override {
        if (!visible || state == UIState::DISABLED) return;
        
        bool wasHovering = (state == UIState::HOVER || state == UIState::ACTIVE);
        bool hovering = containsPoint(input.getMouseX(), input.getMouseY());
        
        if (hovering) {
            if (input.isMouseButtonDown(SDL_BUTTON_LEFT)) {
                state = UIState::ACTIVE;
            } else {
                if (state == UIState::ACTIVE && input.isMouseButtonReleased(SDL_BUTTON_LEFT)) {
                    // Toggle checked state
                    checked = !checked;
                    if (onValueChanged) {
                        onValueChanged(checked);
                    }
                }
                state = UIState::HOVER;
                if (!wasHovering && onHover) {
                    onHover();
                }
            }
        } else {
            state = UIState::NORMAL;
            if (wasHovering && onLeave) {
                onLeave();
            }
        }
    }
    
    void cleanup() override {
        freeTexture(labelTexture);
    }
    
    // Setters & Getters
    void setChecked(bool value) {
        if (checked != value) {
            checked = value;
            if (onValueChanged) {
                onValueChanged(checked);
            }
        }
    }
    
    bool isChecked() const {
        return checked;
    }
    
    void setLabel(const std::string& newLabel, SDL_Renderer* renderer) {
        if (label != newLabel) {
            label = newLabel;
            freeTexture(labelTexture);
            if (font && !label.empty()) {
                labelTexture = renderTextToTexture(renderer, label, font, textColor);
            }
        }
    }
    
    void setOnValueChanged(std::function<void(bool)> callback) {
        onValueChanged = callback;
    }
    
    void setColors(const SDL_Color& normal, const SDL_Color& hover, 
                   const SDL_Color& active, const SDL_Color& disabled,
                   const SDL_Color& text, const SDL_Color& border,
                   const SDL_Color& checkmark) {
        normalColor = normal;
        hoverColor = hover;
        activeColor = active;
        disabledColor = disabled;
        textColor = text;
        borderColor = border;
        checkmarkColor = checkmark;
    }

private:
    std::string label;
    std::string fontPath;
    int fontSize;
    bool checked;
    
    TTF_Font* font = nullptr;
    SDL_Texture* labelTexture = nullptr;
    
    // Colors
    SDL_Color normalColor = {60, 60, 60, 255};
    SDL_Color hoverColor = {80, 80, 80, 255};
    SDL_Color activeColor = {40, 40, 40, 255};
    SDL_Color disabledColor = {30, 30, 30, 128};
    SDL_Color textColor = {240, 240, 240, 255};
    SDL_Color borderColor = {100, 100, 100, 255};
    SDL_Color checkmarkColor = {200, 200, 200, 255};
    
    // Callback
    std::function<void(bool)> onValueChanged;
};

} // namespace UI

#endif // UI_CHECKBOX_H
