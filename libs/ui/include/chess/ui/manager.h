#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <SDL.h>
#include <vector>
#include "controls/ui/uiConfig.h"
#include "controls/ui/uiElement.h"

class Input;

class UIManager {
public:
    UIManager(SDL_Renderer* renderer, int screenWidth, int screenHeight);
    ~UIManager();

    template<typename T, typename... Args>
    T* addElement(Args&&... args) {
        T* element = new T(std::forward<Args>(args)...);
        elements.push_back(element);
        return element;
    }

    void update(Input& input);
    void render();
    void clearElements();
    // Global callbacks gate helpers
    void setCallbacksEnabled(bool enabled) { UIConfig::setCallbacksEnabled(enabled); }
    bool areCallbacksEnabled() const { return UIConfig::areCallbacksEnabled(); }
    // Find last modal index (top-most) or -1 if none
    int getTopModalIndex() const;

private:
    SDL_Renderer* renderer;
    int screenWidth;
    int screenHeight;
    SDL_Surface* uiSurface = nullptr;
    SDL_Texture* uiTexture = nullptr;
    std::vector<UIElement*> elements;

    void SDL_RenderDrawCircle(SDL_Renderer* sdlRenderer, int x, int y, int radius);
};

#endif // UI_MANAGER_H
