#include "chess/ui/manager.h" // may not be in include path; fallback to relative if build still fails
#include "chess/ui/controls/ui/uiElement.h"
#include "chess/ui/input.h"
#include <iostream>

UIManager::UIManager(SDL_Renderer* renderer, int screenWidth, int screenHeight)
    : renderer(renderer), screenWidth(screenWidth), screenHeight(screenHeight) {
    uiSurface = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32,
                                     0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (uiSurface == nullptr) {
        std::cout << "UI Surface could not be created! SDL Error: " << SDL_GetError() << std::endl;
    }

    uiTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_TARGET, screenWidth, screenHeight);
    if (uiTexture == nullptr) {
        std::cout << "UI Texture could not be created! SDL Error: " << SDL_GetError() << std::endl;
    }

    SDL_SetTextureBlendMode(uiTexture, SDL_BLENDMODE_BLEND);
}

UIManager::~UIManager() {
    for (auto element : elements) {
        delete element;
    }
    elements.clear();

    if (uiSurface != nullptr) {
        SDL_FreeSurface(uiSurface);
    }
    if (uiTexture != nullptr) {
        SDL_DestroyTexture(uiTexture);
    }
}

void UIManager::update(Input& input) {
    int topModal = getTopModalIndex();
    size_t start = 0;
    if (topModal >= 0) {
        // When a modal exists, only update that modal and any elements after it
        start = static_cast<size_t>(topModal);
    }

    // Iterate through all SDL events captured this frame so elements see SDL_TEXTINPUT, KEYDOWN, etc.
    const auto& evs = input.getEvents();
    if (!evs.empty()) {
        for (const auto& e : evs) {
            input.setCurrentEvent(e);
            for (size_t i = start; i < elements.size(); ++i) {
                auto* element = elements[i];
                if (element->visible) element->update(input);
            }
        }
    } else {
        // No events this frame; still allow elements to update hover/pressed states, etc.
        for (size_t i = start; i < elements.size(); ++i) {
            auto* element = elements[i];
            if (element->visible) element->update(input);
        }
    }
}

void UIManager::render() {
    SDL_SetRenderTarget(renderer, uiTexture);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    for (auto element : elements) {
        if (element->visible) element->render(renderer);
    }

    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawCircle(renderer, mouseX, mouseY, 10);

    // Draw overlays (e.g., dropdown lists) above everything after base pass
    for (auto element : elements) {
        if (element->visible) element->renderOverlay(renderer);
    }

    SDL_SetRenderTarget(renderer, nullptr);

    SDL_RenderCopy(renderer, uiTexture, nullptr, nullptr);
}

int UIManager::getTopModalIndex() const {
    for (int i = static_cast<int>(elements.size()) - 1; i >= 0; --i) {
        if (elements[i]->visible && elements[i]->isModal()) return i;
    }
    return -1;
}

void UIManager::clearElements() {
    auto elementsCopy = elements;
    elements.clear();
    for (auto element : elementsCopy) {
        delete element;
    }
}

void UIManager::SDL_RenderDrawCircle(SDL_Renderer* sdlRenderer, int x, int y, int radius) {
    const int diameter = radius * 2;

    int x_pos = radius - 1;
    int y_pos = 0;
    int tx = 1;
    int ty = 1;
    int error = tx - diameter;

    while (x_pos >= y_pos) {
    SDL_RenderDrawPoint(sdlRenderer, x + x_pos, y - y_pos);
    SDL_RenderDrawPoint(sdlRenderer, x + x_pos, y + y_pos);
    SDL_RenderDrawPoint(sdlRenderer, x - x_pos, y - y_pos);
    SDL_RenderDrawPoint(sdlRenderer, x - x_pos, y + y_pos);
    SDL_RenderDrawPoint(sdlRenderer, x + y_pos, y - x_pos);
    SDL_RenderDrawPoint(sdlRenderer, x + y_pos, y + x_pos);
    SDL_RenderDrawPoint(sdlRenderer, x - y_pos, y - x_pos);
    SDL_RenderDrawPoint(sdlRenderer, x - y_pos, y + x_pos);

        if (error <= 0) {
            y_pos++;
            error += ty;
            ty += 2;
        }

        if (error > 0) {
            x_pos--;
            tx += 2;
            error += tx - diameter;
        }
    }
}
