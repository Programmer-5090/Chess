#pragma once

// Forward declarations to avoid heavy dependencies in the header
class BoardState;
class PieceManager;
struct Move;
struct UndoInfo;

class MoveExecutor {
private:
    BoardState& boardState;
    PieceManager& pieceManager;

public:
    UndoInfo executeMove(const Move& move);
    void undoMove(const Move& move, const UndoInfo& undoInfo);
    bool isLegalMove(const Move& move) const;
};