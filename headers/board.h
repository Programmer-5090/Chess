#ifndef BOARD_H
#define BOARD_H

#include "enums.h"
#include "pieceClasses.h"
#include <SDL2/SDL.h> 
#include <iostream>
#include <memory>
#include <vector>

// Forward declaration
struct Move;
class Piece;

class Board{
    public:

        Board(int width, int height, float offSet);
        
        ~Board();

        void initializeBoard(SDL_Renderer* gameRenderer);

        void updateBoardState();

        void draw(SDL_Renderer* renderer, const std::pair<int, int>* selectedSquare, const std::vector<Move>* possibleMoves);

        Piece* getPieceAt(int r, int c) const;

        bool screenToBoardCoords(int screenX, int screenY, int& boardR, int& boardC) const;

        SDL_FRect getSquareRect(int r, int c) const;

        void movePiece(const Move& move);

        std::vector<Move> getAllLegalMoves(Color color, bool generateCastlingMoves) const;

        bool isKingInCheck(Color color) const;

        bool checkIfMoveRemovesCheck(const Move& move);

        bool isCheckMate(Color color);
        void clearEnPassantFlags(Color colorToClear);

    public:
        std::array<std::array<std::unique_ptr<Piece>, 8>, 8> boardState;
    
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

};

#endif // BOARD_H