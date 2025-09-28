#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <SDL.h>

class TextureCache {
public:
    // Initialize with an SDL_Renderer (must be called before getTexture)
    static void init(SDL_Renderer* renderer);

    // Returns a cached texture for the given image path. Loads on first use.
    // Returns nullptr on failure.
    static SDL_Texture* getTexture(const std::string& path);

    // Clears and destroys all cached textures (call on shutdown)
    static void clear();

private:
    static SDL_Renderer* s_renderer;
    static std::unordered_map<std::string, SDL_Texture*> s_cache;
    static std::mutex s_mutex;
};
