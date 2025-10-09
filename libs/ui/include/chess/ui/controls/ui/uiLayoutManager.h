// DEPRECATED: Legacy UILayoutManager kept only for historical reference.
// Not included in builds; UIPanel built-in layouts + UIEnhancedBuilder replace this.
// This file will be removed in a future cleanup.
#ifndef UI_LAYOUT_MANAGER_H
#define UI_LAYOUT_MANAGER_H

#include <vector>
#include <functional>
#include <string>

#if __has_include(<SDL.h>)
#  include <SDL.h>
#else
#  include <SDL2/SDL.h>
#endif

// Automatic layout system inspired by immediate mode UI
// Provides stack-based layout management for cleaner positioning

class UILayoutManager {
public:
    enum class LayoutKind {
        Horizontal,
        Vertical,
        Manual  // No automatic positioning
    };

    struct Layout {
        LayoutKind kind;
        SDL_Point pos;      // Current position for next element
        SDL_Point size;     // Current total size of layout
        int padding;        // Space between elements
        SDL_Rect bounds;    // Available area for layout

        SDL_Point getNextPos() const;
        void addElement(SDL_Rect elementRect);
    };

private:
    static const size_t MAX_LAYOUTS = 32;
    Layout layouts[MAX_LAYOUTS];
    size_t layoutCount = 0;

public:
    // Stack management
    void pushLayout(LayoutKind kind, SDL_Rect bounds, int padding = 8);
    void popLayout();
    Layout* getCurrentLayout();
    
    // Helper for positioning elements
    SDL_Rect getNextElementRect(int width, int height);
    void addElementToLayout(SDL_Rect rect);
    
    // Check if we have active layouts
    bool hasActiveLayout() const { return layoutCount > 0; }
    
    // Clear all layouts
    void clear() { layoutCount = 0; }
};

// Helper functions for common layout patterns
namespace UILayout {
    // Begin/end layout blocks
    void beginVertical(UILayoutManager& mgr, SDL_Rect bounds, int padding = 8);
    void beginHorizontal(UILayoutManager& mgr, SDL_Rect bounds, int padding = 8);
    void end(UILayoutManager& mgr);
    
    // Auto-size common elements
    SDL_Rect buttonRect(UILayoutManager& mgr, int width = 200, int height = 40);
    SDL_Rect labelRect(UILayoutManager& mgr, const std::string& text, int fontSize = 16);
}

#endif // UI_LAYOUT_MANAGER_H
