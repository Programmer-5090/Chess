#pragma once

// SDL headers (support both flat and SDL2/ prefixed installations)
#if __has_include(<SDL.h>)
#  include <SDL.h>
#else
#  include <SDL2/SDL.h>
#endif
#if __has_include(<SDL_ttf.h>)
#  include <SDL_ttf.h>
#else
#  include <SDL2/SDL_ttf.h>
#endif
#include <string>
#include "chess/ui/controls/ui/uiElement.h"

class Label : public UIElement {
public:
    Label(int x, int y, const std::string& text,
          SDL_Color color = {255, 255, 255, 255},
          int fontSize = 20, std::string fontPath = "");
    ~Label();

    void render(SDL_Renderer* renderer) override;
    void setText(const std::string& newText);

private:
    std::string text;
    SDL_Color color;
    TTF_Font* font = nullptr;
    int fontSize;

    void loadFont(const std::string& fontPath);
    void updateTextDimensions();
};
