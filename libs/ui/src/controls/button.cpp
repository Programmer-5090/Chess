#include "chess/ui/controls/button.h"
#include "chess/ui/controls/ui/uiCommon.h"
#include "chess/ui/controls/ui/uiConfig.h"
#include "chess/ui/input.h"
#include <algorithm>
#include <iostream>
#include <chess/utils/logger.h>

Button::Button(int x, int y, int width, int height, const std::string& text,
               std::function<void()> callback,
               SDL_Color color, SDL_Color hoverColor,
               std::string fontPath, SDL_Color textColor,
               int elevation, int fontSize)
    : UIElement(x, y, width, height),
      text(text), callback(callback), color(color), hoverColor(hoverColor),
      textColor(textColor), elevation(elevation), dynamicElevation(elevation),
      originalYPos(y), fontSize(fontSize) {

    topRect = {x, y - elevation, width, height};
    bottomRect = {x, y, width, height};

    loadFont(fontPath);

    int mouseX_init, mouseY_init;
    uint32_t mouseState_init = SDL_GetMouseState(&mouseX_init, &mouseY_init);
    bool hover_init = (mouseX_init >= rect.x && mouseX_init <= rect.x + rect.w &&
                       mouseY_init >= originalYPos - elevation &&
                       mouseY_init <= originalYPos - elevation + rect.h);
    if (hover_init) {
        currentColor = hoverColor;
        if (mouseState_init & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            isPressed = true;
            dynamicElevation = 0;
        }
    } else {
        currentColor = color;
    }

    bottomColor = {
        static_cast<Uint8>(std::max(0, currentColor.r - 40)),
        static_cast<Uint8>(std::max(0, currentColor.g - 40)),
        static_cast<Uint8>(std::max(0, currentColor.b - 40)),
        currentColor.a
    };

    clickCooldownTimestamp = SDL_GetTicks64();
}

Button::~Button() {
    if (font != nullptr) {
        TTF_CloseFont(font);
    }
}

void Button::update(Input& input) {
    if (!visible) return;

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    bool hover = (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
                  mouseY >= rect.y - dynamicElevation &&
                  mouseY <= rect.y - dynamicElevation + rect.h);

    bool mouseIsCurrentlyDown = input.getMouseStates()["left"];

    if (isPressed || hover) {
        currentColor = hoverColor;
    } else {
        currentColor = color;
    }

    bottomColor = {
        static_cast<Uint8>(std::max(0, currentColor.r - 40)),
        static_cast<Uint8>(std::max(0, currentColor.g - 40)),
        static_cast<Uint8>(std::max(0, currentColor.b - 40)),
        currentColor.a
    };

    if (hover) {
        if (mouseIsCurrentlyDown) {
            if (!isPressed) {
                Uint64 currentTime = SDL_GetTicks64();
                if (currentTime >= clickCooldownTimestamp) {
                    isPressed = true;
                    dynamicElevation = 0;
                    clickCooldownTimestamp = currentTime + 200;
                }
            }
        } else {
            if (isPressed) {
                isPressed = false;
                dynamicElevation = elevation;
            }
        }
    } else {
        if (isPressed) {
            isPressed = false;
            dynamicElevation = elevation;
        }
    }

    topRect.y = originalYPos - dynamicElevation;

    if (isPressed && mouseIsCurrentlyDown && hover) {
        clickStarted = true;
        callbackExecuted = false;
    } else if (clickStarted && hover && !mouseIsCurrentlyDown && !callbackExecuted) {
        callbackExecuted = true;
        clickStarted = false;
        if (callback && (bypassCallbackGate || UIConfig::areCallbacksEnabled())) callback();
    } else if (!hover || !mouseIsCurrentlyDown) {
        clickStarted = false;
    }
}

void Button::render(SDL_Renderer* renderer) {
    if (!visible) return;

    if (dynamicElevation > 0) {
        SDL_SetRenderDrawColor(renderer, bottomColor.r, bottomColor.g, bottomColor.b, bottomColor.a);
        SDL_RenderFillRect(renderer, &bottomRect);
    }

    SDL_Rect adjustedTopRect = { topRect.x, topRect.y, topRect.w, topRect.h };
    SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);
    SDL_RenderFillRect(renderer, &adjustedTopRect);

    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderDrawRect(renderer, &adjustedTopRect);

    if (font != nullptr) {
        renderText(renderer, adjustedTopRect);
    }
}

void Button::loadFont(const std::string& fontPath) {
    // If the button has no text, skip loading a font to avoid noisy errors (e.g., icon-only buttons)
    if (text.empty()) {
        font = nullptr;
        return;
    }

    if (!TTF_WasInit() && TTF_Init() == -1) {
        Logger::log(LogLevel::ERROR, std::string("SDL_ttf could not initialize! SDL_ttf Error: ") + TTF_GetError(), __FILE__, __LINE__);
        return;
    }

    if (!fontPath.empty()) {
        font = TTF_OpenFont(fontPath.c_str(), fontSize);
        if (font == nullptr) {
            Logger::log(LogLevel::WARN, std::string("Failed to load font: ") + fontPath + std::string(" SDL_ttf Error: ") + TTF_GetError(), __FILE__, __LINE__);
        }
    }

    // If still not loaded, try a sensible project-default fallback present in both build/runtime dirs
    if (font == nullptr) {
        font = TTF_OpenFont("assets/fonts/OpenSans-Regular.ttf", fontSize);
    }

    if (font == nullptr) {
        Logger::log(LogLevel::ERROR, std::string("Failed to load any font! SDL_ttf Error: ") + TTF_GetError(), __FILE__, __LINE__);
    }
}

void Button::renderText(SDL_Renderer* renderer, const SDL_Rect& buttonRect) {
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), textColor);
    if (textSurface == nullptr) {
        Logger::log(LogLevel::ERROR, std::string("Unable to render text surface! SDL_ttf Error: ") + TTF_GetError(), __FILE__, __LINE__);
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == nullptr) {
        Logger::log(LogLevel::ERROR, std::string("Unable to create texture from rendered text! SDL Error: ") + SDL_GetError(), __FILE__, __LINE__);
        SDL_FreeSurface(textSurface);
        return;
    }
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    SDL_Rect renderQuad = {
        buttonRect.x + (buttonRect.w - textWidth) / 2,
        buttonRect.y + (buttonRect.h - textHeight) / 2,
        textWidth,
        textHeight
    };
    SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

void Button::setRect(int x, int y, int w, int h) {
    // Update geometry and dependent rects while preserving current elevation state
    rect = { x, y, w, h };
    originalYPos = y;
    // dynamicElevation may be 0 when pressed; topRect should reflect that
    topRect = { x, y - dynamicElevation, w, h };
    bottomRect = { x, y, w, h };
}
