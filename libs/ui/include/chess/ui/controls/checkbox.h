#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <SDL.h>
#include <SDL_ttf.h>

#include <functional>
#include <string>
#include "chess/ui/controls/ui/uiElement.h"

class Input;

class UICheckbox : public UIElement {
public:
    UICheckbox(int x, int y, int size,
               const std::string& labelText = "",
               bool checked = false,
               SDL_Color boxColor = {220, 220, 220, 255},
               SDL_Color checkColor = {60, 180, 75, 255},
               SDL_Color borderColor = {80, 80, 80, 255},
               SDL_Color labelColor = {255, 255, 255, 255},
               int fontSize = 20,
               const std::string& fontPath = "");

    ~UICheckbox();

    void update(Input& input) override;
    void render(SDL_Renderer* renderer) override;

    void setChecked(bool value);
    bool isChecked() const { return checked; }

    void setOnChange(std::function<void(bool)> cb) { onChange = std::move(cb); }
    void setBypassCallbacks(bool bypass) { bypassCallbacks = bypass; }

private:
    int size;
    SDL_Color boxColor;
    SDL_Color checkColor;
    SDL_Color borderColor;
    SDL_Color labelColor;
    bool checked;
    bool wasMouseDown = false;
    std::string labelText;
    TTF_Font* font = nullptr;
    int fontSize;
    void loadFont(const std::string& path);
    void renderLabel(SDL_Renderer* renderer, int startX);
    std::function<void(bool)> onChange;
    bool bypassCallbacks = false;
};

#endif // CHECKBOX_H

