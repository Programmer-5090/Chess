#ifndef PIECE_H
#define PIECE_H

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <utility>
#include <SDL.h>
#include <SDL_image.h>
#include "../enums.h"
#include "logger.h"
#include <sstream>

// Forward declarations
struct Move;
class Board;

class Piece {
public:
    unsigned int id; // Unique identifier for each piece instance
    // Static members to track next available IDs per color
    static unsigned int next_white_id;
    static unsigned int next_black_id;
    Piece(Color color, PieceType type, SDL_Renderer* renderer);
    virtual ~Piece();

    // Pure virtual function for move generation
    virtual std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const = 0;

    // Utility functions
    bool canCapture(int targetRow, int targetCol, const Board& board) const;
    virtual void setHasMoved(bool moved);
    void draw(SDL_FRect& rect);
    void setPosition(int r, int c);
    std::string stringPieceType() const;

    // Getters
    Color getColor() const { return color; }
    int getColorAsInt() const { return color == WHITE ? 8 : 16; }
    PieceType getType() const { return type; }
    std::pair<int, int> getPosition() const { return position; }
    virtual int getValue() const { return value; }
    virtual int getPoints() const { return points; }
    bool getHasMoved() const { return hasMoved; }
    SDL_Texture* getTexture() const { return pieceText; }
    SDL_Renderer* getRenderer() const { return renderer; }

protected:
    SDL_Surface* pieceImg;
    SDL_Texture* pieceText;
    SDL_Renderer* renderer;
    std::pair<int, int> position;
    Color color;
    PieceType type;
    int value;
    int points;
    bool hasMoved;
    std::string name;

    bool in_bounds(int r, int c) const;
};

// Move structure
struct Move {
    std::pair<int, int> startPos;
    std::pair<int, int> endPos;
    const Piece* piece;
    const Piece* capturedPiece;
    bool castling;
    bool isKingSide;
    bool isQueenSide;
        bool isPromotion;
        PieceType promotionType;

    // Constructor for convenience
        Move(std::pair<int, int> start,
                 std::pair<int, int> end,
                 const Piece* movedPiece,
                 const Piece* takenPiece = nullptr,
                 bool isCastling = false,
                 bool isKingSide = false,
                 bool isQueenSide = false,
                 bool isPromotion_ = false,
                 PieceType promotionType_ = QUEEN)
                : startPos(start), endPos(end), piece(movedPiece), capturedPiece(takenPiece),
                    castling(isCastling), isKingSide(isKingSide), isQueenSide(isQueenSide),
                    isPromotion(isPromotion_), promotionType(promotionType_) {}

    // Default constructor
    Move() : startPos({-1, -1}), endPos({-1, -1}), piece(nullptr), capturedPiece(nullptr),
             castling(false), isKingSide(false), isQueenSide(false),
             isPromotion(false), promotionType(QUEEN) {}
};

#endif // PIECE_H
