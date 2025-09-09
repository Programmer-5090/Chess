#ifndef UI_LABEL_H
#define UI_LABEL_H

#include "uiElement.h"

namespace UI {

/**
 * Label UI element for displaying text
 */
class Label : public UIElement {
public:
    Label(const SDL_FRect& rect, const std::string& text, 
          const std::string& fontPath, int fontSize)
        : UIElement(rect), text(text), fontPath(fontPath), fontSize(fontSize),
          alignment(TextAlignment::CENTER) {}
    
    ~Label() override {
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
        
        // Render background if enabled
        if (drawBackground) {
            SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, 
                                  backgroundColor.b, backgroundColor.a);
            SDL_RenderFillRectF(renderer, &rect);
        }
        
        // Render text
        if (texture) {
            renderTextureWithAlignment(renderer, texture, rect, alignment);
        }
    }
    
    void update(Input& input) override {
        // Labels don't have interactive behavior by default
        if (!visible) return;
    }
    
    void cleanup() override {
        freeTexture(texture);
    }
    
    // Setters
    void setText(const std::string& newText, SDL_Renderer* renderer) {
        if (text != newText) {
            text = newText;
            // Recreate texture with new text
            freeTexture(texture);
            updateTextTexture(renderer);
        }
    }
    
    void setTextColor(const SDL_Color& color, SDL_Renderer* renderer) {
        if (!colorsEqual(textColor, color)) {
            textColor = color;
            freeTexture(texture);
            updateTextTexture(renderer);
        }
    }
    
    void setAlignment(TextAlignment align) {
        alignment = align;
    }
    
    void setBackgroundColor(const SDL_Color& color) {
        backgroundColor = color;
    }
    
    void setDrawBackground(bool draw) {
        drawBackground = draw;
    }

private:
    bool colorsEqual(const SDL_Color& a, const SDL_Color& b) {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }
    
    void updateTextTexture(SDL_Renderer* renderer) {
        if (texture) {
            freeTexture(texture);
        }
        
        if (font && !text.empty()) {
            texture = renderTextToTexture(renderer, text, font, textColor);
        }
    }

    std::string text;
    std::string fontPath;
    int fontSize;
    TextAlignment alignment;
    
    TTF_Font* font = nullptr;
    SDL_Texture* texture = nullptr;
    
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Color backgroundColor = {0, 0, 0, 0};
    bool drawBackground = false;
};

} // namespace UI

#endif // UI_LABEL_H
