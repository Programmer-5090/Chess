#include "../../include/ui/uiDropdown.h"
#include "../../include/ui/uiButton.h"
#include "../../include/ui/uiConfig.h"
#include "../../include/input.h"
#include <algorithm>
#include <iostream>

UIDropdown::~UIDropdown() {
    if (arrowButton) { delete arrowButton; arrowButton = nullptr; }
    if (font) { TTF_CloseFont(font); font = nullptr; }
}

void UIDropdown::ensureFont() {
    if (!font) {
        if (!TTF_WasInit()) TTF_Init();
        font = TTF_OpenFont(fontPath.c_str(), fontSize);
        if (!font) {
            std::cout << "Dropdown font load failed: " << TTF_GetError() << std::endl;
        }
    }
}

void UIDropdown::layoutArrowButton() {
    // Arrow button takes a square area on the right, height of control, minus padding on right
    int size = rect.h - 4; // small inset for border
    int ax = rect.x + rect.w - padding - size;
    int ay = rect.y + 2; // small top inset aligning with border
    arrowRect = { ax, ay, size, size };
    // Keep the internal Button geometry in sync with control rect
    if (arrowButton) {
        arrowButton->setRect(ax, ay, size, size);
    }
}

void UIDropdown::ensureArrowButton() {
    if (arrowButton) return;
    // Create a tiny button without text; its callback toggles expanded state
    int size = rect.h - 4;
    int ax = rect.x + rect.w - padding - size;
    int ay = rect.y + 2;
    arrowRect = { ax, ay, size, size };
    arrowButton = new Button(ax, ay, size, size, "", [this]() {
        expanded = !expanded;
        // Reset transient states when toggling
        pressedInMain = false;
        pressedInMainWhileExpanded = false;
        pressedItemIndex = -1;
        hoveredIndex = -1;
    },
    // Use neutral colors matching control
    SDL_Color{230,230,230,255}, SDL_Color{200,200,200,255}, "", SDL_Color{0,0,0,255}, 3, 16);
}

void UIDropdown::update(Input& input) {
    if (!visible) return;

    int mx = input.getMouseX();
    int my = input.getMouseY();
    bool down = input.getMouseStates()["left"];

    SDL_Rect main = { rect.x, rect.y, rect.w, rect.h };
    auto hit = [&](const SDL_Rect& r){ return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h; };

    // ensure arrow button exists and is laid out when geometry changes
    ensureArrowButton();
    layoutArrowButton();
    // Update arrow button regardless of expanded state for hover/press visuals and callback handling
    arrowButton->update(input);

    if (!expanded) {
        if (down && hit(main) && !pressedInMain && !hit(arrowRect)) {
            pressedInMain = true;
        }
        if (!down && pressedInMain) {
            if (hit(main) && !hit(arrowRect)) expanded = true; // toggle open on click on main area
            pressedInMain = false;
        }
    } else {
        // Track click on main area to close when expanded
        if (down && hit(main) && !pressedInMainWhileExpanded && !hit(arrowRect)) {
            pressedInMainWhileExpanded = true;
        }
        if (!down && pressedInMainWhileExpanded) {
            if (hit(main) && !hit(arrowRect)) {
                expanded = false; // close on main click when expanded
                pressedInMainWhileExpanded = false;
                return;
            }
            pressedInMainWhileExpanded = false;
        }

        // if click outside list area, close
        SDL_Rect listRect{ rect.x, rect.y + rect.h, rect.w, static_cast<int>(options.size()) * itemHeight };
        hoveredIndex = -1;
        if (hit(listRect)) {
            int relY = my - listRect.y;
            int idx = relY / itemHeight;
            if (idx >= 0 && idx < static_cast<int>(options.size())) hoveredIndex = idx;
        }
        if (down && hoveredIndex != -1 && pressedItemIndex == -1) {
            pressedItemIndex = hoveredIndex;
        }
        if (!down && pressedItemIndex != -1) {
            if (hoveredIndex == pressedItemIndex) {
                selectedIndex = pressedItemIndex;
                if (onChange && UIConfig::areCallbacksEnabled()) onChange(selectedIndex, options[selectedIndex]);
            }
            pressedItemIndex = -1;
            expanded = false;
        }
        if (!hit(listRect) && !down) {
            // click release outside closes (except when released on arrow; handled above)
            if (!hit(main) && !hit(arrowRect)) expanded = false;
        }
    }
}

void UIDropdown::render(SDL_Renderer* renderer) {
    if (!visible) return;
    ensureFont();
    ensureArrowButton();
    layoutArrowButton();

    // main box
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_Rect main{ rect.x, rect.y, rect.w, rect.h };
    SDL_RenderFillRect(renderer, &main);
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRect(renderer, &main);

    // selected text
    std::string sel = getSelectedValue();
    renderText(renderer, sel, rect.x + padding, rect.y + (rect.h - fontSize)/2);

    // render arrow button area using Button style (drawn by Button)
    if (arrowButton) {
        arrowButton->render(renderer);
        // Draw triangle icon on top (visual only)
        int cx = arrowRect.x + arrowRect.w/2;
        int cy = arrowRect.y + arrowRect.h/2;
        SDL_Point pts[4];
        if (!expanded) {
            pts[0] = { cx-6, cy-3 }; pts[1] = { cx+6, cy-3 }; pts[2] = { cx, cy+5 }; pts[3] = pts[0];
        } else {
            pts[0] = { cx-6, cy+3 }; pts[1] = { cx+6, cy+3 }; pts[2] = { cx, cy-5 }; pts[3] = pts[0];
        }
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderDrawLines(renderer, pts, 4);
    }

    // list is now drawn in renderOverlay() to ensure it appears above all elements
}

void UIDropdown::renderOverlay(SDL_Renderer* renderer) {
    if (!visible || !expanded) return;
    // Draw dropdown list on top of everything
    SDL_Rect listRect{ rect.x, rect.y + rect.h, rect.w, static_cast<int>(options.size()) * itemHeight };
    SDL_SetRenderDrawColor(renderer, listBgColor.r, listBgColor.g, listBgColor.b, listBgColor.a);
    SDL_RenderFillRect(renderer, &listRect);
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRect(renderer, &listRect);
    for (int i = 0; i < static_cast<int>(options.size()); ++i) {
        SDL_Rect item{ listRect.x, listRect.y + i*itemHeight, listRect.w, itemHeight };
        if (i == hoveredIndex) {
            SDL_SetRenderDrawColor(renderer, hoverColor.r, hoverColor.g, hoverColor.b, hoverColor.a);
            SDL_RenderFillRect(renderer, &item);
        }
        renderText(renderer, options[i], item.x + padding, item.y + (itemHeight - fontSize)/2);
    }
}

void UIDropdown::renderText(SDL_Renderer* renderer, const std::string& text, int x, int y) {
    if (!font || text.empty()) return;
    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), textColor);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }
    SDL_Rect dst{ x, y, surf->w, surf->h };
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}
