#include "chess/ui/controls/slider.h"
#include "chess/ui/input.h"
#include "chess/ui/controls/ui/uiConfig.h"
#include <algorithm>

void UISlider::setRange(double minV, double maxV) {
    if (maxV < minV) std::swap(minV, maxV);
    minVal = minV;
    maxVal = maxV;
    setValue(value);
}

void UISlider::setValue(double v) {
    value = clamp(v, minVal, maxVal);
    if (onChange && UIConfig::areCallbacksEnabled()) onChange(value);
}

int UISlider::thumbXFromValue() const {
    double t = (value - minVal) / (maxVal - minVal);
    t = clamp(t, 0.0, 1.0);
    int left = rect.x + 8;
    int right = rect.x + rect.w - 8;
    return left + static_cast<int>(t * (right - left));
}

void UISlider::updateFromMouseX(int mx) {
    int left = rect.x + 8;
    int right = rect.x + rect.w - 8;
    mx = clampInt(mx, left, right);
    double t = (right == left) ? 0.0 : (static_cast<double>(mx - left) / (right - left));
    setValue(minVal + t * (maxVal - minVal));
}

void UISlider::update(Input& input) {
    if (!visible) return;
    int mx = input.getMouseX();
    int my = input.getMouseY();
    bool mouseDown = input.getMouseStates()["left"];

    SDL_Rect trackRect{ rect.x, rect.y + rect.h/2 - 3, rect.w, 6 };
    // Thumb is a small rectangle centered around the computed thumb x
    int tx = thumbXFromValue();
    SDL_Rect thumbRect{ tx - 8, rect.y + rect.h/2 - 10, 16, 20 };

    auto hit = [&](const SDL_Rect& r){ return mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h; };

    if (mouseDown && !prevMouseDown) {
        if (hit(thumbRect) || hit(trackRect)) {
            dragging = true;
            updateFromMouseX(mx);
        }
    } else if (!mouseDown && prevMouseDown) {
        dragging = false;
    } else if (mouseDown && dragging) {
        updateFromMouseX(mx);
    }

    prevMouseDown = mouseDown;
}

void UISlider::render(SDL_Renderer* renderer) {
    if (!visible) return;

    // Track background
    SDL_Rect trackRect{ rect.x, rect.y + rect.h/2 - 3, rect.w, 6 };
    SDL_SetRenderDrawColor(renderer, trackColor.r, trackColor.g, trackColor.b, trackColor.a);
    SDL_RenderFillRect(renderer, &trackRect);
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRect(renderer, &rect);

    // Filled portion
    int tx = thumbXFromValue();
    SDL_Rect filled{ rect.x, trackRect.y, tx - rect.x, trackRect.h };
    SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
    SDL_RenderFillRect(renderer, &filled);

    // Thumb
    SDL_Rect thumbRect{ tx - 8, rect.y + rect.h/2 - 10, 16, 20 };
    SDL_SetRenderDrawColor(renderer, thumbColor.r, thumbColor.g, thumbColor.b, thumbColor.a);
    SDL_RenderFillRect(renderer, &thumbRect);
    SDL_SetRenderDrawColor(renderer, thumbBorder.r, thumbBorder.g, thumbBorder.b, thumbBorder.a);
    SDL_RenderDrawRect(renderer, &thumbRect);
}
