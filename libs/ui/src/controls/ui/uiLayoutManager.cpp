#include "chess/ui/controls/ui/uiLayoutManager.h"
#include <algorithm>
#include <string>

SDL_Point UILayoutManager::Layout::getNextPos() const {
    switch (kind) {
    case LayoutKind::Horizontal:
        return { pos.x + size.x + (size.x > 0 ? padding : 0), pos.y };
    case LayoutKind::Vertical:
        return { pos.x, pos.y + size.y + (size.y > 0 ? padding : 0) };
    case LayoutKind::Manual:
    default:
        return pos;
    }
}

void UILayoutManager::Layout::addElement(SDL_Rect elementRect) {
    switch (kind) {
    case LayoutKind::Horizontal:
        size.x += elementRect.w + (size.x > 0 ? padding : 0);
        size.y = std::max(size.y, elementRect.h);
        break;
    case LayoutKind::Vertical:
        size.x = std::max(size.x, elementRect.w);
        size.y += elementRect.h + (size.y > 0 ? padding : 0);
        break;
    case LayoutKind::Manual:
        // No automatic sizing for manual layouts
        break;
    }
}

void UILayoutManager::pushLayout(LayoutKind kind, SDL_Rect bounds, int padding) {
    if (layoutCount >= MAX_LAYOUTS) return;
    
    Layout& layout = layouts[layoutCount++];
    layout.kind = kind;
    layout.pos = { bounds.x, bounds.y };
    layout.size = { 0, 0 };
    layout.padding = padding;
    layout.bounds = bounds;
}

void UILayoutManager::popLayout() {
    if (layoutCount > 0) {
        layoutCount--;
    }
}

UILayoutManager::Layout* UILayoutManager::getCurrentLayout() {
    return layoutCount > 0 ? &layouts[layoutCount - 1] : nullptr;
}

SDL_Rect UILayoutManager::getNextElementRect(int width, int height) {
    Layout* current = getCurrentLayout();
    if (!current) {
        // No layout active, return a default position
        return { 0, 0, width, height };
    }
    
    SDL_Point nextPos = current->getNextPos();
    
    // Clamp to layout bounds if needed
    if (nextPos.x + width > current->bounds.x + current->bounds.w) {
        width = (current->bounds.x + current->bounds.w) - nextPos.x;
    }
    if (nextPos.y + height > current->bounds.y + current->bounds.h) {
        height = (current->bounds.y + current->bounds.h) - nextPos.y;
    }
    
    return { nextPos.x, nextPos.y, std::max(0, width), std::max(0, height) };
}

void UILayoutManager::addElementToLayout(SDL_Rect rect) {
    Layout* current = getCurrentLayout();
    if (current) {
        current->addElement(rect);
    }
}

// Helper functions implementation
namespace UILayout {
    void beginVertical(UILayoutManager& mgr, SDL_Rect bounds, int padding) {
        mgr.pushLayout(UILayoutManager::LayoutKind::Vertical, bounds, padding);
    }
    
    void beginHorizontal(UILayoutManager& mgr, SDL_Rect bounds, int padding) {
        mgr.pushLayout(UILayoutManager::LayoutKind::Horizontal, bounds, padding);
    }
    
    void end(UILayoutManager& mgr) {
        mgr.popLayout();
    }
    
    SDL_Rect buttonRect(UILayoutManager& mgr, int width, int height) {
        SDL_Rect rect = mgr.getNextElementRect(width, height);
        mgr.addElementToLayout(rect);
        return rect;
    }
    
    SDL_Rect labelRect(UILayoutManager& mgr, const std::string& text, int fontSize) {
        // Estimate text width (rough approximation)
        int estimatedWidth = static_cast<int>(text.length() * fontSize * 0.6f);
        int height = fontSize + 4;
        
        SDL_Rect rect = mgr.getNextElementRect(estimatedWidth, height);
        mgr.addElementToLayout(rect);
        return rect;
    }
}
