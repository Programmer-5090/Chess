#pragma once

#if __has_include(<SDL.h>)
#  include <SDL.h>
#else
#  include <SDL2/SDL.h>
#endif

// UIPanel: container widget that owns children and optionally applies a layout.
// - Layouts: None, Vertical, Grid, or a custom lambda provided at runtime
// - Horizontal layout (new): lays out children left-to-right and wraps to a new row when exceeding inner width
// - Edit mode: when enabled, children can be dragged with the mouse; movement is clamped
//   to the panel and visuals update continuously during motion via onRectChanged()
// - Input: forwards updates only when the mouse is over both the panel and the child,
//   unless a child explicitly opts-in to outside input (e.g., expanded dropdown)
#include <vector>
#include <functional>
#include <algorithm>

#include "chess/ui/controls/ui/uiElement.h"
#include "chess/ui/controls/ui/uiConfig.h" // for temporary callback disable in edit mode
#include "chess/ui/input.h"

class UIPanel : public UIElement {
public:
    enum class LayoutType { None, Vertical, Horizontal, Grid };

    UIPanel(int x, int y, int width, int height,
            SDL_Color background = {30,30,40,220},
            SDL_Color border = {50,50,60,255},
            int borderThickness = 2)
        : UIElement(x, y, width, height),
          background(background), border(border), borderThickness(borderThickness) {}

    ~UIPanel() override {
        clearChildren();
    }

    // Add a child element managed by this panel (ownership transferred)
    template<typename T, typename... Args>
    T* addChild(Args&&... args) {
        T* child = new T(std::forward<Args>(args)...);
        children.push_back(child);
        layoutDirty = true;
        return child;
    }

    void clearChildren() {
        for (auto* c : children) delete c;
        children.clear();
    }

    // Layout APIs
    void setLayoutNone() { layoutType = LayoutType::None; layoutDirty = true; }
    void setLayoutVertical(int px = 10, int py = 10, int spY = 8) {
        layoutType = LayoutType::Vertical;
        this->paddingX = px;
        this->paddingY = py;
        this->spacingX = 0;
        this->spacingY = spY;
        layoutDirty = true;
    }
    void setLayoutHorizontal(int px = 10, int py = 10, int gapX = 8, int gapY = 8) {
        layoutType = LayoutType::Horizontal;
        this->paddingX = px;
        this->paddingY = py;
        this->spacingX = gapX;
        this->spacingY = gapY;
        layoutDirty = true;
    }
    void setLayoutGrid(int cols, int px = 10, int py = 10, int gapX = 8, int gapY = 8) {
        layoutType = LayoutType::Grid;
        this->columns = std::max(1, cols);
        this->paddingX = px;
        this->paddingY = py;
        this->spacingX = gapX;
        this->spacingY = gapY;
        layoutDirty = true;
    }
    void setCustomLayout(std::function<void(UIPanel&)> fn) {
        customLayout = std::move(fn);
        layoutDirty = true;
    }

    // Enable edit mode to drag children with the mouse
    void setEditable(bool on) {
        if (editable == on) return;
        editable = on;
        if (editable) {
            // Edit mode: disable all callbacks except bypass buttons
            UIConfig::setEditModeActive(true);
        } else {
            // Leaving edit mode: stop dragging and restore callbacks
            draggingChild = nullptr;
            UIConfig::setEditModeActive(false);
        }
    }
    bool isEditable() const { return editable; }

    // Update propagates to children; in edit mode, supports drag-to-move
    void update(Input& input) override {
        if (!visible) return;

    // Relayout if needed (and not while actively dragging or editing)
    if (layoutDirty && !draggingChild && !editable) {
            applyLayout();
            layoutDirty = false;
        }

        const SDL_Event& e = input.getCurrentEvent();
        const int mx = input.getMouseX();
        const int my = input.getMouseY();

        if (editable) {
            switch (e.type) {
                case SDL_MOUSEBUTTONDOWN:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        if (pointInRect(mx, my, rect)) {
                            // find topmost child under mouse
                            for (int i = static_cast<int>(children.size()) - 1; i >= 0; --i) {
                                UIElement* c = children[i];
                                if (c && c->visible && pointInRect(mx, my, c->rect)) {
                                    draggingChild = c;
                                    // bring to front
                                    children.erase(children.begin() + i);
                                    children.push_back(c);
                                    dragOffsetX = mx - c->rect.x;
                                    dragOffsetY = my - c->rect.y;
                                    break;
                                }
                            }
                        }
                    }
                    return; // eat down event so children don't react in edit mode
                case SDL_MOUSEMOTION:
                    if (draggingChild) {
                        draggingChild->rect.x = mx - dragOffsetX;
                        draggingChild->rect.y = my - dragOffsetY;
                        // Clamp inside panel bounds
                        if (draggingChild->rect.x < rect.x) draggingChild->rect.x = rect.x;
                        if (draggingChild->rect.y < rect.y) draggingChild->rect.y = rect.y;
                        if (draggingChild->rect.x + draggingChild->rect.w > rect.x + rect.w)
                            draggingChild->rect.x = rect.x + rect.w - draggingChild->rect.w;
                        if (draggingChild->rect.y + draggingChild->rect.h > rect.y + rect.h)
                            draggingChild->rect.y = rect.y + rect.h - draggingChild->rect.h;
                        // Ensure the element updates any internal geometry for immediate visual feedback
                        draggingChild->onRectChanged();
                    }
                    return; // eat motion in edit mode
                case SDL_MOUSEBUTTONUP:
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        if (draggingChild) {
                            draggingChild->onRectChanged();
                            draggingChild = nullptr;
                        }
                    }
                    return; // eat up in edit mode
                default:
                    break;
            }
        }

        // Forward update to children in order, with input clipping to panel bounds.
        // Allow exceptions (e.g., expanded dropdown) by letting a child opt-in.
        for (auto* c : children) {
            if (!c || !c->visible) continue;
            bool allow = false;
            // Basic rule: if mouse is inside panel and inside child, allow.
            if (pointInRect(mx, my, rect) && pointInRect(mx, my, c->rect)) {
                allow = true;
            } else if (c->wantsOutsidePanelInput()) {
                // If child wants outside input (e.g., dropdown list), allow only when mouse intersects child's main rect
                // or the child's own overlay/list will handle closing on outside clicks.
                if (pointInRect(mx, my, c->rect)) allow = true;
            }
            // Also allow non-mouse events (keyboard/text) to focused children; for simplicity, forward always for now
            // when the event is not a mouse event.
            const SDL_Event& ev = input.getCurrentEvent();
            if (ev.type != SDL_MOUSEMOTION && ev.type != SDL_MOUSEBUTTONDOWN && ev.type != SDL_MOUSEBUTTONUP) {
                allow = true;
            }
            if (allow) c->update(input);
        }
    }

    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        // Fill background
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
        SDL_RenderFillRect(renderer, &rect);

        // Clip children to panel bounds
        SDL_Rect prevClip; bool hadClip = SDL_FALSE;
        SDL_RenderGetClipRect(renderer, &prevClip);
        // Unfortunately SDL doesn't return a flag, assume any prevClip w/h=0 means no clip
        hadClip = (prevClip.w != 0 || prevClip.h != 0);
        SDL_RenderSetClipRect(renderer, &rect);

        for (auto* c : children) {
            if (c && c->visible) c->render(renderer);
        }

        // restore clip
        if (hadClip) SDL_RenderSetClipRect(renderer, &prevClip); else SDL_RenderSetClipRect(renderer, nullptr);

        // Border on top
        if (borderThickness > 0 && border.a != 0) {
            SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
            SDL_Rect r = rect;
            for (int i = 0; i < borderThickness; ++i) {
                SDL_RenderDrawRect(renderer, &r);
                r.x += 1; r.y += 1; r.w -= 2; r.h -= 2;
                if (r.w <= 0 || r.h <= 0) break;
            }
        }
    }

    void renderOverlay(SDL_Renderer* renderer) override {
        if (!visible) return;
        // Overlays should not be clipped to panel by default
        for (auto* c : children) {
            if (c && c->visible) c->renderOverlay(renderer);
        }
    }

    // Debug-only helper to iterate children from outside (used by demo custom layout)
    const std::vector<UIElement*>& _debugGetChildren() const { return children; }

    // Accessors for layout metrics
    int getPaddingX() const { return paddingX; }
    int getPaddingY() const { return paddingY; }
    int getSpacingX() const { return spacingX; }
    int getSpacingY() const { return spacingY; }

private:
    SDL_Color background;
    SDL_Color border;
    int borderThickness;

    std::vector<UIElement*> children;

    // Layout state
    LayoutType layoutType = LayoutType::None;
    int paddingX = 10;
    int paddingY = 10;
    int spacingX = 8;
    int spacingY = 8;
    int columns = 2;
    bool layoutDirty = false;
    std::function<void(UIPanel&)> customLayout;

    // Edit mode
    bool editable = false;
    UIElement* draggingChild = nullptr;
    int dragOffsetX = 0;
    int dragOffsetY = 0;

    static bool pointInRect(int x, int y, const SDL_Rect& r) {
        return x >= r.x && y >= r.y && x < (r.x + r.w) && y < (r.y + r.h);
    }

    void applyLayout() {
        if (customLayout) { customLayout(*this); return; }
        switch (layoutType) {
            case LayoutType::Vertical: layoutVertical(); break;
            case LayoutType::Horizontal: layoutHorizontal(); break;
            case LayoutType::Grid: layoutGrid(); break;
            case LayoutType::None: default: break;
        }
    }

    void layoutVertical() {
        int x = rect.x + paddingX;
        int y = rect.y + paddingY;
        int contentW = rect.w - 2 * paddingX;
        int maxBottom = y;
        
        for (auto* c : children) {
            if (!c) continue;
            // Ensure child width does not exceed content width unless stretching
            if (c->getHorizontalAlign() != UIElement::HorizontalAlign::Stretch && c->rect.w > contentW) {
                c->rect.w = contentW;
            }
            // Horizontal alignment within panel content width
            switch (c->getHorizontalAlign()) {
                case UIElement::HorizontalAlign::Left: c->rect.x = x; break;
                case UIElement::HorizontalAlign::Center: c->rect.x = x + (contentW - c->rect.w) / 2; break;
                case UIElement::HorizontalAlign::Right: c->rect.x = x + (contentW - c->rect.w); break;
                case UIElement::HorizontalAlign::Stretch: c->rect.x = x; c->rect.w = contentW; break;
            }
            // Vertical alignment for row (top/middle/bottom inside its own height)
            // In vertical layout, y is the current cursor; element's own height is respected.
            // Middle/Bottom are no-ops without row height context; keep top behavior for simplicity.
            c->rect.y = y;
            c->onRectChanged();
            y += c->rect.h + spacingY;
            maxBottom = std::max(maxBottom, c->rect.y + c->rect.h);
        }
        
        // Auto-grow panel height to fit all content (prevents clipping)
        if (!children.empty()) {
            int neededH = (maxBottom - rect.y) + paddingY; // content height + bottom padding
            if (neededH > rect.h) {
                rect.h = neededH;
            }
        }
    }

    void layoutGrid() {
        int x0 = rect.x + paddingX;
        int y0 = rect.y + paddingY;
        int col = 0;
        int x = x0;
        int y = y0;
        int maxRowHeight = 0;
        int totalGaps = (columns - 1) * spacingX;
        int colWidth = (rect.w - 2 * paddingX - totalGaps) / std::max(1, columns);
        for (auto* c : children) {
            if (!c) continue;
            if (c->rect.x != x || c->rect.y != y) {
                c->rect.x = x;
                c->rect.y = y;
                c->onRectChanged();
            }
            if (c->rect.w > colWidth) {
                c->rect.w = colWidth;
                c->onRectChanged();
            }
            maxRowHeight = std::max(maxRowHeight, c->rect.h);
            ++col;
            if (col >= columns) {
                col = 0;
                x = x0;
                y += maxRowHeight + spacingY;
                maxRowHeight = 0;
            } else {
                x += c->rect.w + spacingX;
            }
        }
    }

    void layoutHorizontal() {
        int x0 = rect.x + paddingX;
        int y0 = rect.y + paddingY;
        int x = x0;
        int y = y0;
        int maxRowHeight = 0;
        const int contentRight = rect.x + rect.w - paddingX;
        const int contentW = rect.w - 2 * paddingX;
        for (auto* c : children) {
            if (!c) continue;
            // Wrap to next row if exceeding width
            if (x + c->rect.w > contentRight && x != x0) {
                x = x0;
                y += maxRowHeight + spacingY;
                maxRowHeight = 0;
            }
            // Apply stretch width before alignment decisions
            if (c->getHorizontalAlign() == UIElement::HorizontalAlign::Stretch) {
                c->rect.w = std::min(c->rect.w, contentRight - x);
            } else if (c->rect.w > contentW) {
                // Clamp width to content width to avoid overflow/cutoff
                c->rect.w = contentW;
            }
            // Horizontal alignment within current row
            switch (c->getHorizontalAlign()) {
                case UIElement::HorizontalAlign::Left: c->rect.x = x; break;
                case UIElement::HorizontalAlign::Center:
                    // Center within available row space between x and contentRight
                    {
                        int remaining = contentRight - x;
                        c->rect.x = x + std::max(0, (remaining - c->rect.w) / 2);
                    }
                    break;
                case UIElement::HorizontalAlign::Right: c->rect.x = contentRight - c->rect.w; break;
                case UIElement::HorizontalAlign::Stretch: c->rect.x = x; break;
            }
            // Vertical alignment within the current row
            switch (c->getVerticalAlign()) {
                case UIElement::VerticalAlign::Top: c->rect.y = y; break;
                case UIElement::VerticalAlign::Middle: c->rect.y = y + (maxRowHeight > 0 ? (maxRowHeight - c->rect.h) / 2 : 0); break;
                case UIElement::VerticalAlign::Bottom: c->rect.y = y + std::max(0, maxRowHeight - c->rect.h); break;
                case UIElement::VerticalAlign::Stretch: c->rect.y = y; /* keep height */ break;
            }
            c->onRectChanged();
            x += c->rect.w + spacingX;
            maxRowHeight = std::max(maxRowHeight, c->rect.h);
        }
        // Auto-grow panel height to include the final row
        if (!children.empty()) {
            int contentBottom = y + maxRowHeight; // bottom of last row
            int neededH = (contentBottom - rect.y) + paddingY;
            if (neededH > rect.h) rect.h = neededH;
        }
    }
};
