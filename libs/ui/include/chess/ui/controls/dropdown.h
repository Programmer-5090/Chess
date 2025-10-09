#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "uiElement.h"
#include "uiCommon.h"
#include "uiButton.h"

class Input;

class UIDropdown : public UIElement {
public:
    UIDropdown(int x, int y, int w, int h,
               std::vector<std::string> options,
               int selectedIndex = 0,
               const std::string& fontPath = "assets/fonts/OpenSans-Regular.ttf",
               int fontSize = 18,
               SDL_Color bg = {230,230,230,255},
               SDL_Color border = {60,60,60,255},
               SDL_Color text = {20,20,20,255},
               SDL_Color hover = {200,200,200,255},
               SDL_Color listBg = {245,245,245,255})
        : UIElement(x,y,w,h), options(std::move(options)), selectedIndex(selectedIndex),
          fontPath(fontPath), fontSize(fontSize), backgroundColor(bg), borderColor(border),
          textColor(text), hoverColor(hover), listBgColor(listBg) {}

    ~UIDropdown();

    void update(Input& input) override;
    void render(SDL_Renderer* renderer) override;
    void renderOverlay(SDL_Renderer* renderer) override;
    bool wantsOutsidePanelInput() const override { return expanded; }
    void onRectChanged() override { /* keep arrow in sync */ ensureArrowButton(); layoutArrowButton(); }

    void setOnChange(std::function<void(int,const std::string&)> cb) { onChange = std::move(cb); }
    int getSelectedIndex() const { return selectedIndex; }
    std::string getSelectedValue() const { return (selectedIndex>=0 && selectedIndex<(int)options.size()) ? options[selectedIndex] : std::string(); }

private:
    std::vector<std::string> options;
    int selectedIndex = 0;
    bool expanded = false;
    int itemHeight = 28;
    int padding = 8;
    // arrow button geometry (clickable area on the right)
    SDL_Rect arrowRect{0,0,0,0};

    // visuals
    SDL_Color backgroundColor;
    SDL_Color borderColor;
    SDL_Color textColor;
    SDL_Color hoverColor;
    SDL_Color listBgColor;

    // font
    std::string fontPath;
    int fontSize;
    TTF_Font* font = nullptr;
    void ensureFont();
    void renderText(SDL_Renderer* renderer, const std::string& text, int x, int y);

    // internal arrow button (uses Button class)
    // Use raw pointer to avoid incomplete-type issues across TUs with unique_ptr default_delete in MSVC
    Button* arrowButton = nullptr;
    void layoutArrowButton();
    void ensureArrowButton();

    // input state
    bool pressedInMain = false;
    bool pressedInMainWhileExpanded = false;
    int pressedItemIndex = -1;
    int hoveredIndex = -1;

    std::function<void(int,const std::string&)> onChange;
};
