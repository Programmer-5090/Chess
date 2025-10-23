#ifndef GAME_LOGICBB_H
#define GAME_LOGICBB_H

#include <chess/enums.h>
#include <chess/board/bitboard/move.h>
#include <vector>
#include <utility>
#include <memory>
#include <future>
#include <atomic>

// Forward declarations
class BoardBB;
class AI_BB;

class GameLogicBB{
public:
    GameLogicBB();

    void switchPlayer();
    void handleMouseClick(int mouseX, int mouseY, BoardBB& board, bool leftMouseClicked);
    void makeMove(const chess::BBMove& move, BoardBB& board);
    void update(BoardBB& board);
    void setAI(std::shared_ptr<class AI_BB> aiInstance, Color aiColor);
    void setAISettings(int searchDepth, unsigned threadCount);

    int getPieceAt(int row, int col, const BoardBB& board) const;

    Color getCurrentPlayer() const;
    void setCurrentPlayer(Color c) { currentPlayer = c; }
    const std::pair<int, int>* getSelectedPieceSquare() const;
    const std::vector<chess::BBMove>& getPossibleMoves() const;
    void clearSelection();

private:
    Color currentPlayer;
    std::pair<int, int> selectedPieceSquare;
    bool pieceIsSelected;
    std::vector<chess::BBMove> possibleMoves;
    // AI related members
    std::shared_ptr<AI_BB> ai;
    Color aiColor = NO_COLOR;
    std::future<std::pair<std::pair<chess::BBMove, int>, std::string>> aiFuture;
    std::atomic<bool> aiSearchRunning{false};
    int aiSearchDepth = 4;
    unsigned aiThreadCount = 1;
};

#endif // GAME_LOGICBB_H

