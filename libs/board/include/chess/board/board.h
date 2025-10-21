#ifndef BOARD_H
#define BOARD_H

#include <SDL.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <chess/board/pieces/piece.h>
#include <chess/ui/input.h>
#include <chess/board/move_executor.h>

class PieceManager;
class BoardRenderer;
class UIPromotionDialog;
struct RenderContext;
class MoveExecutor;


namespace chess {
    struct BitboardState;
    class MoveGeneratorBB;
    struct BBMove;
}

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
    int screenHeight;
    int screenWidth;
    float offSet;
    float startXPos;
    float startYPos;
    float endXPos;
    float endYPos;
    float squareSide;
    bool isFlipped = false;


    // Deprected - to be removed
    std::array<std::array<SDL_FRect, 8>, 8> boardGrid;
    std::array<std::array<Piece*, 8>, 8> pieceGrid;
    
    
    std::unique_ptr<PieceManager> pieceManager;
    std::unique_ptr<BoardRenderer> boardRenderer;
    std::unique_ptr<UIPromotionDialog> promotionDialog;
    std::unique_ptr<MoveExecutor> moveExecutor;

    std::vector<std::unique_ptr<Piece>> whiteCapturedPieces;
    std::vector<std::unique_ptr<Piece>> blackCapturedPieces;

    // Bitboard related members
    std::unique_ptr<chess::BitboardState> bbState;
    std::unique_ptr<chess::MoveGeneratorBB> bbGenerator;

    std::string startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Color currentPlayer = WHITE;
    // Initialize these in the constructor after bbState is constructed to avoid
    // referencing an incomplete type here (BitboardState is forward-declared).
    int halfMoveClock = 0;
    int fullMoveNumber = 1;

    

public:
    Board(int width, int height, float offSet);
    ~Board();

    void loadFEN(const std::string& fen, SDL_Renderer* gameRenderer);
    void initializeBoard(SDL_Renderer* gameRenderer);
    void resetBoard(SDL_Renderer* gameRenderer);
    void clearPieceGridAndPieces();
    void clearEnPassantFlags(Color colorToClear);
    void setStartFEN(const std::string& fen) { startFEN = fen; }

    void setFlipped(bool flipped);
    
    void draw(SDL_Renderer* renderer, const std::pair<int, int>* selectedSquare = nullptr, 
              const std::vector<Move>* possibleMoves = nullptr);
    
    bool screenToBoardCoords(int screenX, int screenY, int& boardR, int& boardC) const;
    SDL_FRect getSquareRect(int r, int c) const;
    
    Piece* getPieceAt(int r, int c) const;
    PieceManager* getPieceManager() const;
    
    UndoMove  executeMove(const Move& move, bool trackUndo = true);
    void undoMove(const Move& move, UndoMove& undo);

    friend class MoveExecutor;
    friend class AI;

    friend class FENUtil;
    const Move* getLastMovePtr() const;
    
    std::vector<Move> getAllLegalMoves(Color color, bool generateCastlingMoves = true) const;
    void getAllLegalMoves(Color color, std::vector<Move>& out, bool generateCastlingMoves = true) const;
    
    std::vector<Move> getAllPseudoLegalMoves(Color color, bool generateCastlingMoves = true) const;
    void getAllPseudoLegalMoves(Color color, std::vector<Move>& out, bool generateCastlingMoves = true) const;
    
    std::vector<Move> getAllPseudoLegalMovesBB(Color color, bool generateCastlingMoves = true);
    void getAllPseudoLegalMovesBB(Color color, std::vector<Move>& out, bool generateCastlingMoves = true);
    
    void syncBitboardState();
    
    Move bbMoveToMove(const chess::BBMove& bbMove) const;
    
    bool isKingInCheck(Color color) const;
    bool isSquareAttacked(int r, int c, Color byColor) const;
    bool checkIfMoveRemovesCheck(const Move& move);
    bool isCheckMate(Color color);
    bool isStaleMate(Color color);
    
    bool isPinnedPiece(int pieceRow, int pieceCol, Color pieceColor) const;
    bool wouldMoveCauseDiscoveredCheck(const Move& move, Color movingColor) const;
    
    void updatePromotionDialog(Input& input);
    void renderPromotionDialog(SDL_Renderer* renderer);
    bool isPromotionDialogActive() const;
    
    // Utilities used by MoveExecutor and other internals
    void updatePiecePositionInManager(Piece* piece);
    void handlePawnPromotion(const Piece* pawn, int row, int col);
    void promotePawnTo(int row, int col, Color color, PieceType pieceType, SDL_Renderer* renderer);
    void showPromotionDialog(int row, int col, Color color, SDL_Renderer* renderer);
    void logCapturedPieces(Color capturer) const;
    
    void updatePieceGrid();

    std::array<std::array<Piece*, 8>, 8> getPieceGrid() const { return pieceGrid;}
    std::string getStartFEN() const { return startFEN; }
    std::string getCurrentFEN() const;
    bool getIsFlipped() const { return isFlipped; }
    Color getCurrentPlayer() const { return currentPlayer; }
    void setCurrentPlayer(Color player) { currentPlayer = player; }
    int getHalfMoveClock() const { return halfMoveClock; }
    int getFullMoveNumber() const { return fullMoveNumber; }

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

    bool isKingInCheck(Color color, const Move* hypotheticalMove);
};

#endif