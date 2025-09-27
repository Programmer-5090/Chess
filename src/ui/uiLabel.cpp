#include "ui/uiLabel.h"
#include "logger.h"
#include <iostream>

Label::Label(int x, int y, const std::string& text, SDL_Color color, int fontSize, std::string fontPath)
    : UIElement(x, y, 100, 20), text(text), color(color), fontSize(fontSize) {
    loadFont(fontPath);
    updateTextDimensions();
}

Label::~Label() {
    if (font != nullptr) {
        TTF_CloseFont(font);
    }
}

static bool isWhitespaceOnly(const std::string& s) {
    for (unsigned char c : s) {
        if (!std::isspace(c)) return false;
    }
    return !s.empty();
}

void Label::render(SDL_Renderer* renderer) {
    if (!visible || font == nullptr || text.empty() || isWhitespaceOnly(text)) return;
    
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (textSurface == nullptr) {
        LOG_ERROR(std::string("Unable to render text surface! SDL_ttf Error: ") + TTF_GetError());
        LOG_ERROR(std::string("Text was: '") + text + "' (length: " + std::to_string(text.length()) + ")");
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (textTexture == nullptr) {
        std::cout << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        return;
    }
    // Always render at the text's natural size to avoid stretching when layouts change rect.w/h
    SDL_Rect dst{ rect.x, rect.y, textSurface->w, textSurface->h };
    SDL_RenderCopy(renderer, textTexture, NULL, &dst);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

void Label::setText(const std::string& newText) {
    text = newText; // Allow empty; render() will skip whitespace-only safely
    updateTextDimensions();
}

void Label::loadFont(const std::string& fontPath) {
    if (!TTF_WasInit() && TTF_Init() == -1) {
        LOG_ERROR(std::string("SDL_ttf could not initialize! SDL_ttf Error: ") + TTF_GetError());
        return;
    }
    if (!fontPath.empty()) {
        font = TTF_OpenFont(fontPath.c_str(), fontSize);
        if (font == nullptr) {
            LOG_ERROR(std::string("Failed to load font: ") + fontPath + " SDL_ttf Error: " + TTF_GetError());
        }
    }
    if (font == nullptr) {
        font = TTF_OpenFont("assets/fonts/OpenSans-Regular.ttf", fontSize);
    }
    if (font == nullptr) {
        LOG_ERROR(std::string("Failed to load any font! SDL_ttf Error: ") + TTF_GetError());
    }
}

void Label::updateTextDimensions() {
    if (font == nullptr) return;
    int width = 0, height = 0;
    // For whitespace-only or empty text, keep current rect (let caller set size like spacers)
    if (!text.empty() && !isWhitespaceOnly(text)) {
        if (TTF_SizeText(font, text.c_str(), &width, &height) == 0) {
            rect.w = width;
            rect.h = height;
        }
    }
}
