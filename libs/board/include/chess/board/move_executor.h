#ifndef MOVE_EXECUTOR_H
#define MOVE_EXECUTOR_H

#include <vector>
#include <memory>
#include <SDL.h>
#include <chess/enums.h>

// Forward declarations
class Board;
class Piece;

// Move structure
struct Move {
    std::pair<int, int> startPos;
    std::pair<int, int> endPos;
    const Piece* piece = nullptr;
    const Piece* capturedPiece = nullptr;
    CastlingType castlingType = CastlingType::NONE;
    bool isPromotion = false;
    PieceType promotionType = QUEEN;

            Move(std::pair<int, int> start = {-1, -1},
                     std::pair<int, int> end = {-1, -1},
                     const Piece* movedPiece = nullptr,
                     const Piece* takenPiece = nullptr,
                     CastlingType castling = CastlingType::NONE,
                     bool isPromotion_ = false,
                     PieceType promotionType_ = QUEEN)
                    : startPos(start), endPos(end), piece(movedPiece), capturedPiece(takenPiece),
                        castlingType(castling), isPromotion(isPromotion_), promotionType(promotionType_) {}

    bool isCastling() const { return castlingType != CastlingType::NONE; }
};

// UndoMove (shared)
struct UndoMove {
    bool movedPiecePrevHasMoved = false;
    bool rookPrevHasMoved = false;
    bool wasCapture = false;
    CastlingType castlingType = CastlingType::NONE;
    bool wasEnPassant = false;
    bool wasPromotion = false;
    PieceType originalPromotionType = PAWN;
    std::unique_ptr<Piece> promotedPawn;
    std::pair<int, int> capturedPiecePos = {-1, -1};
    std::unique_ptr<Piece> capturedPiece;
    int rookRow = -1;
    int rookFromCol = -1;
    int rookToCol = -1;
};


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
    void undoMove(const Move& move, UndoMove& undoInfo);
    const Move* getLastMovePtr() const;
    void undoPieceMove(int r1, int c1, int r2, int c2, bool prevHasMoved);
};

#endif // MOVE_EXECUTOR_H