#ifndef UI_DROPDOWN_H
#define UI_DROPDOWN_H

#include "uiElement.h"
#include <vector>
#include <functional>

namespace UI {

/**
 * Dropdown menu UI element
 */
class Dropdown : public UIElement {
public:
    Dropdown(const SDL_FRect& rect, const std::vector<std::string>& items,
             const std::string& fontPath, int fontSize)
        : UIElement(rect), items(items), fontPath(fontPath), fontSize(fontSize),
          selectedIndex(-1), isOpen(false) {}
    
    ~Dropdown() override {
        cleanup();
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        
        // Lazy initialization of font
        if (!font) {
            font = FontManager::getInstance().getFont(fontPath, fontSize);
        }
        
        // Draw dropdown main box
        SDL_Color bgColor = isOpen ? activeColor : normalColor;
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRectF(renderer, &rect);
        
        // Draw border
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        SDL_RenderDrawRectF(renderer, &rect);
        
        // Render selected text or placeholder
        std::string displayText = (selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size())) 
                                ? items[selectedIndex] : "Select an option";
        
        if (!headerTexture || displayText != lastRenderedHeader) {
            freeTexture(headerTexture);
            headerTexture = renderTextToTexture(renderer, displayText, font, textColor);
            lastRenderedHeader = displayText;
        }
        
        // Render selected text
        if (headerTexture) {
            renderTextureWithAlignment(renderer, headerTexture, rect, TextAlignment::LEFT);
        }
        
        // Draw dropdown arrow
        drawArrow(renderer);
        
        // If dropdown is open, draw the dropdown list
        if (isOpen) {
            // Calculate dropdown list rect
            SDL_FRect dropdownRect = {
                rect.x, rect.y + rect.h,
                rect.w, rect.h * std::min(5.0f, static_cast<float>(items.size()))
            };
            
            // Draw dropdown background
            SDL_SetRenderDrawColor(renderer, dropdownBgColor.r, dropdownBgColor.g, 
                                  dropdownBgColor.b, dropdownBgColor.a);
            SDL_RenderFillRectF(renderer, &dropdownRect);
            
            // Draw border
            SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, 
                                  borderColor.b, borderColor.a);
            SDL_RenderDrawRectF(renderer, &dropdownRect);
            
            // Draw items
            for (size_t i = 0; i < items.size(); i++) {
                SDL_FRect itemRect = {
                    dropdownRect.x, dropdownRect.y + (rect.h * i),
                    dropdownRect.w, rect.h
                };
                
                // Highlight hovered item
                if (static_cast<int>(i) == hoveredIndex) {
                    SDL_SetRenderDrawColor(renderer, hoverColor.r, hoverColor.g, 
                                          hoverColor.b, hoverColor.a);
                    SDL_RenderFillRectF(renderer, &itemRect);
                }
                
                // Render item text
                if (i >= itemTextures.size() || !itemTextures[i]) {
                    // Create texture if it doesn't exist
                    if (i >= itemTextures.size()) {
                        itemTextures.resize(i + 1, nullptr);
                    }
                    itemTextures[i] = renderTextToTexture(renderer, items[i], font, textColor);
                }
                
                if (itemTextures[i]) {
                    renderTextureWithAlignment(renderer, itemTextures[i], itemRect, TextAlignment::LEFT);
                }
                
                // Draw separator line between items
                if (i < items.size() - 1) {
                    SDL_SetRenderDrawColor(renderer, separatorColor.r, separatorColor.g, 
                                          separatorColor.b, separatorColor.a);
                    SDL_FRect separatorRect = {
                        itemRect.x + 5, itemRect.y + itemRect.h - 1,
                        itemRect.w - 10, 1
                    };
                    SDL_RenderFillRectF(renderer, &separatorRect);
                }
            }
        }
    }
    
    void update(Input& input) override {
        if (!visible || state == UIState::DISABLED) return;
        
        int mouseX = input.getMouseX();
        int mouseY = input.getMouseY();
        
        bool wasOpen = isOpen;
        bool isHovering = containsPoint(mouseX, mouseY);
        
        // Update hoveredIndex
        hoveredIndex = -1;
        if (isOpen) {
            SDL_FRect dropdownRect = {
                rect.x, rect.y + rect.h,
                rect.w, rect.h * std::min(5.0f, static_cast<float>(items.size()))
            };
            
            if (mouseX >= dropdownRect.x && mouseX <= dropdownRect.x + dropdownRect.w &&
                mouseY >= dropdownRect.y && mouseY <= dropdownRect.y + dropdownRect.h) {
                hoveredIndex = static_cast<int>((mouseY - dropdownRect.y) / rect.h);
                if (hoveredIndex >= static_cast<int>(items.size())) {
                    hoveredIndex = -1;
                }
            }
        }
        
        // Handle mouse click
        if (input.isMouseButtonReleased(SDL_BUTTON_LEFT)) {
            if (isHovering) {
                isOpen = !isOpen;
            } else if (isOpen && hoveredIndex != -1) {
                selectedIndex = hoveredIndex;
                isOpen = false;
                if (onSelectionChanged) {
                    onSelectionChanged(selectedIndex);
                }
            } else {
                isOpen = false;
            }
        }
        
        // Handle mouse hover
        if (isHovering) {
            state = UIState::HOVER;
        } else {
            state = UIState::NORMAL;
        }
    }
    
    void cleanup() override {
        freeTexture(headerTexture);
        for (auto& texture : itemTextures) {
            freeTexture(texture);
        }
        itemTextures.clear();
    }
    
    // Setters
    void setItems(const std::vector<std::string>& newItems) {
        items = newItems;
        if (selectedIndex >= static_cast<int>(items.size())) {
            selectedIndex = -1;
        }
        cleanup(); // Force recreation of textures
    }
    
    void setSelectedIndex(int index) {
        if (index >= -1 && index < static_cast<int>(items.size())) {
            selectedIndex = index;
            if (onSelectionChanged) {
                onSelectionChanged(selectedIndex);
            }
        }
    }
    
    int getSelectedIndex() const {
        return selectedIndex;
    }
    
    std::string getSelectedItem() const {
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(items.size())) {
            return items[selectedIndex];
        }
        return "";
    }
    
    void setOnSelectionChanged(std::function<void(int)> callback) {
        onSelectionChanged = callback;
    }
    
    void setColors(const SDL_Color& normal, const SDL_Color& hover, 
                   const SDL_Color& active, const SDL_Color& text,
                   const SDL_Color& dropdownBg, const SDL_Color& border,
                   const SDL_Color& separator) {
        normalColor = normal;
        hoverColor = hover;
        activeColor = active;
        textColor = text;
        dropdownBgColor = dropdownBg;
        borderColor = border;
        separatorColor = separator;
    }

private:
    void drawArrow(SDL_Renderer* renderer) {
        // Draw a simple triangle pointing down (or up if open)
        float arrowSize = rect.h * 0.25f;
        float startX = rect.x + rect.w - arrowSize * 2;
        float startY = rect.y + (rect.h - arrowSize) / 2;
        
        // Create an array of points for the triangle
        SDL_FPoint points[3];
        if (!isOpen) {
            // Down arrow
            points[0] = {startX, startY};
            points[1] = {startX + arrowSize * 2, startY};
            points[2] = {startX + arrowSize, startY + arrowSize};
        } else {
            // Up arrow
            points[0] = {startX, startY + arrowSize};
            points[1] = {startX + arrowSize * 2, startY + arrowSize};
            points[2] = {startX + arrowSize, startY};
        }
        
        // Draw the triangle
        // Note: SDL2 doesn't have a built-in triangle drawing function, so we simulate it with lines
        SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b, textColor.a);
        SDL_RenderDrawLineF(renderer, points[0].x, points[0].y, points[1].x, points[1].y);
        SDL_RenderDrawLineF(renderer, points[1].x, points[1].y, points[2].x, points[2].y);
        SDL_RenderDrawLineF(renderer, points[2].x, points[2].y, points[0].x, points[0].y);
    }

    std::vector<std::string> items;
    std::string fontPath;
    int fontSize;
    TTF_Font* font = nullptr;
    
    SDL_Texture* headerTexture = nullptr;
    std::vector<SDL_Texture*> itemTextures;
    std::string lastRenderedHeader;
    
    int selectedIndex;
    int hoveredIndex = -1;
    bool isOpen;
    
    // Colors
    SDL_Color normalColor = {60, 60, 60, 255};
    SDL_Color hoverColor = {80, 80, 80, 255};
    SDL_Color activeColor = {40, 40, 40, 255};
    SDL_Color textColor = {240, 240, 240, 255};
    SDL_Color dropdownBgColor = {30, 30, 30, 240};
    SDL_Color borderColor = {100, 100, 100, 255};
    SDL_Color separatorColor = {100, 100, 100, 100};
    
    // Callback
    std::function<void(int)> onSelectionChanged;
};

} // namespace UI

#endif // UI_DROPDOWN_H
