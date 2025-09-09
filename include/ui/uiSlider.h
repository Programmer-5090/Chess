#ifndef UI_SLIDER_H
#define UI_SLIDER_H

#include "uiElement.h"
#include <algorithm>
#include <cmath>

namespace UI {

/**
 * Slider UI element with customizable appearance
 */
class Slider : public UIElement {
public:
    Slider(const SDL_FRect& rect, float min, float max, float value)
        : UIElement(rect), minValue(min), maxValue(max), currentValue(value) {
        // Ensure initial value is within bounds
        currentValue = std::clamp(currentValue, minValue, maxValue);
        // Calculate initial handle position
        updateHandlePosition();
    }
    
    ~Slider() override {
        cleanup();
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        
        // Calculate track height
        float trackHeight = std::max(2.0f, rect.h / 4.0f);
        float trackY = rect.y + (rect.h - trackHeight) / 2.0f;
        
        // Draw background track
        SDL_FRect trackRect = {
            rect.x, trackY, rect.w, trackHeight
        };
        
        SDL_SetRenderDrawColor(renderer, trackColor.r, trackColor.g, trackColor.b, trackColor.a);
        SDL_RenderFillRectF(renderer, &trackRect);
        
        // Draw progress/filled part
        SDL_FRect filledRect = {
            rect.x, trackY, handlePosition - rect.x, trackHeight
        };
        
        SDL_SetRenderDrawColor(renderer, progressColor.r, progressColor.g, 
                              progressColor.b, progressColor.a);
        SDL_RenderFillRectF(renderer, &filledRect);
        
        // Draw handle
        float handleSize = rect.h * 0.8f;
        SDL_FRect handleRect = {
            handlePosition - handleSize / 2.0f,
            rect.y + (rect.h - handleSize) / 2.0f,
            handleSize,
            handleSize
        };
        
        // Determine handle color based on state
        SDL_Color hColor;
        switch (state) {
            case UIState::NORMAL:
                hColor = handleColor;
                break;
            case UIState::HOVER:
                hColor = handleHoverColor;
                break;
            case UIState::ACTIVE:
                hColor = handleActiveColor;
                break;
            case UIState::DISABLED:
                hColor = handleDisabledColor;
                break;
        }
        
        SDL_SetRenderDrawColor(renderer, hColor.r, hColor.g, hColor.b, hColor.a);
        SDL_RenderFillRectF(renderer, &handleRect);
        
        // Draw handle border
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        SDL_RenderDrawRectF(renderer, &handleRect);
        
        // Draw value label if enabled
        if (showValue && valueFont) {
            if (!valueTexture || valueChanged) {
                freeTexture(valueTexture);
                std::string valueText = formatValue(currentValue);
                valueTexture = renderTextToTexture(renderer, valueText, valueFont, textColor);
                valueChanged = false;
            }
            
            if (valueTexture) {
                // Position the value label above the slider
                int textW, textH;
                SDL_QueryTexture(valueTexture, nullptr, nullptr, &textW, &textH);
                
                SDL_FRect labelRect = {
                    handlePosition - textW / 2.0f,
                    rect.y - textH - 5.0f, // 5 pixels above the slider
                    static_cast<float>(textW),
                    static_cast<float>(textH)
                };
                
                SDL_RenderCopyF(renderer, valueTexture, nullptr, &labelRect);
            }
        }
    }
    
    void update(Input& input) override {
        if (!visible || state == UIState::DISABLED) return;
        
        float mouseX = static_cast<float>(input.getMouseX());
        float mouseY = static_cast<float>(input.getMouseY());
        
        // Handle size for hit testing
        float handleSize = rect.h * 0.8f;
        bool onHandle = std::abs(mouseX - handlePosition) <= handleSize / 2.0f && 
                        mouseY >= rect.y && mouseY <= rect.y + rect.h;
        
        bool onTrack = mouseX >= rect.x && mouseX <= rect.x + rect.w && 
                       mouseY >= rect.y && mouseY <= rect.y + rect.h;
        
        // Update state based on mouse interaction
        if (state == UIState::ACTIVE) {
            if (input.isMouseButtonDown(SDL_BUTTON_LEFT)) {
                // Constrain handle position to slider track
                float newX = std::clamp(mouseX, rect.x, rect.x + rect.w);
                
                // Update handle position and value
                handlePosition = newX;
                updateValueFromPosition();
                
                valueChanged = true;
                
                // Trigger value change callback
                if (onValueChanged) {
                    onValueChanged(currentValue);
                }
            } else {
                // Mouse released
                state = onHandle ? UIState::HOVER : UIState::NORMAL;
            }
        } else {
            if (onHandle) {
                state = UIState::HOVER;
                
                if (input.isMouseButtonPressed(SDL_BUTTON_LEFT)) {
                    state = UIState::ACTIVE;
                }
            } else if (onTrack) {
                if (input.isMouseButtonPressed(SDL_BUTTON_LEFT)) {
                    // Jump to clicked position
                    handlePosition = mouseX;
                    updateValueFromPosition();
                    
                    valueChanged = true;
                    state = UIState::ACTIVE;
                    
                    // Trigger value change callback
                    if (onValueChanged) {
                        onValueChanged(currentValue);
                    }
                }
            } else {
                state = UIState::NORMAL;
            }
        }
    }
    
    void cleanup() override {
        freeTexture(valueTexture);
    }
    
    // Setters & Getters
    float getValue() const {
        return currentValue;
    }
    
    void setValue(float value) {
        float oldValue = currentValue;
        currentValue = std::clamp(value, minValue, maxValue);
        
        if (currentValue != oldValue) {
            updateHandlePosition();
            valueChanged = true;
            
            if (onValueChanged) {
                onValueChanged(currentValue);
            }
        }
    }
    
    void setRange(float min, float max) {
        minValue = min;
        maxValue = max;
        
        // Make sure current value is still in range
        setValue(currentValue);
    }
    
    void setShowValue(bool show) {
        showValue = show;
    }
    
    void setValueFont(TTF_Font* font) {
        valueFont = font;
        valueChanged = true;
    }
    
    void setOnValueChanged(std::function<void(float)> callback) {
        onValueChanged = callback;
    }
    
    void setStep(float step) {
        this->step = step;
    }
    
    void setPrecision(int precision) {
        this->precision = precision;
        valueChanged = true;
    }
    
    void setColors(const SDL_Color& track, const SDL_Color& progress,
                   const SDL_Color& handle, const SDL_Color& handleHover,
                   const SDL_Color& handleActive, const SDL_Color& handleDisabled,
                   const SDL_Color& border, const SDL_Color& text) {
        trackColor = track;
        progressColor = progress;
        handleColor = handle;
        handleHoverColor = handleHover;
        handleActiveColor = handleActive;
        handleDisabledColor = handleDisabled;
        borderColor = border;
        textColor = text;
    }

private:
    void updateHandlePosition() {
        float valueRange = maxValue - minValue;
        if (valueRange == 0) {
            handlePosition = rect.x;
            return;
        }
        
        float percentage = (currentValue - minValue) / valueRange;
        handlePosition = rect.x + percentage * rect.w;
    }
    
    void updateValueFromPosition() {
        float percentage = (handlePosition - rect.x) / rect.w;
        float rawValue = minValue + percentage * (maxValue - minValue);
        
        // Apply step if it's greater than 0
        if (step > 0) {
            rawValue = std::round(rawValue / step) * step;
        }
        
        currentValue = std::clamp(rawValue, minValue, maxValue);
    }
    
    std::string formatValue(float value) {
        std::string format = "%." + std::to_string(precision) + "f";
        char buffer[32];
        snprintf(buffer, sizeof(buffer), format.c_str(), value);
        return std::string(buffer);
    }

    float minValue;
    float maxValue;
    float currentValue;
    float handlePosition;
    float step = 0.0f; // 0 means continuous slider
    int precision = 1; // Decimal places to show
    
    bool showValue = true;
    bool valueChanged = true;
    TTF_Font* valueFont = nullptr;
    SDL_Texture* valueTexture = nullptr;
    
    // Colors
    SDL_Color trackColor = {80, 80, 80, 255};
    SDL_Color progressColor = {0, 120, 215, 255};
    SDL_Color handleColor = {200, 200, 200, 255};
    SDL_Color handleHoverColor = {220, 220, 220, 255};
    SDL_Color handleActiveColor = {240, 240, 240, 255};
    SDL_Color handleDisabledColor = {120, 120, 120, 128};
    SDL_Color borderColor = {40, 40, 40, 255};
    SDL_Color textColor = {255, 255, 255, 255};
    
    // Callback
    std::function<void(float)> onValueChanged;
};

} // namespace UI

#endif // UI_SLIDER_H
