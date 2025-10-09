#include <chess/rendering/texture_cache.h>
#include <SDL_image.h>
#include <chess/utils/logger.h>

SDL_Renderer* TextureCache::s_renderer = nullptr;
std::unordered_map<std::string, SDL_Texture*> TextureCache::s_textureMap;

void TextureCache::setRenderer(SDL_Renderer* renderer) {
    s_renderer = renderer;
    Logger::log(LogLevel::DEBUG, 
        std::string("TextureCache::setRenderer called with renderer=") + (renderer ? "valid" : "nullptr"),
        __FILE__, __LINE__);
    // Clear any previously cached textures when renderer changes
    clear();
}

SDL_Texture* TextureCache::getTexture(const std::string& path) {
    // Don't attempt to load if no renderer is set
    if (!s_renderer) {
        Logger::log(LogLevel::WARN, 
            "TextureCache::getTexture called but no renderer set for: " + path,
            __FILE__, __LINE__);
        return nullptr;
    }
    
    // Check if texture is already cached
    auto it = s_textureMap.find(path);
    if (it != s_textureMap.end()) {
        Logger::log(LogLevel::DEBUG, "Texture cache hit: " + path, __FILE__, __LINE__);
        return it->second;
    }
    
    // Load new texture
    SDL_Texture* texture = IMG_LoadTexture(s_renderer, path.c_str());
    if (!texture) {
        Logger::log(LogLevel::ERROR, 
            "Failed to load texture: " + path + " - " + SDL_GetError(),
            __FILE__, __LINE__);
        return nullptr;
    }
    
    // Cache and return
    s_textureMap[path] = texture;
    Logger::log(LogLevel::DEBUG, "Loaded texture: " + path, __FILE__, __LINE__);
    return texture;
}

void TextureCache::clear() {
    for (auto& pair : s_textureMap) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    s_textureMap.clear();
}
