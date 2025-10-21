#ifndef TEXTURE_CACHE_H
#define TEXTURE_CACHE_H

#include <SDL.h>
#include <string>
#include <unordered_map>

class TextureCache {
public:
    static void setRenderer(SDL_Renderer* renderer);
    static SDL_Texture* getTexture(const std::string& path);
    static void clear();

private:
    static SDL_Renderer* s_renderer;
    static std::unordered_map<std::string, SDL_Texture*> s_textureMap;
};

#endif // TEXTURE_CACHE_H
