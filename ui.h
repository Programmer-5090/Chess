#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "input.h"

using namespace std;

// Helper function to normalize SDL_Color
inline SDL_Color normalizeColor(const SDL_Color& color, int alpha = 255) {
    SDL_Color result = color;
    result.a = alpha;
    return result;
}

// Helper function to convert RGB(A) tuples to SDL_Color
inline SDL_Color tupleToColor(int r, int g, int b, int a = 255) {
    SDL_Color color = {static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b), static_cast<Uint8>(a)};
    return color;
}

// Base UI Element class
class UIElement {
public:
    UIElement(int x, int y, int width, int height) {
        rect = {x, y, width, height};
        visible = true;
    }
    
    virtual ~UIElement() {}
    
    virtual void update(Input& input) {}
    
    virtual void render(SDL_Renderer* renderer) {}
    
    SDL_Rect rect;
    bool visible;
};

// Button class with 3D effect
class Button : public UIElement {
public:
    Button(int x, int y, int width, int height, const string& text, function<void()> callback,
           SDL_Color color = {255, 255, 255, 255},
           SDL_Color hoverColor = {130, 130, 130, 255},
           string fontPath = "", 
           SDL_Color textColor = {0, 0, 0, 255},
           int elevation = 6, 
           int fontSize = 24)
        : UIElement(x, y, width, height),
          text(text),
          callback(callback),
          color(color),
          hoverColor(hoverColor),
          textColor(textColor),
          elevation(elevation),
          dynamicElevation(elevation), // Default to full elevation
          originalYPos(y),
          fontSize(fontSize)
    {
        topRect = {x, y - elevation, width, height};
        bottomRect = {x, y, width, height};
        
        loadFont(fontPath);
        
        // Initialize visual state based on mouse at creation
        int mouseX_init, mouseY_init;
        uint32_t mouseState_init = SDL_GetMouseState(&mouseX_init, &mouseY_init);

        // Check for hover at creation time (button is in its "up" state)
        bool hover_init = (mouseX_init >= rect.x && mouseX_init <= rect.x + rect.w &&
                           mouseY_init >= originalYPos - elevation && 
                           mouseY_init <= originalYPos - elevation + rect.h);

        if (hover_init) {
            this->currentColor = hoverColor; // Set hover color if mouse is over it
            if (mouseState_init & SDL_BUTTON(SDL_BUTTON_LEFT)) { // Check if also pressed
                isPressed = true;
                dynamicElevation = 0; // Visually pressed
            } else {
                isPressed = false;
                // dynamicElevation remains `elevation` (button is up but hovered)
            }
        } else {
            this->currentColor = color; // Not hovering, use base color
            isPressed = false;
            // dynamicElevation remains `elevation`
        }
        
        // Initialize bottomColor based on the determined currentColor
        bottomColor = {
            static_cast<Uint8>(max(0, this->currentColor.r - 40)),
            static_cast<Uint8>(max(0, this->currentColor.g - 40)),
            static_cast<Uint8>(max(0, this->currentColor.b - 40)),
            this->currentColor.a
        };
        
        clickCooldownTimestamp = SDL_GetTicks64(); // Ready for first click immediately
    }
    
    ~Button() {
        if (font != nullptr) {
            TTF_CloseFont(font);
        }
    }
    
    void update(Input& input) override {
        if (!visible) return;

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        
        bool hover = (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
                      mouseY >= rect.y - dynamicElevation && 
                      mouseY <= rect.y - dynamicElevation + rect.h);
        
        bool mouseIsCurrentlyDown = input.getMouseStates()["left"];

        // Set currentColor based on hover and pressed state
        if (isPressed || hover) { 
            currentColor = hoverColor;
        } else {
            currentColor = color;
        }
        
        bottomColor = {
            static_cast<Uint8>(max(0, currentColor.r - 40)),
            static_cast<Uint8>(max(0, currentColor.g - 40)),
            static_cast<Uint8>(max(0, currentColor.b - 40)),
            currentColor.a
        };
        
        if (hover) {
            if (mouseIsCurrentlyDown) {
                if (!isPressed) { // Mouse just went down over the button (transition to pressed)
                    Uint64 currentTime = SDL_GetTicks64();
                    if (currentTime >= clickCooldownTimestamp) { // Use >= for cooldown check
                        isPressed = true;
                        dynamicElevation = 0; // Button goes down visually

                        clickCooldownTimestamp = currentTime + 200; // Set cooldown for the next potential click
                        
                        // We'll execute the callback at the end of the update method
                        // This ensures all button states are properly updated first
                    }
                }
                // If already isPressed, mouse is still held down over button. No new callback.
            } else { // Mouse is up while hovering
                if (isPressed) { // Was pressed, now released over button (transition to released)
                    isPressed = false;
                    dynamicElevation = elevation; // Button comes up visually
                }
            }
        } else { // Not hovering
            if (isPressed) { // Was pressed, but mouse released off-button or moved off while up
                isPressed = false;
                dynamicElevation = elevation; // Button comes up visually
            }
        }
        
        // Update button's visual position
        topRect.y = originalYPos - dynamicElevation;
        
        // Execute any pending callback after all state updates are complete
        // Only execute the callback once when the button is initially pressed AND released
        if (isPressed && mouseIsCurrentlyDown && hover) {
            // Button just pressed - mark the start of a click but don't execute yet
            clickStarted = true;
            callbackExecuted = false;
        } 
        else if (clickStarted && hover && !mouseIsCurrentlyDown && !callbackExecuted) {
            // Button released while hovering AND we haven't executed the callback yet
            callbackExecuted = true;
            clickStarted = false;
            
            if (this->callback) {
                this->callback();
            }
        }
        else if (!hover || !mouseIsCurrentlyDown) {
            // Reset the click state if mouse moves away or is released
            clickStarted = false;
        }
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        
        if (dynamicElevation > 0) {
            SDL_SetRenderDrawColor(renderer, bottomColor.r, bottomColor.g, bottomColor.b, bottomColor.a);
            SDL_RenderFillRect(renderer, &bottomRect);
        }
        
        SDL_Rect adjustedTopRect = {topRect.x, topRect.y, topRect.w, topRect.h};
        SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);
        SDL_RenderFillRect(renderer, &adjustedTopRect);
        
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderDrawRect(renderer, &adjustedTopRect);
        
        if (font != nullptr) {
            renderText(renderer, adjustedTopRect);
        }
    }

private:
    string text;
    function<void()> callback;
    SDL_Color color;
    SDL_Color hoverColor;
    SDL_Color currentColor;
    SDL_Color bottomColor;
    SDL_Color textColor;
    TTF_Font* font = nullptr;
    int elevation;
    int dynamicElevation;
    int originalYPos;
    int fontSize;
    bool isPressed;
    SDL_Rect topRect;
    SDL_Rect bottomRect;
    Uint64 clickCooldownTimestamp; // Renamed from clickCooldown
    bool callbackExecuted = false; // Track callback execution per button
    bool clickStarted = false; // Track if a click has started
    
    void loadFont(const string& fontPath) {
        if (!TTF_WasInit() && TTF_Init() == -1) {
            cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << endl;
            return;
        }
        
        if (!fontPath.empty()) {
            font = TTF_OpenFont(fontPath.c_str(), fontSize);
            if (font == nullptr) {
                cout << "Failed to load font: " << fontPath << " SDL_ttf Error: " << TTF_GetError() << endl;
                font = TTF_OpenFont("../fonts/Silkscreen-Regular.ttf", fontSize);
            }
        } else {
            font = TTF_OpenFont("../fonts/Silkscreen-Regular.ttf", fontSize);
        }
        
        if (font == nullptr) {
            cout << "Failed to load any font! SDL_ttf Error: " << TTF_GetError() << endl;
        }
    }
    
    void renderText(SDL_Renderer* renderer, const SDL_Rect& buttonRect) {
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), textColor);
        if (textSurface == nullptr) {
            cout << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << endl;
            return;
        }
        
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture == nullptr) {
            cout << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << endl;
            SDL_FreeSurface(textSurface);
            return;
        }
        
        int textWidth = textSurface->w;
        int textHeight = textSurface->h;
        
        SDL_Rect renderQuad = {
            buttonRect.x + (buttonRect.w - textWidth) / 2, 
            buttonRect.y + (buttonRect.h - textHeight) / 2,
            textWidth, 
            textHeight
        };
        
        SDL_RenderCopy(renderer, textTexture, NULL, &renderQuad);
        
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }
};

// Label class for displaying text
class Label : public UIElement {
public:
    Label(int x, int y, const string& text, SDL_Color color = {255, 255, 255, 255},
          int fontSize = 20, string fontPath = "")
        : UIElement(x, y, 100, 20),
          text(text),
          color(color),
          fontSize(fontSize) {
        
        loadFont(fontPath);
        updateTextDimensions();
    }
    
    ~Label() {
        if (font != nullptr) {
            TTF_CloseFont(font);
        }
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible || font == nullptr) return;
        
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (textSurface == nullptr) {
            cout << "Unable to render text surface! SDL_ttf Error: " << TTF_GetError() << endl;
            return;
        }
        
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (textTexture == nullptr) {
            cout << "Unable to create texture from rendered text! SDL Error: " << SDL_GetError() << endl;
            SDL_FreeSurface(textSurface);
            return;
        }
        
        SDL_RenderCopy(renderer, textTexture, NULL, &rect);
        
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
    }
    
    void setText(const string& newText) {
        text = newText;
        updateTextDimensions();
    }

private:
    string text;
    SDL_Color color;
    TTF_Font* font = nullptr;
    int fontSize;
    
    void loadFont(const string& fontPath) {
        if (!TTF_WasInit() && TTF_Init() == -1) {
            cout << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << endl;
            return;
        }
        
        if (!fontPath.empty()) {
            font = TTF_OpenFont(fontPath.c_str(), fontSize);
            if (font == nullptr) {
                cout << "Failed to load font: " << fontPath << " SDL_ttf Error: " << TTF_GetError() << endl;
                font = TTF_OpenFont("../fonts/Silkscreen-Regular.ttf", fontSize);
            }
        } else {
            font = TTF_OpenFont("../fonts/Silkscreen-Regular.ttf", fontSize);
        }
        
        if (font == nullptr) {
            cout << "Failed to load any font! SDL_ttf Error: " << TTF_GetError() << endl;
        }
    }
    
    void updateTextDimensions() {
        if (font == nullptr) return;
        
        int width, height;
        if (TTF_SizeText(font, text.c_str(), &width, &height) == 0) {
            rect.w = width;
            rect.h = height;
        }
    }
};

// UI Manager to handle all UI elements
class UIManager {
public:
    UIManager(SDL_Renderer* renderer, int screenWidth, int screenHeight)
        : renderer(renderer), screenWidth(screenWidth), screenHeight(screenHeight) {
        uiSurface = SDL_CreateRGBSurface(0, screenWidth, screenHeight, 32, 
                                         0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        if (uiSurface == nullptr) {
            cout << "UI Surface could not be created! SDL Error: " << SDL_GetError() << endl;
        }
        
        uiTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, 
                                      SDL_TEXTUREACCESS_TARGET, screenWidth, screenHeight);
        if (uiTexture == nullptr) {
            cout << "UI Texture could not be created! SDL Error: " << SDL_GetError() << endl;
        }
        
        SDL_SetTextureBlendMode(uiTexture, SDL_BLENDMODE_BLEND);
    }
    
    ~UIManager() {
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
    
    template<typename T, typename... Args>
    T* addElement(Args&&... args) {
        T* element = new T(std::forward<Args>(args)...);
        elements.push_back(element);
        return element;
    }
    
    void update(Input& input) {
        for (auto element : elements) {
            if (element->visible) {
                element->update(input);
            }
        }
    }
    
    void render() {
        SDL_SetRenderTarget(renderer, uiTexture);
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        
        for (auto element : elements) {
            element->render(renderer);
        }
        
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderDrawCircle(renderer, mouseX, mouseY, 10);
        
        SDL_SetRenderTarget(renderer, nullptr);
        
        SDL_RenderCopy(renderer, uiTexture, nullptr, nullptr);
    }
    
    void clearElements() {
        // Create a temporary copy of elements to avoid iterator invalidation
        auto elementsCopy = elements;
        elements.clear(); // Clear the main list first
        
        // Then delete the elements from our copy
        for (auto element : elementsCopy) {
            delete element;
        }
    }

private:
    SDL_Renderer* renderer;
    int screenWidth;
    int screenHeight;
    SDL_Surface* uiSurface = nullptr;
    SDL_Texture* uiTexture = nullptr;
    vector<UIElement*> elements;
    
    void SDL_RenderDrawCircle(SDL_Renderer* renderer, int x, int y, int radius) {
        const int diameter = radius * 2;
        
        int x_pos = radius - 1;
        int y_pos = 0;
        int tx = 1;
        int ty = 1;
        int error = tx - diameter;
        
        while (x_pos >= y_pos) {
            SDL_RenderDrawPoint(renderer, x + x_pos, y - y_pos);
            SDL_RenderDrawPoint(renderer, x + x_pos, y + y_pos);
            SDL_RenderDrawPoint(renderer, x - x_pos, y - y_pos);
            SDL_RenderDrawPoint(renderer, x - x_pos, y + y_pos);
            SDL_RenderDrawPoint(renderer, x + y_pos, y - x_pos);
            SDL_RenderDrawPoint(renderer, x + y_pos, y + x_pos);
            SDL_RenderDrawPoint(renderer, x - y_pos, y - x_pos);
            SDL_RenderDrawPoint(renderer, x - y_pos, y + x_pos);
            
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
};