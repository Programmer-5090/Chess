#pragma once

// SDL headers (support both flat and SDL2/ prefixed installations)
#if __has_include(<SDL.h>)
#  include <SDL.h>
#else
#  include <SDL2/SDL.h>
#endif
#if __has_include(<SDL_image.h>)
#  include <SDL_image.h>
#else
#  include <SDL2/SDL_image.h>
#endif
#include <vector>
#include <string>
#include <functional>
#include <memory>
// Adjusted include paths to match actual file locations
#include "chess/ui/controls/ui/uiElement.h"
#include "chess/ui/controls/button.h"
#include <chess/enums.h>

class Input;
class Button;

class UIPromotionDialog : public UIElement {
public:
    UIPromotionDialog(int boardX, int boardY, float squareSize, int screenWidth, 
                     Color pawnColor, SDL_Renderer* renderer);
    ~UIPromotionDialog();

    void update(Input& input) override;
    void render(SDL_Renderer* renderer) override;
    bool isModal() const override { return visible; }
    
    void show() { visible = true; }
    void hide() { visible = false; }
    
    // Set callback for when a piece type is selected
    void setOnPromotionSelected(std::function<void(PieceType)> callback) {
        onPromotionSelected = std::move(callback);
    }

public:
    bool visible = false;

private:
    struct PieceButtonInfo {
        std::unique_ptr<Button> button;
        PieceType pieceType;
        SDL_Texture* pieceTexture = nullptr;
    };

    Color pawnColor;
    SDL_Renderer* renderer;
    float squareSize;
    int screenWidth;
    
    // Dialog properties
    SDL_Rect dialogRect;
    SDL_Color backgroundColor = {45, 45, 55, 220}; // Slightly transparent
    SDL_Color borderColor = {80, 80, 100, 255};
    
    // Buttons for each promotion option using UIButton class
    std::vector<PieceButtonInfo> promotionButtons;
    
    // Callback when a piece is selected
    std::function<void(PieceType)> onPromotionSelected;
    
    // Helper methods
    void createButtons(int boardX, int boardY);
    void loadPieceTextures();
    SDL_Texture* loadPieceTexture(PieceType type, Color color);
    std::string getPieceImagePath(PieceType type, Color color);
    void calculateDialogPosition(int boardX, int boardY);
    void renderDialog(SDL_Renderer* renderer);
    void renderPieceTextures(SDL_Renderer* renderer);
    
    // Button layout constants
    static constexpr int BUTTON_PADDING = 8;
    static constexpr int BUTTON_SPACING = 4;
    static constexpr int BORDER_WIDTH = 2;
};
