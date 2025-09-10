#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <functional>
#include <string>
#include "uiElement.h"
#include "uiCommon.h"

class Input;

class Button : public UIElement {
public:
    Button(int x, int y, int width, int height,
           const std::string& text,
           std::function<void()> callback,
           SDL_Color color = {255, 255, 255, 255},
           SDL_Color hoverColor = {130, 130, 130, 255},
           std::string fontPath = "",
           SDL_Color textColor = {0, 0, 0, 255},
           int elevation = 6,
           int fontSize = 24);
    ~Button();

    void update(Input& input) override;
    void render(SDL_Renderer* renderer) override;

    // Update the button's rectangle at runtime (used when parent layout changes)
    void setRect(int x, int y, int w, int h);

    // Change the on-click callback after construction
    void setCallback(std::function<void()> cb) { callback = std::move(cb); }
    // Change the displayed text after construction
    void setText(const std::string& newText) { text = newText; }
    // Allow this button's callback to bypass the global callbacks gate
    void setBypassCallbackGate(bool bypass) { bypassCallbackGate = bypass; }

    void onRectChanged() override { setRect(rect.x, rect.y, rect.w, rect.h); }

private:
    // data
    std::string text;
    std::function<void()> callback;
    SDL_Color color;
    SDL_Color hoverColor;
    SDL_Color currentColor{};
    SDL_Color bottomColor{};
    SDL_Color textColor;
    TTF_Font* font = nullptr;
    int elevation;
    int dynamicElevation;
    int originalYPos;
    int fontSize;
    bool isPressed = false;
    SDL_Rect topRect{};
    SDL_Rect bottomRect{};
    Uint64 clickCooldownTimestamp = 0;
    bool callbackExecuted = false;
    bool clickStarted = false;
    bool bypassCallbackGate = false;

    // helpers
    void loadFont(const std::string& fontPath);
    void renderText(SDL_Renderer* renderer, const SDL_Rect& buttonRect);
};
