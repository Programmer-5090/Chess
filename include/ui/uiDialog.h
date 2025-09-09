#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include "uiElement.h"
#include "uiPanel.h"
#include "uiButton.h"
#include "uiLabel.h"
#include <vector>
#include <memory>
#include <functional>

namespace UI {

/**
 * Dialog UI element that serves as a modal popup
 */
class Dialog : public UIElement {
public:
    enum class DialogResult {
        NONE,
        OK,
        CANCEL,
        YES,
        NO,
        CUSTOM
    };

    Dialog(const SDL_FRect& rect, const std::string& title, const std::string& message,
           const std::string& fontPath, int titleFontSize = 20, int messageFontSize = 16)
        : UIElement(rect), title(title), message(message), 
          fontPath(fontPath), titleFontSize(titleFontSize), messageFontSize(messageFontSize),
          dialogResult(DialogResult::NONE), isDragging(false), dragOffsetX(0), dragOffsetY(0) {
        
        // Create internal panel for dialog background
        panel = std::make_shared<Panel>(rect);
        panel->setBackgroundColor({40, 40, 40, 240});
        panel->setDrawBorder(true);
        panel->setBorderColor({100, 100, 100, 255});
    }
    
    ~Dialog() override {
        cleanup();
    }
    
    void render(SDL_Renderer* renderer) override {
        if (!visible) return;
        
        // Render semi-transparent overlay
        if (modal) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
            SDL_FRect fullScreen = {0, 0, 800, 600}; // Adjust to match screen size
            SDL_RenderFillRectF(renderer, &fullScreen);
        }
        
        // Render panel background
        panel->setRect(rect);
        panel->render(renderer);
        
        // Render title bar
        float titleBarHeight = 30.0f;
        SDL_FRect titleBarRect = {
            rect.x, rect.y, rect.w, titleBarHeight
        };
        
        // Draw title bar background
        SDL_SetRenderDrawColor(renderer, titleBarColor.r, titleBarColor.g, 
                              titleBarColor.b, titleBarColor.a);
        SDL_RenderFillRectF(renderer, &titleBarRect);
        
        // Draw title text
        if (!titleFont) {
            titleFont = FontManager::getInstance().getFont(fontPath, titleFontSize);
        }
        
        if (!titleTexture && titleFont) {
            titleTexture = renderTextToTexture(renderer, title, titleFont, textColor);
        }
        
        if (titleTexture) {
            SDL_FRect textRect = {
                rect.x + 10, rect.y, rect.w - 20, titleBarHeight
            };
            renderTextureWithAlignment(renderer, titleTexture, textRect, TextAlignment::LEFT);
        }
        
        // Draw close button
        float closeButtonSize = titleBarHeight - 6;
        SDL_FRect closeButtonRect = {
            rect.x + rect.w - closeButtonSize - 5, rect.y + 3,
            closeButtonSize, closeButtonSize
        };
        
        SDL_SetRenderDrawColor(renderer, closeButtonColor.r, closeButtonColor.g, 
                              closeButtonColor.b, closeButtonColor.a);
        SDL_RenderFillRectF(renderer, &closeButtonRect);
        
        // Draw X symbol on close button
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        float padding = closeButtonSize * 0.3f;
        SDL_RenderDrawLineF(renderer,
                           closeButtonRect.x + padding, closeButtonRect.y + padding,
                           closeButtonRect.x + closeButtonSize - padding, 
                           closeButtonRect.y + closeButtonSize - padding);
        SDL_RenderDrawLineF(renderer,
                           closeButtonRect.x + padding, closeButtonRect.y + closeButtonSize - padding,
                           closeButtonRect.x + closeButtonSize - padding, closeButtonRect.y + padding);
        
        // Render message text
        if (!messageFont) {
            messageFont = FontManager::getInstance().getFont(fontPath, messageFontSize);
        }
        
        if (!messageTexture && messageFont) {
            messageTexture = renderTextToTexture(renderer, message, messageFont, textColor);
        }
        
        if (messageTexture) {
            SDL_FRect contentRect = {
                rect.x + 20, rect.y + titleBarHeight + 20,
                rect.w - 40, rect.h - titleBarHeight - 80 // Leave space for buttons
            };
            renderTextureWithAlignment(renderer, messageTexture, contentRect, TextAlignment::LEFT, TextAlignment::TOP);
        }
        
        // Render buttons
        float buttonY = rect.y + rect.h - 50;
        float buttonWidth = 100;
        float buttonHeight = 30;
        float buttonSpacing = 10;
        float totalButtonWidth = buttons.size() * buttonWidth + (buttons.size() - 1) * buttonSpacing;
        float startX = rect.x + (rect.w - totalButtonWidth) / 2;
        
        for (size_t i = 0; i < buttons.size(); i++) {
            if (!buttonFonts[i]) {
                buttonFonts[i] = FontManager::getInstance().getFont(fontPath, messageFontSize);
            }
            
            if (!buttonTextures[i] && buttonFonts[i]) {
                buttonTextures[i] = renderTextToTexture(renderer, buttons[i], buttonFonts[i], textColor);
            }
            
            float buttonX = startX + i * (buttonWidth + buttonSpacing);
            SDL_FRect buttonRect = {buttonX, buttonY, buttonWidth, buttonHeight};
            
            // Draw button background based on hover state
            bool isHovered = buttonHoverStates[i];
            SDL_Color bgColor = isHovered ? buttonHoverColor : buttonColor;
            
            SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
            SDL_RenderFillRectF(renderer, &buttonRect);
            
            // Draw button border
            SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
            SDL_RenderDrawRectF(renderer, &buttonRect);
            
            // Draw button text
            if (buttonTextures[i]) {
                renderTextureWithAlignment(renderer, buttonTextures[i], buttonRect);
            }
        }
    }
    
    void update(Input& input) override {
        if (!visible) return;
        
        int mouseX = input.getMouseX();
        int mouseY = input.getMouseY();
        
        // Handle title bar dragging
        float titleBarHeight = 30.0f;
        SDL_FRect titleBarRect = {
            rect.x, rect.y, rect.w - titleBarHeight, titleBarHeight // Exclude close button area
        };
        
        bool onTitleBar = mouseX >= titleBarRect.x && mouseX <= titleBarRect.x + titleBarRect.w &&
                         mouseY >= titleBarRect.y && mouseY <= titleBarRect.y + titleBarRect.h;
        
        if (onTitleBar) {
            if (input.isMouseButtonPressed(SDL_BUTTON_LEFT)) {
                isDragging = true;
                dragOffsetX = mouseX - rect.x;
                dragOffsetY = mouseY - rect.y;
            }
        }
        
        if (isDragging) {
            if (input.isMouseButtonDown(SDL_BUTTON_LEFT)) {
                rect.x = mouseX - dragOffsetX;
                rect.y = mouseY - dragOffsetY;
                
                // Keep dialog within screen bounds (assuming 800x600 screen)
                if (rect.x < 0) rect.x = 0;
                if (rect.y < 0) rect.y = 0;
                if (rect.x + rect.w > 800) rect.x = 800 - rect.w;
                if (rect.y + rect.h > 600) rect.y = 600 - rect.h;
            } else {
                isDragging = false;
            }
        }
        
        // Handle close button
        float closeButtonSize = titleBarHeight - 6;
        SDL_FRect closeButtonRect = {
            rect.x + rect.w - closeButtonSize - 5, rect.y + 3,
            closeButtonSize, closeButtonSize
        };
        
        bool onCloseButton = mouseX >= closeButtonRect.x && mouseX <= closeButtonRect.x + closeButtonRect.w &&
                            mouseY >= closeButtonRect.y && mouseY <= closeButtonRect.y + closeButtonRect.h;
        
        if (onCloseButton && input.isMouseButtonReleased(SDL_BUTTON_LEFT)) {
            dialogResult = DialogResult::CANCEL;
            visible = false;
            
            if (onResult) {
                onResult(dialogResult);
            }
        }
        
        // Handle button interactions
        float buttonY = rect.y + rect.h - 50;
        float buttonWidth = 100;
        float buttonHeight = 30;
        float buttonSpacing = 10;
        float totalButtonWidth = buttons.size() * buttonWidth + (buttons.size() - 1) * buttonSpacing;
        float startX = rect.x + (rect.w - totalButtonWidth) / 2;
        
        for (size_t i = 0; i < buttons.size(); i++) {
            float buttonX = startX + i * (buttonWidth + buttonSpacing);
            SDL_FRect buttonRect = {buttonX, buttonY, buttonWidth, buttonHeight};
            
            bool onButton = mouseX >= buttonRect.x && mouseX <= buttonRect.x + buttonRect.w &&
                           mouseY >= buttonRect.y && mouseY <= buttonRect.y + buttonRect.h;
            
            buttonHoverStates[i] = onButton;
            
            if (onButton && input.isMouseButtonReleased(SDL_BUTTON_LEFT)) {
                DialogResult result;
                
                switch (i) {
                    case 0:
                        if (buttons.size() <= 2) {
                            result = DialogResult::OK;
                        } else {
                            result = DialogResult::YES;
                        }
                        break;
                    case 1:
                        if (buttons.size() <= 2) {
                            result = DialogResult::CANCEL;
                        } else {
                            result = DialogResult::NO;
                        }
                        break;
                    default:
                        result = DialogResult::CUSTOM;
                        break;
                }
                
                dialogResult = result;
                
                if (buttonCallbacks[i]) {
                    buttonCallbacks[i]();
                }
                
                if (onResult) {
                    onResult(dialogResult);
                }
                
                // Auto-close on button click if set
                if (autoCloseOnButton) {
                    visible = false;
                }
            }
        }
    }
    
    void cleanup() override {
        freeTexture(titleTexture);
        freeTexture(messageTexture);
        
        for (auto& texture : buttonTextures) {
            freeTexture(texture);
        }
        
        buttonTextures.clear();
        buttonFonts.clear();
        
        if (panel) {
            panel->cleanup();
        }
    }
    
    // Set button configuration
    void setButtons(const std::vector<std::string>& buttonLabels) {
        buttons = buttonLabels;
        buttonHoverStates.resize(buttons.size(), false);
        buttonCallbacks.resize(buttons.size(), nullptr);
        buttonTextures.resize(buttons.size(), nullptr);
        buttonFonts.resize(buttons.size(), nullptr);
    }
    
    // Set button callback
    void setButtonCallback(size_t index, std::function<void()> callback) {
        if (index < buttonCallbacks.size()) {
            buttonCallbacks[index] = callback;
        }
    }
    
    // Set result callback
    void setOnResult(std::function<void(DialogResult)> callback) {
        onResult = callback;
    }
    
    // Set auto-close behavior
    void setAutoCloseOnButton(bool autoClose) {
        autoCloseOnButton = autoClose;
    }
    
    // Set modal behavior (darkens screen behind dialog)
    void setModal(bool isModal) {
        modal = isModal;
    }
    
    // Show the dialog
    void show() {
        visible = true;
        dialogResult = DialogResult::NONE;
    }
    
    // Get the dialog result
    DialogResult getResult() const {
        return dialogResult;
    }
    
    // Set colors
    void setColors(const SDL_Color& background, const SDL_Color& border,
                  const SDL_Color& titleBar, const SDL_Color& closeButton,
                  const SDL_Color& button, const SDL_Color& buttonHover,
                  const SDL_Color& text) {
        panel->setBackgroundColor(background);
        panel->setBorderColor(border);
        titleBarColor = titleBar;
        closeButtonColor = closeButton;
        buttonColor = button;
        buttonHoverColor = buttonHover;
        textColor = text;
        borderColor = border;
    }

private:
    std::string title;
    std::string message;
    std::string fontPath;
    int titleFontSize;
    int messageFontSize;
    
    TTF_Font* titleFont = nullptr;
    TTF_Font* messageFont = nullptr;
    SDL_Texture* titleTexture = nullptr;
    SDL_Texture* messageTexture = nullptr;
    
    std::shared_ptr<Panel> panel;
    
    std::vector<std::string> buttons;
    std::vector<bool> buttonHoverStates;
    std::vector<std::function<void()>> buttonCallbacks;
    std::vector<TTF_Font*> buttonFonts;
    std::vector<SDL_Texture*> buttonTextures;
    
    DialogResult dialogResult;
    std::function<void(DialogResult)> onResult;
    
    bool autoCloseOnButton = true;
    bool modal = true;
    
    bool isDragging;
    float dragOffsetX;
    float dragOffsetY;
    
    // Colors
    SDL_Color titleBarColor = {60, 60, 100, 255};
    SDL_Color closeButtonColor = {180, 60, 60, 255};
    SDL_Color buttonColor = {70, 70, 70, 255};
    SDL_Color buttonHoverColor = {90, 90, 90, 255};
    SDL_Color textColor = {240, 240, 240, 255};
    SDL_Color borderColor = {100, 100, 100, 255};
};

// Helper function to create message dialogs
inline std::shared_ptr<Dialog> createMessageDialog(
    const std::string& title, const std::string& message,
    const std::string& fontPath, int width = 400, int height = 200) {
    
    // Position dialog in center of screen (assuming 800x600 screen)
    SDL_FRect rect = {
        (800 - width) / 2.0f, (600 - height) / 2.0f,
        static_cast<float>(width), static_cast<float>(height)
    };
    
    auto dialog = std::make_shared<Dialog>(rect, title, message, fontPath);
    dialog->setButtons({"OK"});
    dialog->setModal(true);
    return dialog;
}

// Helper function to create confirmation dialogs
inline std::shared_ptr<Dialog> createConfirmDialog(
    const std::string& title, const std::string& message,
    const std::string& fontPath, int width = 400, int height = 200) {
    
    // Position dialog in center of screen (assuming 800x600 screen)
    SDL_FRect rect = {
        (800 - width) / 2.0f, (600 - height) / 2.0f,
        static_cast<float>(width), static_cast<float>(height)
    };
    
    auto dialog = std::make_shared<Dialog>(rect, title, message, fontPath);
    dialog->setButtons({"OK", "Cancel"});
    dialog->setModal(true);
    return dialog;
}

// Helper function to create yes/no dialogs
inline std::shared_ptr<Dialog> createYesNoDialog(
    const std::string& title, const std::string& message,
    const std::string& fontPath, int width = 400, int height = 200) {
    
    // Position dialog in center of screen (assuming 800x600 screen)
    SDL_FRect rect = {
        (800 - width) / 2.0f, (600 - height) / 2.0f,
        static_cast<float>(width), static_cast<float>(height)
    };
    
    auto dialog = std::make_shared<Dialog>(rect, title, message, fontPath);
    dialog->setButtons({"Yes", "No"});
    dialog->setModal(true);
    return dialog;
}

} // namespace UI

#endif // UI_DIALOG_H
