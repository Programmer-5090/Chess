#ifndef UI_PANEL_H
#define UI_PANEL_H

#include "uiElement.h"
#include <vector>
#include <memory>

namespace UI {

/**
 * Panel UI element that can contain other UI elements
 */
class Panel : public UIElement {
public:
    Panel(const SDL_FRect& rect) 
        : UIElement(rect), backgroundColor({40, 40, 40, 220}) {}
    
    ~Panel() override {
        cleanup();
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        
        // Render panel background
        SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, 
                              backgroundColor.b, backgroundColor.a);
        SDL_RenderFillRectF(renderer, &rect);
        
        // Render border if enabled
        if (drawBorder) {
            SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, 
                                  borderColor.b, borderColor.a);
            SDL_RenderDrawRectF(renderer, &rect);
        }
        
        // Render all child elements
        for (auto& element : children) {
            if (element) {
                element->render(renderer);
            }
        }
    }
    
    void update(Input& input) override {
        if (!visible) return;
        
        // Update all child elements
        for (auto& element : children) {
            if (element) {
                element->update(input);
            }
        }
    }
    
    void cleanup() override {
        // Clean up all child elements
        for (auto& element : children) {
            if (element) {
                element->cleanup();
            }
        }
        children.clear();
    }
    
    // Add a child UI element to the panel
    void addChild(std::shared_ptr<UIElement> element) {
        if (element) {
            children.push_back(element);
        }
    }
    
    // Remove a child UI element from the panel
    void removeChild(std::shared_ptr<UIElement> element) {
        auto it = std::find(children.begin(), children.end(), element);
        if (it != children.end()) {
            (*it)->cleanup();
            children.erase(it);
        }
    }
    
    // Clear all child elements
    void clearChildren() {
        for (auto& element : children) {
            if (element) {
                element->cleanup();
            }
        }
        children.clear();
    }
    
    // Setters
    void setBackgroundColor(const SDL_Color& color) {
        backgroundColor = color;
    }
    
    void setBorderColor(const SDL_Color& color) {
        borderColor = color;
    }
    
    void setDrawBorder(bool draw) {
        drawBorder = draw;
    }

private:
    std::vector<std::shared_ptr<UIElement>> children;
    SDL_Color backgroundColor;
    SDL_Color borderColor = {255, 255, 255, 255};
    bool drawBorder = false;
};

} // namespace UI

#endif // UI_PANEL_H
