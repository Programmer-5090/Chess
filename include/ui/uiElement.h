#pragma once

#include <SDL.h>

class Input; // forward declaration if needed elsewhere

class UIElement {
public:
    UIElement(int x, int y, int width, int height)
        : rect{ x, y, width, height }, visible(true) {}
    virtual ~UIElement() = default;

    virtual void update(Input& input) {}
    virtual void render(SDL_Renderer* renderer) {}
    // Optional second render pass for overlays that must appear above all elements
    virtual void renderOverlay(SDL_Renderer* renderer) {}
    virtual bool isModal() const { return false; }
    // Panels can ask whether an element needs input even when the mouse is outside
    // the panel area (e.g., dropdown with an expanded list). Default: false.
    virtual bool wantsOutsidePanelInput() const { return false; }
    // Called by containers when rect is changed externally (layout, drag, etc.)
    virtual void onRectChanged() {}

    SDL_Rect rect;
    bool visible;
};
