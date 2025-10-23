#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <chess/enums.h>
#include <vector>
#include <utility>
#include <memory>

// Forward declarations
class Board;
class BoardBB;
struct Move;
class Piece;
class AI;

namespace chess {
    struct BBMove;
}

class GameLogic {
public:
    GameLogic();
    ~GameLogic();

    void switchPlayer(Board& board);
    void handleMouseClick(int mouseX, int mouseY, Board& board, bool leftMouseClicked);
    void makeMove(const Move& move, Board& board);
    void update(Board& board);

    Piece* getPieceAt(int row, int col, const Board& board) const;

    Color getCurrentPlayer(const Board& board) const;
    const std::pair<int, int>* getSelectedPieceSquare() const;
    const std::vector<Move>& getPossibleMoves() const;
    void clearSelection();

    void setAI(std::shared_ptr<AI> aiInstance, Color aiColor);
    bool isAITurn(const Board& board) const;
    void makeAIMove(Board& board);

private:
    Color aiPlayer;
    std::pair<int, int> selectedPieceSquare;
    bool pieceIsSelected;
    std::vector<Move> possibleMoves;
    std::shared_ptr<AI> ai;
    bool aiMovePending = false;
};

#endif // GAME_LOGIC_H
