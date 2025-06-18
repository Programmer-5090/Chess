#pragma once

#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "../enums.h"

struct Move;
class Board; // Forward declaration of Board

class Piece{
    public:
        Piece(Color color, PieceType type, SDL_Renderer* renderer);
        virtual ~Piece();

        virtual std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const = 0;

        bool canCapture(int targetRow, int targetCol, const Board& board) const;

        virtual void setHasMoved(bool moved);

        void draw(SDL_FRect& rect);

        void setPosition(int r, int c);

        std::string stringPieceType() const;

        // Getters for attributes
        Color getColor() const { 
            return color; 
        }
        PieceType getType() const { 
            return type; 
        }
        std::pair<int, int> getPosition() const { 
            return position; 
        }
        int getValue() const { 
            return value; 
        }
        bool getHasMoved() const { 
            return hasMoved; 
        }
        SDL_Texture* getTexture() const { 
            return pieceText; 
        }
        
        protected:
        SDL_Surface* pieceImg;
        SDL_Texture* pieceText; 
        SDL_Renderer* renderer;
        std::pair<int , int> position;
        Color color;
        PieceType type;
        int value;
        bool hasMoved;
        bool in_bounds(int r,int c) const;
        std::string name;
};

struct Move
{
    std::pair<int, int> startPos;
    std::pair<int, int> endPos;
    const Piece* piece;
    const Piece* capturedPiece;
    bool castling;
    bool isKingSide;
    bool isQueenSide;
    

    // Constructor for convenience
    Move(std::pair<int, int> start, 
        std::pair<int, int> end, 
        const Piece* movedPiece, const Piece* takenPiece, bool isCastling = false, bool isKingSide = false, bool isQueenSide = false) : 
        startPos(start), endPos(end), piece(movedPiece), capturedPiece(takenPiece), castling(isCastling), isKingSide(isKingSide), isQueenSide(isQueenSide) {}
};

