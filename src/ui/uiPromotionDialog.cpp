#include "ui/uiPromotionDialog.h"
#include "ui/uiButton.h"
#include "input.h"
#include <iostream>

UIPromotionDialog::UIPromotionDialog(int boardX, int boardY, float squareSize, 
                                   int screenWidth, Color pawnColor, SDL_Renderer* renderer)
    : UIElement(0, 0, 0, 0), pawnColor(pawnColor), renderer(renderer), 
      squareSize(squareSize), screenWidth(screenWidth) {
    
    createButtons(boardX, boardY);
    loadPieceTextures();
}

UIPromotionDialog::~UIPromotionDialog() {
    // Clean up textures
    for (auto& buttonInfo : promotionButtons) {
        if (buttonInfo.pieceTexture) {
            SDL_DestroyTexture(buttonInfo.pieceTexture);
        }
    }
}

void UIPromotionDialog::createButtons(int boardX, int boardY) {
    // Create buttons for Queen, Rook, Bishop, Knight (in order of importance)
    std::vector<PieceType> pieceTypes = {QUEEN, ROOK, BISHOP, KNIGHT};
    
    int buttonSize = static_cast<int>(squareSize * 0.8f); // Slightly smaller than square
    int totalWidth = (buttonSize * 4) + (BUTTON_SPACING * 3) + (BUTTON_PADDING * 2) + (BORDER_WIDTH * 2);
    int totalHeight = buttonSize + (BUTTON_PADDING * 2) + (BORDER_WIDTH * 2);
    
    calculateDialogPosition(boardX, boardY);
    
    // Update dialog dimensions
    dialogRect.w = totalWidth;
    dialogRect.h = totalHeight;
    rect = dialogRect; // Update UIElement's rect
    
    // Create promotion buttons using UIButton class
    promotionButtons.clear();
    promotionButtons.reserve(4);
    
    int startX = dialogRect.x + BORDER_WIDTH + BUTTON_PADDING;
    int startY = dialogRect.y + BORDER_WIDTH + BUTTON_PADDING;
    
    for (size_t i = 0; i < pieceTypes.size(); ++i) {
        PieceButtonInfo buttonInfo;
        buttonInfo.pieceType = pieceTypes[i];
        
        int buttonX = startX + i * (buttonSize + BUTTON_SPACING);
        int buttonY = startY;
        
        // Create UIButton with appropriate styling
        SDL_Color buttonColor = {60, 60, 70, 220};        // Slightly lighter than dialog background
        SDL_Color hoverColor = {100, 150, 200, 220};      // Light blue highlight
        
        buttonInfo.button = std::make_unique<Button>(
            buttonX, buttonY, buttonSize, buttonSize,
            "", // No text - we'll render the piece image on top
            [this, pieceType = pieceTypes[i]]() {
                if (onPromotionSelected) {
                    onPromotionSelected(pieceType);
                }
                this->hide();
            },
            buttonColor,
            hoverColor,
            "", // No font path needed
            SDL_Color{255, 255, 255, 255}, // Text color (not used)
            2, // Small elevation for subtle 3D effect
            20 // Font size (not used)
        );
        
        // Set button to bypass callback gate to ensure it works even if callbacks are disabled
        buttonInfo.button->setBypassCallbackGate(true);
        
        buttonInfo.pieceTexture = nullptr; // Will be loaded later
        
        promotionButtons.push_back(std::move(buttonInfo));
    }
}

void UIPromotionDialog::calculateDialogPosition(int boardX, int boardY) {
    int buttonSize = static_cast<int>(squareSize * 0.8f);
    int totalWidth = (buttonSize * 4) + (BUTTON_SPACING * 3) + (BUTTON_PADDING * 2) + (BORDER_WIDTH * 2);
    int totalHeight = buttonSize + (BUTTON_PADDING * 2) + (BORDER_WIDTH * 2);
    
    // Check if we're past the middle of the screen horizontally
    bool showOnLeft = (boardX > screenWidth / 2);
    
    if (showOnLeft) {
        // Show dialog to the left of the square
        dialogRect.x = boardX - totalWidth - 10;
    } else {
        // Show dialog to the right of the square
        dialogRect.x = boardX + static_cast<int>(squareSize) + 10;
    }
    
    // Vertically center the dialog on the square
    dialogRect.y = boardY - (totalHeight - static_cast<int>(squareSize)) / 2;
    
    // Make sure dialog stays within screen bounds
    if (dialogRect.x < 0) dialogRect.x = 10;
    if (dialogRect.y < 0) dialogRect.y = 10;
    if (dialogRect.x + totalWidth > screenWidth) {
        dialogRect.x = screenWidth - totalWidth - 10;
    }
}

void UIPromotionDialog::loadPieceTextures() {
    for (auto& buttonInfo : promotionButtons) {
        buttonInfo.pieceTexture = loadPieceTexture(buttonInfo.pieceType, pawnColor);
    }
}

SDL_Texture* UIPromotionDialog::loadPieceTexture(PieceType type, Color color) {
    std::string imagePath = getPieceImagePath(type, color);
    SDL_Surface* surface = IMG_Load(imagePath.c_str());
    
    if (!surface) {
        std::cout << "Failed to load piece image: " << imagePath << " Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    if (!texture) {
        std::cout << "Failed to create texture from surface for: " << imagePath << " Error: " << SDL_GetError() << std::endl;
    }
    
    return texture;
}

std::string UIPromotionDialog::getPieceImagePath(PieceType type, Color color) {
    std::string colorPrefix = (color == WHITE) ? "W_" : "B_";
    std::string pieceName;
    
    switch (type) {
        case QUEEN:  pieceName = "Queen"; break;
        case ROOK:   pieceName = "Rook"; break;
        case BISHOP: pieceName = "Bishop"; break;
        case KNIGHT: pieceName = "Knight"; break;
        default:     pieceName = "Queen"; break; // Default to queen
    }
    
    return "images/" + colorPrefix + pieceName + ".png";
}

void UIPromotionDialog::update(Input& input) {
    if (!visible) return;
    
    // Update all button states using UIButton's built-in update logic
    for (auto& buttonInfo : promotionButtons) {
        if (buttonInfo.button) {
            buttonInfo.button->update(input);
        }
    }
}

void UIPromotionDialog::render(SDL_Renderer* renderer) {
    if (!visible) return;
    
    renderDialog(renderer);
    
    // Render buttons using UIButton's built-in rendering
    for (const auto& buttonInfo : promotionButtons) {
        if (buttonInfo.button) {
            buttonInfo.button->render(renderer);
        }
    }
    
    // Render piece textures on top of buttons
    renderPieceTextures(renderer);
}

void UIPromotionDialog::renderDialog(SDL_Renderer* renderer) {
    // Enable blending for transparency
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // Draw dialog background
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, 
                          backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(renderer, &dialogRect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, 
                          borderColor.b, borderColor.a);
    for (int i = 0; i < BORDER_WIDTH; ++i) {
        SDL_Rect borderRect = {
            dialogRect.x - i, dialogRect.y - i,
            dialogRect.w + 2 * i, dialogRect.h + 2 * i
        };
        SDL_RenderDrawRect(renderer, &borderRect);
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void UIPromotionDialog::renderPieceTextures(SDL_Renderer* renderer) {
    // Render piece images on top of the buttons using the same logic as piece.cpp
    for (const auto& buttonInfo : promotionButtons) {
        if (buttonInfo.pieceTexture && buttonInfo.button) {
            // Get the button's current visual rect (moves with button press)
            SDL_Rect buttonRect = buttonInfo.button->getVisualRect();
            SDL_FRect boardSquareRect = {
                static_cast<float>(buttonRect.x),
                static_cast<float>(buttonRect.y),
                static_cast<float>(buttonRect.w),
                static_cast<float>(buttonRect.h)
            };
            
            // Query texture dimensions
            int texW = 0, texH = 0;
            SDL_QueryTexture(buttonInfo.pieceTexture, NULL, NULL, &texW, &texH);
            if (texW == 0 || texH == 0) continue;

            float textureAspectRatio = static_cast<float>(texW) / static_cast<float>(texH);
            
            SDL_FRect fittedRect;

            // Fit texture to button maintaining aspect ratio
            if (boardSquareRect.w / textureAspectRatio <= boardSquareRect.h) {
                fittedRect.w = boardSquareRect.w;
                fittedRect.h = boardSquareRect.w / textureAspectRatio;
            } else {
                fittedRect.h = boardSquareRect.h;
                fittedRect.w = boardSquareRect.h * textureAspectRatio;
            }

            // Apply the same scale factor as pieces
            const float pieceScaleFactor = 1.25f; // Same as piece.cpp
            SDL_FRect destRect;
            destRect.w = fittedRect.w * pieceScaleFactor;
            destRect.h = fittedRect.h * pieceScaleFactor;

            // Center horizontally
            destRect.x = boardSquareRect.x + (boardSquareRect.w - destRect.w) / 2.0f;
            
            // Apply the same visual vertical offset as pieces
            const float visualVerticalOffset = -8.5f; // Same as piece.cpp
            destRect.y = boardSquareRect.y + (boardSquareRect.h - destRect.h) / 2.0f + visualVerticalOffset;

            SDL_RenderCopyF(renderer, buttonInfo.pieceTexture, NULL, &destRect);
        }
    }
}
