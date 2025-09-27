#ifndef BOARD_H
#define BOARD_H

#include <SDL.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include "pieces/piece.h"
#include "input.h"

// Forward declarations
class PieceManager;
class BoardRenderer;
class UIPromotionDialog;
struct RenderContext;
class MoveExecutor;
struct Move;
struct UndoMove;


// Profiling structure (assuming it exists based on usage)
struct MakeUnmakeProfile {
    long long clearEnPassantFlags = 0;
    long long captureHandling = 0;
    long long movePiece = 0;
    long long castlingBookkeeping = 0;
    long long promotionHandling = 0;
    long long applyTime = 0;
    long long applyCalls = 0;
    long long unmakeCastling = 0;
    long long unmakeMoveBack = 0;
    long long unmakeRestoreCap = 0;
    long long unmakeTime = 0;
    long long unmakeCalls = 0;
};

extern MakeUnmakeProfile g_muProfile;

class Board {
private:
    // Screen and board dimensions
    int screenHeight;
    int screenWidth;
    float offSet;
    float startXPos;
    float startYPos;
    float endXPos;
    float endYPos;
    float squareSide;
    bool isFlipped = false;

    // Board layout and pieces
    std::array<std::array<SDL_FRect, 8>, 8> boardGrid;
    std::array<std::array<Piece*, 8>, 8> pieceGrid; // Non-owning pointers
    
    // Piece management
    std::unique_ptr<PieceManager> pieceManager;
    std::unique_ptr<BoardRenderer> boardRenderer;
    std::unique_ptr<UIPromotionDialog> promotionDialog;
    
    // Captured pieces (owning containers)
    std::vector<std::unique_ptr<Piece>> whiteCapturedPieces;
    std::vector<std::unique_ptr<Piece>> blackCapturedPieces;
    
    // Game state
    std::string startFEN = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
    Move lastMove;

    // Helper methods
    void castleRook(int row, int fromCol, int toCol);
    void logCapturedPieces(Color capturer) const;
    void updatePiecePositionInManager(Piece* piece);
    std::unique_ptr<Piece> removePieceFromManagerById(unsigned int id);
    void handlePawnPromotion(const Piece* pawn, int row, int col);
    void promotePawnTo(int row, int col, Color color, PieceType pieceType, SDL_Renderer* renderer);
    void showPromotionDialog(int row, int col, Color color, SDL_Renderer* renderer);
    void addPieceToManager(Piece* piece); // Assuming this exists based on usage

public:
    Board(int width, int height, float offSet);
    ~Board();

    // Initialization and setup
    void loadFEN(const std::string& fen, SDL_Renderer* gameRenderer);
    void initializeBoard(SDL_Renderer* gameRenderer);
    void resetBoard(SDL_Renderer* gameRenderer);
    void clearPieceGridAndPieces();
    void clearEnPassantFlags(Color colorToClear);
    void setStartFEN(const std::string& fen) { startFEN = fen; }

    
    // Board configuration
    void setFlipped(bool flipped);
    
    // Drawing and rendering
    void draw(SDL_Renderer* renderer, const std::pair<int, int>* selectedSquare = nullptr, 
              const std::vector<Move>* possibleMoves = nullptr);
    
    // Coordinate conversion
    bool screenToBoardCoords(int screenX, int screenY, int& boardR, int& boardC) const;
    SDL_FRect getSquareRect(int r, int c) const;
    
    // Piece access
    Piece* getPieceAt(int r, int c) const;
    PieceManager* getPieceManager() const;
    
    // Move operations
    void movePiece(const Move& move);
    UndoMove  executeMove(const Move& move);
    void undoMove(const Move& move, UndoMove& undo);
    
    // Game logic
    std::vector<Move> getAllLegalMoves(Color color, bool generateCastlingMoves = true) const;
    bool isKingInCheck(Color color) const;
    bool isSquareAttacked(int r, int c, Color byColor) const;
    bool checkIfMoveRemovesCheck(const Move& move);
    bool isCheckMate(Color color);
    bool isStaleMate(Color color);
    
    // Promotion dialog management
    void updatePromotionDialog(Input& input);
    void renderPromotionDialog(SDL_Renderer* renderer);
    bool isPromotionDialogActive() const;
    
    // Board state update (placeholder)
    void updatePieceGrid();

    // Accessors
    std::array<std::array<Piece*, 8>, 8> getPieceGrid() const { return pieceGrid;}
    std::string getStartFEN() const { return startFEN; }
    bool getIsFlipped() const { return isFlipped; }

    void addCapturedPiece(Color color, std::unique_ptr<Piece> piece) {
        if(color == WHITE){
            whiteCapturedPieces.push_back(std::move(piece));
        }
        else{
            blackCapturedPieces.push_back(std::move(piece));
        }
    }

    std::vector<std::unique_ptr<Piece>>& getCapturedPieces(Color color) {
        return (color == WHITE) ? whiteCapturedPieces : blackCapturedPieces;
    }

    // New overload: check king-in-check after an optional hypothetical move.
    // This is non-const because it temporarily mutates the non-owning pieceGrid to evaluate the outcome.
    bool isKingInCheck(Color color, const Move* hypotheticalMove);
};

#endif // BOARD_H