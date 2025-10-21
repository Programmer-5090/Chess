#ifndef BOARD_BB_H
#define BOARD_BB_H

#include <SDL.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <chess/ui/input.h>
#include <chess/enums.h>
#include <chess/board/pieces/pieces.h>

// Forward declarations
class BoardRenderer;
class UIPromotionDialog;
struct RenderContext;



namespace chess {
    struct BitboardState;
    class MoveGeneratorBB;
    struct BBMove;
    struct UndoState;
    class MoveExecutorBB;
}

class BoardBB {
private:
    int screenHeight;
    int screenWidth;
    float offSet;
    float startXPos;
    float startYPos;
    float endXPos;
    float endYPos;
    float squareSide;
    bool isFlipped = false;


    std::unique_ptr<BoardRenderer> boardRenderer;
    std::unique_ptr<UIPromotionDialog> promotionDialog;
    std::array<std::array<SDL_FRect, 8>, 8> boardGrid;

    // UI pieces grid mapping directly to bbState (unique_ptr owner)
    #include <chess/board/pieces/pieces.h>
    std::array<std::array<std::unique_ptr<Piece>, 8>, 8> pieceGrid;
    // store renderer used to create piece textures
    SDL_Renderer* uiRenderer = nullptr;


    // Bitboard related members
    std::unique_ptr<chess::BitboardState> bbState;
    std::unique_ptr<chess::MoveGeneratorBB> bbGenerator;
    std::unique_ptr<chess::MoveExecutorBB> moveExecutor;
    
    std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Color currentPlayer = WHITE;
    int halfMoveClock = 0;
    int fullMoveNumber = 1;

    std::vector<chess::BBMove> whiteMoves;
    std::vector<chess::BBMove> blackMoves;

public:
    BoardBB(int width, int height, float offSet);
    ~BoardBB();

    friend class AI;

    void loadFEN(const std::string& fen, SDL_Renderer* gameRenderer);
    void syncUIFromBBState(SDL_Renderer* gameRenderer);
    void initializeBoard(SDL_Renderer* gameRenderer);
    void resetBoard(SDL_Renderer* gameRenderer);
    void setStartFEN(const std::string& fen) { startFEN = fen; }

    void setFlipped(bool flipped);

    void draw(SDL_Renderer* renderer, const std::pair<int, int>* selectedSquare = nullptr,
              const std::vector<chess::BBMove>* possibleMoves = nullptr);

    bool screenToBoardCoords(int screenX, int screenY, int& boardR, int& boardC) const;
    SDL_FRect getSquareRect(int r, int c) const;


    chess::UndoState executeMove(const chess::BBMove& move, bool trackUndo = true);
    void undoMove(const chess::BBMove& move, chess::UndoState& undo);

    
    bool isCheckMate(Color color);
    bool isStaleMate(Color color);
    void handlePawnPromotion(const Piece* pawn, int row, int col);
    void updatePromotionDialog(Input& input);
    void renderPromotionDialog(SDL_Renderer* renderer);
    bool isPromotionDialogActive() const;
    
    int getPieceAt(int r, int c) const;
    int64_t getLastState() const;
    std::vector<chess::BBMove> getAllLegalMoves(Color color) const;
    std::string getStartFEN() const { return startFEN; }
    std::string getCurrentFEN() const;
    bool getIsFlipped() const { return isFlipped; }
    Color getCurrentPlayer() const { return currentPlayer; }
    int getHalfMoveClock() const { return halfMoveClock; }
    int getFullMoveNumber() const { return fullMoveNumber; }

    void setCurrentPlayer(Color player) { currentPlayer = player; }


};

#endif