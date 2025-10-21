#ifndef TEXT_INPUT_H
#define TEXT_INPUT_H

#include <string>
#include <functional>
#include <SDL.h>
#include <SDL_ttf.h>
#include "chess/ui/controls/ui/uiElement.h"
#include "chess/ui/controls/ui/uiCommon.h"

class Input;

class UITextInput : public UIElement {
public:
    UITextInput(int x, int y, int w, int h,
                const std::string& placeholder = "",
                const std::string& fontPath = "assets/fonts/OpenSans-Regular.ttf",
                int fontSize = 18,
                SDL_Color bg = { 245,245,245,255 },
                SDL_Color border = { 60,60,60,255 },
                SDL_Color textColor = { 20,20,20,255 },
                SDL_Color placeholderColor = { 140,140,140,255 })
        : UIElement(x, y, w, h), placeholder(placeholder), fontPath(fontPath), fontSize(fontSize),
          backgroundColor(bg), borderColor(border), textColor(textColor), placeholderColor(placeholderColor) {}

    ~UITextInput();

    void update(Input& input) override;
    void render(SDL_Renderer* renderer) override;

    // API
    void setOnSubmit(std::function<void(const std::string&)> cb) { onSubmit = std::move(cb); }
    void setOnChange(std::function<void(const std::string&)> cb) { onChange = std::move(cb); }
    void setText(const std::string& t) { text = t; cursor = (int)text.size(); changedSinceLastRender = true; ensureCaretVisible(); }
    const std::string& getText() const { return text; }
    void setPasswordMode(bool enabled, char mask = '*') { passwordMode = enabled; maskChar = mask; }
    bool isFocused() const { return focused; }
    void blur();

    // Modes
    void setMultiline(bool on) { multiline = on; }
    bool isMultiline() const { return multiline; }

private:
    // data
    std::string text;
    std::string placeholder;
    bool focused = false;
    int cursor = 0;
    int padding = 8;
    bool passwordMode = false;
    char maskChar = '*';
    bool changedSinceLastRender = false;

    // visuals
    SDL_Color backgroundColor;
    SDL_Color borderColor;
    SDL_Color textColor;
    SDL_Color placeholderColor;

    // font
    std::string fontPath;
    int fontSize;
    TTF_Font* font = nullptr;
    void ensureFont();

    // caret blink
    Uint64 lastBlink = 0;
    bool caretVisible = true;

    // helpers
    void insertText(const std::string& s);
    void backspace();
    void moveCursorLeft();
    void moveCursorRight();
    int measureTextWidth(const std::string& s) const;
    void ensureTextInputStarted();
    void ensureCaretVisible();

    // callbacks
    std::function<void(const std::string&)> onSubmit;
    std::function<void(const std::string&)> onChange;

    // horizontal scroll (to keep caret visible when text exceeds width)
    int scrollOffsetPx = 0; // pixels scrolled to the left

    // multiline state (textbox mode)
    bool multiline = false;
    int firstVisibleLine = 0;
    int lineHeightPx = 0;
    void renderMultiline(SDL_Renderer* renderer, const SDL_Rect& inner);
    void updateLineMetrics();
};

#endif // TEXT_INPUT_H

