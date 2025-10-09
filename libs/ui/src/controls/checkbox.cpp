#include "chess/ui/controls/checkbox.h"
#include "chess/ui/controls/ui/uiConfig.h"
#include "chess/ui/input.h"
#include <iostream>
#include <chess/utils/logger.h>

UICheckbox::UICheckbox(int x, int y, int size,
                       const std::string& labelText,
                       bool checked,
                       SDL_Color boxColor,
                       SDL_Color checkColor,
                       SDL_Color borderColor,
                       SDL_Color labelColor,
                       int fontSize,
                       const std::string& fontPath)
    : UIElement(x, y, size, size), size(size),
      boxColor(boxColor), checkColor(checkColor), borderColor(borderColor),
      labelColor(labelColor), checked(checked), labelText(labelText),
      fontSize(fontSize) {
    loadFont(fontPath);
}

UICheckbox::~UICheckbox() {
    if (font) TTF_CloseFont(font);
}

void UICheckbox::loadFont(const std::string& path) {
    if (!TTF_WasInit() && TTF_Init() == -1) {
        Logger::log(LogLevel::ERROR, std::string("SDL_ttf could not initialize! SDL_ttf Error: ") + TTF_GetError(), __FILE__, __LINE__);
        return;
    }
    if (!path.empty()) {
        font = TTF_OpenFont(path.c_str(), fontSize);
        if (!font) {
            Logger::log(LogLevel::WARN, std::string("Failed to load font: ") + path + std::string(" SDL_ttf Error: ") + TTF_GetError(), __FILE__, __LINE__);
        }
    }
}

void UICheckbox::setChecked(bool value) {
    if (checked != value) {
        checked = value;
        if (onChange && (bypassCallbacks || UIConfig::areCallbacksEnabled())) onChange(checked);
    }
}

void UICheckbox::update(Input& input) {
    if (!visible) return;
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    bool inside = mouseX >= rect.x && mouseX <= rect.x + rect.w &&
                  mouseY >= rect.y && mouseY <= rect.y + rect.h;
    bool down = input.getMouseStates()["left"];
    if (inside && down && !wasMouseDown) {
        setChecked(!checked);
    }
    wasMouseDown = down;
}

void UICheckbox::render(SDL_Renderer* renderer) {
    if (!visible) return;
    // box
    SDL_SetRenderDrawColor(renderer, boxColor.r, boxColor.g, boxColor.b, boxColor.a);
    SDL_RenderFillRect(renderer, &rect);
    // border
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRect(renderer, &rect);
    // check mark (simple filled smaller rect)
    if (checked) {
        SDL_Rect in{ rect.x + rect.w/6, rect.y + rect.h/6, rect.w - rect.w/3, rect.h - rect.h/3 };
        SDL_SetRenderDrawColor(renderer, checkColor.r, checkColor.g, checkColor.b, checkColor.a);
        SDL_RenderFillRect(renderer, &in);
    }
    // label
    if (!labelText.empty() && font) {
        renderLabel(renderer, rect.x + rect.w + 10);
    }
}

void UICheckbox::renderLabel(SDL_Renderer* renderer, int startX) {
    SDL_Surface* surf = TTF_RenderText_Blended(font, labelText.c_str(), labelColor);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }
    SDL_Rect dst{ startX, rect.y + (rect.h - surf->h)/2, surf->w, surf->h };
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}
