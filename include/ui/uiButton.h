#ifndef UIBUTTON_H
#define UIBUTTON_H

#include "uiElement.h"

namespace UI {

struct Button_Style {
    SDL_Color bgColor = {60, 60, 60, 255};
    SDL_Color bottomColor = {40, 40, 40, 255};
    SDL_Color hoverColor = {80, 80, 80, 255};
    SDL_Color activeColor = {40, 40, 40, 255};
    SDL_Color textColor = {240, 240, 240, 255};
    int fontSize = 16;
    SDL_Color borderColor = {100, 100, 100, 255};
    int borderThickness = 1;
    int elevation = 3;
    int dynamicElevation = 2;
};

/**
 * Button UI element with text and elevation effect
 */
class Button : public UIElement {
public:
    Button(const SDL_FRect& rect, const std::string& label, 
           const std::string& fontPath, int fontSize, const Button_Style& style = Button_Style())
        : UIElement(rect), label(label), fontPath(fontPath), fontSize(fontSize), style(style) {}
    
    ~Button() override {
        cleanup();
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        
        // Lazy initialization of font and texture
        if (!font) {
            font = FontManager::getInstance().getFont(fontPath, fontSize);
        }
        
        if (!texture && font) {
            updateTextTexture(renderer);
        }
        
        // Get current elevation based on button state
        int currentElevation = style.elevation;
        if (state == UIState::ACTIVE) {
            currentElevation = style.elevation - style.dynamicElevation;
        }
        
        // Draw shadow/bottom color (if elevation > 0)
        if (currentElevation > 0) {
            SDL_FRect bottomRect = {
                rect.x,
                rect.y + currentElevation,
                rect.w,
                rect.h
            };
            
            SDL_SetRenderDrawColor(renderer, style.bottomColor.r, style.bottomColor.g, 
                                  style.bottomColor.b, style.bottomColor.a);
            SDL_RenderFillRectF(renderer, &bottomRect);
        }
        
        // Draw button top with offset based on state
        SDL_FRect topRect = {
            rect.x,
            rect.y + (state == UIState::ACTIVE ? style.dynamicElevation : 0),
            rect.w,
            rect.h - currentElevation
        };
        
        // Choose color based on state
        SDL_Color bgColor;
        switch (state) {
            case UIState::NORMAL:
                bgColor = style.bgColor;
                break;
            case UIState::HOVER:
                bgColor = style.hoverColor;
                break;
            case UIState::ACTIVE:
                bgColor = style.activeColor;
                break;
            case UIState::DISABLED:
                // Use bg color with reduced alpha for disabled state
                bgColor = style.bgColor;
                bgColor.a = 128;
                break;
        }
        
        // Draw button background
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRectF(renderer, &topRect);
        
        // Draw button border
        if (style.borderThickness > 0) {
            SDL_SetRenderDrawColor(renderer, style.borderColor.r, style.borderColor.g, 
                                  style.borderColor.b, style.borderColor.a);
            SDL_RenderDrawRectF(renderer, &topRect);
        }
        
        // Render text with appropriate offset
        if (texture) {
            SDL_FRect textRect = topRect;
            renderTextureWithAlignment(renderer, texture, textRect);
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
                    // Trigger click event
                    if (onClick) onClick();
                }
                state = UIState::HOVER;
                if (!wasHovering && onHover) onHover();
            }
        } else {
            state = UIState::NORMAL;
            if (wasHovering && onLeave) onLeave();
        }
    }
    
    void cleanup() override {
        freeTexture(texture);
    }
    
    // Setters
    void setLabel(const std::string& newLabel, SDL_Renderer* renderer) {
        label = newLabel;
        // Recreate texture with new text
        freeTexture(texture);
        updateTextTexture(renderer);
    }
    
    void setStyle(const Button_Style& newStyle) {
        style = newStyle;
    }
    
    void setEnabled(bool enabled) {
        state = enabled ? UIState::NORMAL : UIState::DISABLED;
    }

private:
    void updateTextTexture(SDL_Renderer* renderer) {
        if (texture) {
            freeTexture(texture);
        }
        
        if (font) {
            texture = renderTextToTexture(renderer, label, font, style.textColor);
        }
    }

    std::string label;
    std::string fontPath;
    int fontSize;
    
    TTF_Font* font = nullptr;
    SDL_Texture* texture = nullptr;
    
    Button_Style style;
};

} // namespace UI

#endif // UIBUTTON_H