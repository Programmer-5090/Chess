#pragma once

#include <functional>
#include <memory>

// Forward declarations to avoid pulling UI headers into core logic
class UIPromotionDialog;
struct Position;
struct Move;
enum class Color;
enum class PieceType;

class PromotionHandler {
private:
    std::unique_ptr<UIPromotionDialog> dialog;
    std::function<void(Position, PieceType)> onPromotionCallback;

public:
    bool needsPromotion(const Move& move) const;
    void showPromotionDialog(Position pos, Color color);
    void handleSelection(PieceType type);
    bool isDialogActive() const;
};