#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <chess/enums.h>
#include <vector>
#include <utility>
#include <memory>

// Forward declarations
class Board;
struct Move;
class Piece;

class GameLogic {
public:
    GameLogic();

    void switchPlayer();
    void handleMouseClick(int mouseX, int mouseY, Board& board, bool leftMouseClicked);
    void makeMove(const Move& move, Board& board);

    Piece* getPieceAt(int row, int col, const Board& board) const;

    Color getCurrentPlayer() const;
    const std::pair<int, int>* getSelectedPieceSquare() const;
    const std::vector<Move>& getPossibleMoves() const;
    void clearSelection();

private:
    Color currentPlayer;
    std::pair<int, int> selectedPieceSquare;
    bool pieceIsSelected;
    std::vector<Move> possibleMoves;
};

#endif // GAME_LOGIC_H
