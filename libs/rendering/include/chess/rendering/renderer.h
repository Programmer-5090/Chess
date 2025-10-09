#pragma once
// SDL headers (support both flat and SDL2/ prefixed installations)
#if __has_include(<SDL.h>)
#  include <SDL.h>
#elif __has_include(<SDL2/SDL.h>)
#  include <SDL2/SDL.h>
#else
// Forward declaration fallback for IntelliSense when SDL include paths not yet configured
struct SDL_Renderer;
#endif

namespace chess {
namespace rendering {

// Main renderer interface - base class for all renderers
class Renderer {
public:
    virtual ~Renderer() = default;
    
    virtual bool initialize(SDL_Renderer* sdl_renderer) = 0;
    virtual void render() = 0;
    virtual void cleanup() = 0;
    
protected:
    SDL_Renderer* renderer_ = nullptr;
};

} // namespace rendering
} // namespace chess