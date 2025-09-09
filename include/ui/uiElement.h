#ifndef UIELEMENT_H
#define UIELEMENT_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>
#include "../input.h"

namespace UI {

using Id = size_t;
using EventCallback = std::function<void()>;

// Forward declarations
class UIManager;

// Enum for UI element alignment
enum class Alignment {
    LEFT,
    CENTER,
    RIGHT,
    TOP,
    MIDDLE,
    BOTTOM
};

// Alias for backwards compatibility
using TextAlignment = Alignment;

// Enum for UI element states
enum class UIState {
    NORMAL,
    HOVER,
    ACTIVE,
    DISABLED
};

/**
 * Base class for all UI elements
 */
class UIElement {
public:
    UIElement(const SDL_FRect& rect, bool visible = true) 
        : rect(rect), visible(visible), id(nextId++) {}
    
    virtual ~UIElement() = default;

    // Core functions all UI elements must implement
    virtual void render(SDL_Renderer* renderer) = 0;
    virtual void update(Input& input) = 0;
    virtual void cleanup() = 0;
    
    // Common functionality for all UI elements
    virtual bool containsPoint(float x, float y) const {
        return (x >= rect.x && x <= rect.x + rect.w &&
                y >= rect.y && y <= rect.y + rect.h);
    }
    
    // Getters and setters
    Id getId() const { return id; }
    const SDL_FRect& getRect() const { return rect; }
    bool isVisible() const { return visible; }
    UIState getState() const { return state; }
    
    void setRect(const SDL_FRect& newRect) { rect = newRect; }
    void setVisible(bool vis) { visible = vis; }
    void setState(UIState newState) { state = newState; }
    
    // Event callbacks
    void setOnClick(EventCallback callback) { onClick = callback; }
    void setOnHover(EventCallback callback) { onHover = callback; }
    void setOnLeave(EventCallback callback) { onLeave = callback; }

protected:
    SDL_FRect rect;
    bool visible = true;
    UIState state = UIState::NORMAL;
    Id id;
    
    // Event callbacks
    EventCallback onClick;
    EventCallback onHover;
    EventCallback onLeave;
    
    static Id nextId;
    
    friend class UIManager;
};

// Static member initialization
inline Id UIElement::nextId = 0;

/**
 * Font management utilities
 */
class FontManager {
public:
    static FontManager& getInstance() {
        static FontManager instance;
        return instance;
    }
    
    TTF_Font* getFont(const std::string& fontPath, int fontSize) {
        std::string key = fontPath + "_" + std::to_string(fontSize);
        
        if (fonts.find(key) == fonts.end()) {
            fonts[key] = TTF_OpenFont(fontPath.c_str(), fontSize);
            if (!fonts[key]) {
                SDL_Log("Failed to load font %s: %s", fontPath.c_str(), TTF_GetError());
            }
        }
        
        return fonts[key];
    }
    
    void cleanup() {
        for (auto& [key, font] : fonts) {
            TTF_CloseFont(font);
        }
        fonts.clear();
    }
    
    ~FontManager() {
        cleanup();
    }
    
private:
    FontManager() {
        if (TTF_Init() != 0) {
            SDL_Log("TTF_Init failed: %s", TTF_GetError());
        }
    }
    
    std::unordered_map<std::string, TTF_Font*> fonts;
};

/**
 * Text rendering utilities
 */
inline SDL_Texture* renderTextToTexture(SDL_Renderer* renderer, const std::string& text, 
                                      TTF_Font* font, const SDL_Color& color) {
    if (!font) return nullptr;
    
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) {
        SDL_Log("Failed to render text: %s", TTF_GetError());
        return nullptr;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    return texture;
}

inline void renderTextureWithAlignment(SDL_Renderer* renderer, SDL_Texture* texture, 
                                     const SDL_FRect& rect, Alignment hAlign = Alignment::CENTER, 
                                     Alignment vAlign = Alignment::MIDDLE) {
    if (!texture) return;
    
    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
    
    SDL_FRect destRect = rect;
    
    // Horizontal alignment
    switch (hAlign) {
        case Alignment::LEFT:
            destRect.x = rect.x;
            break;
        case Alignment::CENTER:
            destRect.x = rect.x + (rect.w - width) / 2.0f;
            break;
        case Alignment::RIGHT:
            destRect.x = rect.x + rect.w - width;
            break;
        default:
            break;
    }
    
    // Vertical alignment
    switch (vAlign) {
        case Alignment::TOP:
            destRect.y = rect.y;
            break;
        case Alignment::MIDDLE:
            destRect.y = rect.y + (rect.h - height) / 2.0f;
            break;
        case Alignment::BOTTOM:
            destRect.y = rect.y + rect.h - height;
            break;
        default:
            break;
    }
    
    destRect.w = static_cast<float>(width);
    destRect.h = static_cast<float>(height);
    
    SDL_RenderCopyF(renderer, texture, nullptr, &destRect);
}

/**
 * Memory management utilities
 */
inline void freeSurface(SDL_Surface*& surface) {
    if (surface) {
        SDL_FreeSurface(surface);
        surface = nullptr;
    }
}

inline void freeTexture(SDL_Texture*& texture) {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}

} // namespace UI

#endif // UIELEMENT_H