#ifndef MOVE_EXECUTOR_H
#define MOVE_EXECUTOR_H
#include <vector>
#include <memory>
#include <SDL.h>

// Forward declarations to avoid heavy dependencies in the header
class Board;
class PieceManager;
struct UndoMove;
struct Move;


class MoveExecutor {
private:
    Board* board;
    std::vector<Move> moveHistory; // Optional: keep track of move history

public:
    MoveExecutor(Board* board);
    ~MoveExecutor() = default;
    std::unique_ptr<Piece> captureAndRemovePiece(const Piece* pieceToCapture, std::pair<int, int>& capturedPos);
    void restorePieceToManager(std::unique_ptr<Piece> piece, int row, int col);
    void executeCastlingRookMove(int kingRow, CastlingType castlingType, UndoMove& undo);
    std::unique_ptr<Piece> createPromotedPiece(PieceType promotionType, Color color, SDL_Renderer* renderer);
    UndoMove executeMove(const Move& move, bool trackUndo = true);
    void undoMove(const Move& move, const UndoMove& undoInfo);
    void undoPieceMove(int r1, int c1, int r2, int c2, bool prevHasMoved);
    bool isLegalMove(const Move& move) const;
};

#endif // MOVE_EXECUTOR_H