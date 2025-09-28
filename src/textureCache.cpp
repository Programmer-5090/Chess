#include "textureCache.h"
#include "perfProfiler.h"
#include <SDL_image.h>
#include <iostream>

SDL_Renderer* TextureCache::s_renderer = nullptr;
std::unordered_map<std::string, SDL_Texture*> TextureCache::s_cache;
std::mutex TextureCache::s_mutex;

void TextureCache::init(SDL_Renderer* renderer) {
    std::lock_guard<std::mutex> lk(s_mutex);
    s_renderer = renderer;
}

SDL_Texture* TextureCache::getTexture(const std::string& path) {
    std::lock_guard<std::mutex> lk(s_mutex);
    if (!s_renderer) return nullptr;
    auto it = s_cache.find(path);
    if (it != s_cache.end()) return it->second;

    g_profiler.startTimer("IMG_Load");
    SDL_Surface* surf = IMG_Load(path.c_str());
    g_profiler.endTimer("IMG_Load");
    if (!surf) {
        // Failed to load image
        return nullptr;
    }

    g_profiler.startTimer("SDL_CreateTextureFromSurface");
    SDL_Texture* tex = SDL_CreateTextureFromSurface(s_renderer, surf);
    g_profiler.endTimer("SDL_CreateTextureFromSurface");

    SDL_FreeSurface(surf);

    if (!tex) {
        return nullptr;
    }

    s_cache[path] = tex;
    return tex;
}

void TextureCache::clear() {
    std::lock_guard<std::mutex> lk(s_mutex);
    for (auto &p : s_cache) {
        if (p.second) SDL_DestroyTexture(p.second);
    }
    s_cache.clear();
}
