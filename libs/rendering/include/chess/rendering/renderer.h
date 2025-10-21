#ifndef RENDERER_H
#define RENDERER_H

#include <SDL.h>

namespace chess {
namespace rendering {
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

#endif // RENDERER_H