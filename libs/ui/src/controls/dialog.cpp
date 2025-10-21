#include "chess/ui/controls/dialog.h"
#include "chess/ui/controls/button.h"
#include "chess/ui/input.h"
#include <iostream>
#include <chess/utils/logger.h>

UIDialog::UIDialog(int x, int y, int w, int h,
                   const std::string& title,
                   const std::string& message,
                   const std::string& okText,
                   const std::string& cancelText,
                   const std::string& fontPath,
                   int fontSize,
                   SDL_Color overlay,
                   SDL_Color bg,
                   SDL_Color border,
                   SDL_Color textColor,
                   SDL_Color btnBg,
                   SDL_Color btnText)
    : UIElement(x, y, w, h), title(title), message(message), okText(okText),
      cancelText(cancelText), overlayColor(overlay), backgroundColor(bg), borderColor(border),
      textColor(textColor), buttonBgColor(btnBg), buttonTextColor(btnText), fontSize(fontSize),
      fontPath(fontPath) {
    createButtons();
}

UIDialog::~UIDialog() {
    if (font) TTF_CloseFont(font);
    delete okButton;
    delete cancelButton;
}

void UIDialog::ensureFont() {
    if (!font) {
        if (!TTF_WasInit()) TTF_Init();
        font = TTF_OpenFont(fontPath.c_str(), fontSize);
        if (!font) {
            Logger::log(LogLevel::ERROR, std::string("Dialog font load failed: ") + TTF_GetError(), __FILE__, __LINE__);
        }
    }
}

void UIDialog::createButtons() {
    int okX = rect.x + rect.w - padding - buttonWidth;
    int okY = rect.y + rect.h - padding - buttonHeight;
    int cancelX = okX - spacing - buttonWidth;
    int cancelY = okY;

    okButton = new Button(okX, okY, buttonWidth, buttonHeight, okText, [this]() {
        if (onOk) onOk();
        this->visible = false;
    }, buttonBgColor, buttonBgColor, fontPath, buttonTextColor, 4, 20);

    cancelButton = new Button(cancelX, cancelY, buttonWidth, buttonHeight, cancelText, [this]() {
        if (onCancel) onCancel();
        this->visible = false;
    }, buttonBgColor, buttonBgColor, fontPath, buttonTextColor, 4, 20);
}

void UIDialog::layoutButtons() {
    if (!okButton || !cancelButton) return;
    int okX = rect.x + rect.w - padding - buttonWidth;
    int okY = rect.y + rect.h - padding - buttonHeight;
    int cancelX = okX - spacing - buttonWidth;
    int cancelY = okY;
    okButton->rect.x = okX;
    okButton->rect.y = okY;
    cancelButton->rect.x = cancelX;
    cancelButton->rect.y = cancelY;
}

void UIDialog::update(Input& input) {
    if (!visible) return;
    layoutButtons();
    if (cancelButton) cancelButton->update(input);
    if (okButton) okButton->update(input);
}

void UIDialog::render(SDL_Renderer* renderer) {
    if (!visible) return;
    ensureFont();

    // dim overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, overlayColor.r, overlayColor.g, overlayColor.b, overlayColor.a);
    SDL_Rect full{0,0,0,0}; SDL_GetRendererOutputSize(renderer, &full.w, &full.h);
    SDL_RenderFillRect(renderer, &full);

    // dialog body
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRect(renderer, &rect);

    // title & message
    renderText(renderer, title, rect.x + padding, rect.y + padding);
    renderText(renderer, message, rect.x + padding, rect.y + padding + 30);

    // buttons via Button class
    if (cancelButton) cancelButton->render(renderer);
    if (okButton) okButton->render(renderer);
}

void UIDialog::renderText(SDL_Renderer* renderer, const std::string& text, int x, int y, bool centerX) {
    if (!font) return;
    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), textColor);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }
    int tx = x;
    if (centerX) {
        int w; int h; SDL_GetRendererOutputSize(renderer, &w, &h);
        tx = (w - surf->w)/2;
    }
    SDL_Rect dst{ tx, y, surf->w, surf->h };
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}
