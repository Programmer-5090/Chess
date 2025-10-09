#pragma once

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

// Normalize color alpha
inline SDL_Color normalizeColor(const SDL_Color& color, int alpha = 255) {
    SDL_Color result = color;
    result.a = static_cast<Uint8>(alpha);
    return result;
}

// Convert RGBA ints to SDL_Color
inline SDL_Color tupleToColor(int r, int g, int b, int a = 255) {
    return SDL_Color{static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b), static_cast<Uint8>(a)};
}
