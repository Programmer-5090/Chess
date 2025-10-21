#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include <SDL.h>

// Forward declaration
class Input;

class UIElement {
public:
    // Alignment enums used by containers (e.g., UIPanel) during layout
    enum class HorizontalAlign { Left, Center, Right, Stretch };
    enum class VerticalAlign { Top, Middle, Bottom, Stretch };

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

    // Alignment API
    void setHorizontalAlign(HorizontalAlign a) { hAlign = a; }
    void setVerticalAlign(VerticalAlign a) { vAlign = a; }
    HorizontalAlign getHorizontalAlign() const { return hAlign; }
    VerticalAlign getVerticalAlign() const { return vAlign; }

    SDL_Rect rect;
    bool visible;

private:
    HorizontalAlign hAlign = HorizontalAlign::Left;
    VerticalAlign vAlign = VerticalAlign::Top;
};

#endif // UI_ELEMENT_H
