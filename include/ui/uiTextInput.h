#ifndef UI_TEXTINPUT_H
#define UI_TEXTINPUT_H

#include "uiElement.h"
#include <string>
#include <functional>
#include <chrono>
#include <SDL_keyboard.h>

namespace UI {

/**
 * TextInput UI element for user text entry
 */
class TextInput : public UIElement {
public:
    TextInput(const SDL_FRect& rect, const std::string& fontPath, int fontSize)
        : UIElement(rect), fontPath(fontPath), fontSize(fontSize),
          placeholder("Enter text..."), text(""), maxLength(100),
          cursorPosition(0), selectionStart(-1), selectionEnd(-1),
          cursorBlinkTime(0), showCursor(false), hasFocus(false) {}
    
    ~TextInput() override {
        cleanup();
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        
        // Lazy initialization of font
        if (!font) {
            font = FontManager::getInstance().getFont(fontPath, fontSize);
        }
        
        // Update texture if text changed or doesn't exist
        if ((!texture && font) || textChanged) {
            updateTextTexture(renderer);
            textChanged = false;
        }
        
        // Draw input field background
        SDL_Color bgColor;
        if (state == UIState::DISABLED) {
            bgColor = disabledColor;
        } else if (hasFocus) {
            bgColor = activeColor;
        } else if (state == UIState::HOVER) {
            bgColor = hoverColor;
        } else {
            bgColor = normalColor;
        }
        
        SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        SDL_RenderFillRectF(renderer, &rect);
        
        // Draw border
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
        SDL_RenderDrawRectF(renderer, &rect);
        
        // Calculate text area
        SDL_FRect textRect = {
            rect.x + padding, rect.y,
            rect.w - (padding * 2), rect.h
        };
        
        // Render text or placeholder
        if (texture && !text.empty()) {
            renderTextureWithAlignment(renderer, texture, textRect, TextAlignment::LEFT);
        } else if (font && text.empty() && !placeholder.empty()) {
            // Render placeholder if no text
            if (!placeholderTexture) {
                placeholderTexture = renderTextToTexture(renderer, placeholder, font, placeholderColor);
            }
            if (placeholderTexture) {
                renderTextureWithAlignment(renderer, placeholderTexture, textRect, TextAlignment::LEFT);
            }
        }
        
        // Draw cursor if this input has focus
        if (hasFocus && showCursor) {
            // Get cursor position in pixels
            int cursorX = textRect.x;
            if (!text.empty() && cursorPosition > 0) {
                std::string textBeforeCursor = text.substr(0, cursorPosition);
                int textW, textH;
                TTF_SizeUTF8(font, textBeforeCursor.c_str(), &textW, &textH);
                cursorX += textW;
            }
            
            // Draw cursor line
            SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b, textColor.a);
            SDL_RenderDrawLineF(renderer, 
                               cursorX, textRect.y + 5,
                               cursorX, textRect.y + textRect.h - 5);
        }
        
        // Draw selection if any
        if (hasFocus && selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
            int startPos = std::min(selectionStart, selectionEnd);
            int endPos = std::max(selectionStart, selectionEnd);
            
            std::string textBeforeSelection = text.substr(0, startPos);
            std::string selectedText = text.substr(startPos, endPos - startPos);
            
            int startX, endX, textH;
            TTF_SizeUTF8(font, textBeforeSelection.c_str(), &startX, &textH);
            int selectionWidth;
            TTF_SizeUTF8(font, selectedText.c_str(), &selectionWidth, &textH);
            
            startX += textRect.x;
            endX = startX + selectionWidth;
            
            // Draw selection rectangle
            SDL_FRect selectionRect = {
                static_cast<float>(startX), textRect.y + 2,
                static_cast<float>(selectionWidth), textRect.h - 4
            };
            
            SDL_SetRenderDrawColor(renderer, selectionColor.r, selectionColor.g, 
                                  selectionColor.b, selectionColor.a);
            SDL_RenderFillRectF(renderer, &selectionRect);
        }
    }
    
    void update(Input& input) override {
        if (!visible || state == UIState::DISABLED) return;
        
        // Update cursor blink timer
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - lastBlinkTime).count();
        
        if (elapsedTime > 500) { // 500ms blink rate
            showCursor = !showCursor;
            lastBlinkTime = currentTime;
        }
        
        int mouseX = input.getMouseX();
        int mouseY = input.getMouseY();
        bool isHovering = containsPoint(mouseX, mouseY);
        
        // Handle hover state
        if (isHovering) {
            state = UIState::HOVER;
        } else if (!hasFocus) {
            state = UIState::NORMAL;
        }
        
        // Handle mouse click
        if (input.isMouseButtonReleased(SDL_BUTTON_LEFT)) {
            bool wasFocused = hasFocus;
            hasFocus = isHovering;
            
            if (hasFocus) {
                // Reset cursor blink
                showCursor = true;
                lastBlinkTime = currentTime;
                
                // Position cursor based on click position
                if (font && !text.empty()) {
                    int clickX = mouseX - static_cast<int>(rect.x + padding);
                    positionCursorFromClick(clickX);
                } else {
                    cursorPosition = 0;
                }
                
                // Start text input if not already started
                SDL_StartTextInput();
                
                // Clear selection
                selectionStart = selectionEnd = -1;
                
                // Call focus callback
                if (!wasFocused && onFocus) {
                    onFocus();
                }
            } else if (wasFocused) {
                // Call blur callback
                if (onBlur) {
                    onBlur();
                }
            }
        }
    }
    
    void handleKeyEvent(const SDL_Event& event) {
        if (!hasFocus || state == UIState::DISABLED) return;
        
        if (event.type == SDL_TEXTINPUT) {
            // Delete any selected text first
            deleteSelectedText();
            
            // Insert the text at cursor position
            if (text.length() < maxLength) {
                text.insert(cursorPosition, event.text.text);
                cursorPosition += strlen(event.text.text);
                textChanged = true;
                
                if (onTextChanged) {
                    onTextChanged(text);
                }
            }
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_BACKSPACE:
                    if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                        deleteSelectedText();
                    } else if (cursorPosition > 0) {
                        text.erase(cursorPosition - 1, 1);
                        cursorPosition--;
                        textChanged = true;
                        
                        if (onTextChanged) {
                            onTextChanged(text);
                        }
                    }
                    break;
                    
                case SDLK_DELETE:
                    if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
                        deleteSelectedText();
                    } else if (cursorPosition < static_cast<int>(text.length())) {
                        text.erase(cursorPosition, 1);
                        textChanged = true;
                        
                        if (onTextChanged) {
                            onTextChanged(text);
                        }
                    }
                    break;
                    
                case SDLK_LEFT:
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        // Start or extend selection
                        if (selectionStart == -1) {
                            selectionStart = selectionEnd = cursorPosition;
                        }
                        
                        if (cursorPosition > 0) {
                            cursorPosition--;
                            selectionEnd = cursorPosition;
                        }
                    } else {
                        // Clear selection and move cursor
                        selectionStart = selectionEnd = -1;
                        if (cursorPosition > 0) {
                            cursorPosition--;
                        }
                    }
                    break;
                    
                case SDLK_RIGHT:
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        // Start or extend selection
                        if (selectionStart == -1) {
                            selectionStart = selectionEnd = cursorPosition;
                        }
                        
                        if (cursorPosition < static_cast<int>(text.length())) {
                            cursorPosition++;
                            selectionEnd = cursorPosition;
                        }
                    } else {
                        // Clear selection and move cursor
                        selectionStart = selectionEnd = -1;
                        if (cursorPosition < static_cast<int>(text.length())) {
                            cursorPosition++;
                        }
                    }
                    break;
                    
                case SDLK_HOME:
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        if (selectionStart == -1) {
                            selectionStart = selectionEnd = cursorPosition;
                        }
                        cursorPosition = 0;
                        selectionEnd = cursorPosition;
                    } else {
                        selectionStart = selectionEnd = -1;
                        cursorPosition = 0;
                    }
                    break;
                    
                case SDLK_END:
                    if (event.key.keysym.mod & KMOD_SHIFT) {
                        if (selectionStart == -1) {
                            selectionStart = selectionEnd = cursorPosition;
                        }
                        cursorPosition = static_cast<int>(text.length());
                        selectionEnd = cursorPosition;
                    } else {
                        selectionStart = selectionEnd = -1;
                        cursorPosition = static_cast<int>(text.length());
                    }
                    break;
                    
                case SDLK_a:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                        // Select all
                        selectionStart = 0;
                        selectionEnd = cursorPosition = static_cast<int>(text.length());
                    }
                    break;
                    
                case SDLK_c:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                        // Copy selected text
                        copySelectedText();
                    }
                    break;
                    
                case SDLK_v:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                        // Paste text
                        pasteText();
                    }
                    break;
                    
                case SDLK_x:
                    if (event.key.keysym.mod & KMOD_CTRL) {
                        // Cut selected text
                        copySelectedText();
                        deleteSelectedText();
                    }
                    break;
                    
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (onEnterPressed) {
                        onEnterPressed();
                    }
                    break;
                    
                case SDLK_ESCAPE:
                    hasFocus = false;
                    SDL_StopTextInput();
                    if (onBlur) {
                        onBlur();
                    }
                    break;
            }
            
            // Reset cursor blink after any key press
            showCursor = true;
            lastBlinkTime = std::chrono::steady_clock::now();
        }
    }
    
    void cleanup() override {
        freeTexture(texture);
        freeTexture(placeholderTexture);
    }
    
    // Setters
    void setText(const std::string& newText) {
        if (text != newText) {
            text = newText;
            cursorPosition = static_cast<int>(text.length());
            selectionStart = selectionEnd = -1;
            textChanged = true;
            
            if (onTextChanged) {
                onTextChanged(text);
            }
        }
    }
    
    std::string getText() const {
        return text;
    }
    
    void setPlaceholder(const std::string& newPlaceholder) {
        if (placeholder != newPlaceholder) {
            placeholder = newPlaceholder;
            freeTexture(placeholderTexture);
            placeholderTexture = nullptr;
        }
    }
    
    void setMaxLength(int max) {
        maxLength = max;
    }
    
    void setFocus(bool focus) {
        if (hasFocus != focus) {
            hasFocus = focus;
            if (hasFocus) {
                SDL_StartTextInput();
                cursorPosition = static_cast<int>(text.length());
                showCursor = true;
                lastBlinkTime = std::chrono::steady_clock::now();
                
                if (onFocus) {
                    onFocus();
                }
            } else {
                SDL_StopTextInput();
                selectionStart = selectionEnd = -1;
                
                if (onBlur) {
                    onBlur();
                }
            }
        }
    }
    
    bool isFocused() const {
        return hasFocus;
    }
    
    // Callbacks
    void setOnTextChanged(std::function<void(const std::string&)> callback) {
        onTextChanged = callback;
    }
    
    void setOnEnterPressed(std::function<void()> callback) {
        onEnterPressed = callback;
    }
    
    void setOnFocus(std::function<void()> callback) {
        onFocus = callback;
    }
    
    void setOnBlur(std::function<void()> callback) {
        onBlur = callback;
    }
    
    void setColors(const SDL_Color& normal, const SDL_Color& hover, 
                   const SDL_Color& active, const SDL_Color& disabled,
                   const SDL_Color& text, const SDL_Color& placeholder,
                   const SDL_Color& border, const SDL_Color& selection) {
        normalColor = normal;
        hoverColor = hover;
        activeColor = active;
        disabledColor = disabled;
        textColor = text;
        placeholderColor = placeholder;
        borderColor = border;
        selectionColor = selection;
    }

private:
    void updateTextTexture(SDL_Renderer* renderer) {
        freeTexture(texture);
        
        if (font && !text.empty()) {
            texture = renderTextToTexture(renderer, text, font, textColor);
        }
    }
    
    void positionCursorFromClick(int clickX) {
        if (text.empty()) {
            cursorPosition = 0;
            return;
        }
        
        // Find the character closest to the click position
        for (size_t i = 0; i <= text.length(); i++) {
            std::string textBeforeCursor = text.substr(0, i);
            int textW, textH;
            TTF_SizeUTF8(font, textBeforeCursor.c_str(), &textW, &textH);
            
            if (textW >= clickX) {
                cursorPosition = i;
                return;
            }
        }
        
        // If we get here, the click was past the end of the text
        cursorPosition = static_cast<int>(text.length());
    }
    
    void deleteSelectedText() {
        if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
            int start = std::min(selectionStart, selectionEnd);
            int end = std::max(selectionStart, selectionEnd);
            int length = end - start;
            
            text.erase(start, length);
            cursorPosition = start;
            selectionStart = selectionEnd = -1;
            textChanged = true;
            
            if (onTextChanged) {
                onTextChanged(text);
            }
        }
    }
    
    void copySelectedText() {
        if (selectionStart != -1 && selectionEnd != -1 && selectionStart != selectionEnd) {
            int start = std::min(selectionStart, selectionEnd);
            int end = std::max(selectionStart, selectionEnd);
            std::string selectedText = text.substr(start, end - start);
            
            SDL_SetClipboardText(selectedText.c_str());
        }
    }
    
    void pasteText() {
        if (SDL_HasClipboardText()) {
            char* clipboardText = SDL_GetClipboardText();
            if (clipboardText) {
                // Delete any selected text first
                deleteSelectedText();
                
                // Insert clipboard text
                std::string pasteStr = clipboardText;
                // Limit text to maxLength
                if (text.length() + pasteStr.length() > maxLength) {
                    pasteStr = pasteStr.substr(0, maxLength - text.length());
                }
                
                text.insert(cursorPosition, pasteStr);
                cursorPosition += static_cast<int>(pasteStr.length());
                textChanged = true;
                
                if (onTextChanged) {
                    onTextChanged(text);
                }
                
                SDL_free(clipboardText);
            }
        }
    }

    std::string fontPath;
    int fontSize;
    std::string placeholder;
    std::string text;
    int maxLength;
    bool textChanged = false;
    
    int cursorPosition;
    int selectionStart;
    int selectionEnd;
    
    long long cursorBlinkTime;
    bool showCursor;
    bool hasFocus;
    std::chrono::steady_clock::time_point lastBlinkTime;
    
    TTF_Font* font = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_Texture* placeholderTexture = nullptr;
    
    float padding = 5.0f;
    
    // Colors
    SDL_Color normalColor = {60, 60, 60, 255};
    SDL_Color hoverColor = {80, 80, 80, 255};
    SDL_Color activeColor = {40, 40, 40, 255};
    SDL_Color disabledColor = {30, 30, 30, 128};
    SDL_Color textColor = {240, 240, 240, 255};
    SDL_Color placeholderColor = {150, 150, 150, 255};
    SDL_Color borderColor = {100, 100, 100, 255};
    SDL_Color selectionColor = {100, 100, 170, 128};
    
    // Callbacks
    std::function<void(const std::string&)> onTextChanged;
    std::function<void()> onEnterPressed;
    std::function<void()> onFocus;
    std::function<void()> onBlur;
};

} // namespace UI

#endif // UI_TEXTINPUT_H
