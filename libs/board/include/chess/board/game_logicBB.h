#ifndef GAME_LOGICBB_H
#define GAME_LOGICBB_H

#include <chess/enums.h>
#include <vector>
#include <utility>
#include <memory>

// Forward declarations
class BoardBB;

namespace chess {
    struct BBMove;
}

class GameLogicBB{
public:
    GameLogicBB();

    void switchPlayer();
    void handleMouseClick(int mouseX, int mouseY, BoardBB& board, bool leftMouseClicked);
    void makeMove(const chess::BBMove& move, BoardBB& board);

    int getPieceAt(int row, int col, const BoardBB& board) const;

    Color getCurrentPlayer() const;
    const std::pair<int, int>* getSelectedPieceSquare() const;
    const std::vector<chess::BBMove>& getPossibleMoves() const;
    void clearSelection();

private:
    Color currentPlayer;
    std::pair<int, int> selectedPieceSquare;
    bool pieceIsSelected;
    std::vector<chess::BBMove> possibleMoves;
};

#endif // GAME_LOGICBB_H
