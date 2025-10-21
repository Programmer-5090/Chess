#ifndef PROMOTION_DIALOG_H
#define PROMOTION_DIALOG_H

#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <string>
#include <functional>
#include <memory>
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
    
    SDL_Rect dialogRect;
    SDL_Color backgroundColor = {45, 45, 55, 220};
    SDL_Color borderColor = {80, 80, 100, 255};
    std::vector<PieceButtonInfo> promotionButtons;
    std::function<void(PieceType)> onPromotionSelected;
    void createButtons(int boardX, int boardY);
    void loadPieceTextures();
    SDL_Texture* loadPieceTexture(PieceType type, Color color);
    std::string getPieceImagePath(PieceType type, Color color);
    void calculateDialogPosition(int boardX, int boardY);
    void renderDialog(SDL_Renderer* renderer);
    void renderPieceTextures(SDL_Renderer* renderer);
    
    static constexpr int BUTTON_PADDING = 8;
    static constexpr int BUTTON_SPACING = 4;
    static constexpr int BORDER_WIDTH = 2;
};

#endif // PROMOTION_DIALOG_H

