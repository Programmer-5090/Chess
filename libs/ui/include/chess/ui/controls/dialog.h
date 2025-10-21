#ifndef DIALOG_H
#define DIALOG_H

#include <string>
#include <functional>
#include "chess/ui/controls/ui/uiCommon.h"
#include "chess/ui/controls/ui/uiElement.h"

// Forward Declaration
class Input;
class Button;

class UIDialog : public UIElement {
public:
    UIDialog(int x, int y, int w, int h,
             const std::string& title,
             const std::string& message,
             const std::string& okText = "OK",
             const std::string& cancelText = "Cancel",
             const std::string& fontPath = "assets/fonts/OpenSans-Regular.ttf",
             int fontSize = 20,
             SDL_Color overlay = {0, 0, 0, 180},
             SDL_Color bg = {35, 35, 45, 255},
             SDL_Color border = {70, 70, 90, 255},
             SDL_Color textColor = {255, 255, 255, 255},
             SDL_Color btnBg = {220, 220, 220, 255},
             SDL_Color btnText = {20, 20, 20, 255});

    ~UIDialog();

    void update(Input& input) override;
    void render(SDL_Renderer* renderer) override;
    bool isModal() const override { return visible; }

    void setOnOk(std::function<void()> cb) { onOk = std::move(cb); }
    void setOnCancel(std::function<void()> cb) { onCancel = std::move(cb); }

private:
    std::string title;
    std::string message;
    std::string okText;
    std::string cancelText;
    SDL_Color overlayColor;
    SDL_Color backgroundColor;
    SDL_Color borderColor;
    SDL_Color textColor;
    SDL_Color buttonBgColor;
    SDL_Color buttonTextColor;
    int padding = 16;
    int buttonHeight = 36;
    int buttonWidth = 100;
    int spacing = 10;
    Button* okButton = nullptr;
    Button* cancelButton = nullptr;
    TTF_Font* font = nullptr;
    int fontSize;
    std::string fontPath;
    void ensureFont();
    void renderText(SDL_Renderer* renderer, const std::string& text, int x, int y, bool centerX = false);
    void createButtons();
    void layoutButtons();
    std::function<void()> onOk;
    std::function<void()> onCancel;
};

#endif // DIALOG_H

