#ifndef BOARD_H
#define BOARD_H

#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <SDL.h>
#include "enums.h"
#include "pieceClasses.h"

// Forward declarations
struct Move;
class Piece;
class UIPromotionDialog;

// Fine-grained profiling for make/unmake (updated inside board.cpp)
struct MakeUnmakeProfile {
    long long clearEPTime = 0;              // microseconds spent clearing en passant flags
    long long captureHandleTime = 0;        // apply: handling captures (including EP)
    long long movePieceTime = 0;            // apply: moving piece, setting flags/positions
    long long castlingTime = 0;             // apply: rook bookkeeping/moves
    long long promotionTime = 0;            // apply: promotions (if any)

    long long unmakeMoveBackTime = 0;       // unmake: moving piece back
    long long unmakeRestoreCaptureTime = 0; // unmake: restoring captured piece
    long long unmakeCastlingTime = 0;       // unmake: undoing castling

    long long applyCalls = 0;               // number of applyMoveWithUndo calls
    long long unmakeCalls = 0;              // number of unmakeMove calls
};

extern MakeUnmakeProfile g_muProfile;

// Reversible state for unmaking a move
struct UndoMove {
    bool movedPiecePrevHasMoved = false;
    bool rookPrevHasMoved = false;
    bool wasCapture = false;
    bool wasCastling = false;
    bool wasKingSide = false;
    bool wasQueenSide = false;
    bool wasPromotion = false;
    PieceType promotedFrom = PAWN;
    int rookRow = -1;
    int rookFromCol = -1;
    int rookToCol = -1;
    std::unique_ptr<Piece> capturedPiece; // owned pointer to restore
    std::pair<int, int> capturedPiecePos = {-1, -1}; // actual position of captured piece (for en passant)
    // En passant bookkeeping
    bool prevEnPassantExists = false; // whether any pawn had EP eligible flag before the move
    int prevEPRow = -1;
    int prevEPCol = -1;
    bool newEnPassantSet = false; // whether this move created a new EP eligibility on the moved pawn
    int newEPRow = -1;
    int newEPCol = -1;
    // King position bookkeeping (for fast check detection)
    bool movedKing = false;
    int prevKingRow = -1;
    int prevKingCol = -1;
};

class Board {
public:
    Board(int width, int height, float offSet);
    ~Board();

    void loadFEN(const std::string& fen, SDL_Renderer* gameRenderer);
    void initializeBoard(SDL_Renderer* gameRenderer);
    void setFlipped(bool flipped);
    void resetBoard();
    void updateBoardState();
    void draw(SDL_Renderer* renderer, const std::pair<int, int>* selectedSquare, const std::vector<Move>* possibleMoves);

    Piece* getPieceAt(int r, int c) const;
    bool screenToBoardCoords(int screenX, int screenY, int& boardR, int& boardC) const;
    SDL_FRect getSquareRect(int r, int c) const;
    void movePiece(const Move& move);
    // New reversible make/unmake pair used for search/perft
    void applyMoveWithUndo(const Move& move, UndoMove& undo);
    void unmakeMove(const Move& move, const UndoMove& undo);

    std::vector<Move> getAllLegalMoves(Color color, bool generateCastlingMoves) const;
    bool isKingInCheck(Color kingColor) const;
    bool checkIfMoveRemovesCheck(const Move& move);
    bool isCheckMate(Color color);
    void clearEnPassantFlags(Color colorToClear);
    void promotePawnTo(int row, int col, Color color, PieceType pieceType, SDL_Renderer* renderer);
    void showPromotionDialog(int row, int col, Color color, SDL_Renderer* renderer);
    void updatePromotionDialog(class Input& input);
    void renderPromotionDialog(SDL_Renderer* renderer);
    bool isPromotionDialogActive() const;
    // Fast attack detector used for quick check testing
    bool isSquareAttacked(int r, int c, Color byColor) const;

public:
    std::array<std::array<std::unique_ptr<Piece>, 8>, 8> boardState;

    std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

private:
    int screenWidth;
    int screenHeight;
    float offSet;
    float startXPos;
    float startYPos;
    float endXPos;
    float endYPos;
    float squareSide;
    std::array<std::array<SDL_FRect, 8>, 8> boardGrid;
    std::vector<std::unique_ptr<Piece>> whiteCapturedPieces;
    std::vector<std::unique_ptr<Piece>> blackCapturedPieces;
    std::unique_ptr<UIPromotionDialog> promotionDialog;
    bool isFlipped = false;
    // Cache king positions to avoid scanning every time in isKingInCheck
    std::pair<int,int> whiteKingPos = {-1,-1};
    std::pair<int,int> blackKingPos = {-1,-1};
};

#endif // BOARD_H
