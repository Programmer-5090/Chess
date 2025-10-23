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
#include <chess/enums.h>
#include <chess/utils/logger.h>
#include <sstream>

// Forward declarations
struct Move;
class Board;

class Piece {
public:
    unsigned int id;
    static unsigned int next_white_id;
    static unsigned int next_black_id;
    Piece(Color color, PieceType type, SDL_Renderer* renderer);
    virtual ~Piece();
    virtual std::unique_ptr<Piece> clone() const = 0;

    virtual std::vector<Move> getPseudoLegalMoves(const Board& board, bool generateCastlingMoves = true) const = 0;
    virtual void getPseudoLegalMoves(const Board& board, std::vector<Move>& out, bool generateCastlingMoves = true) const;

    bool canCapture(int targetRow, int targetCol, const Board& board) const;
    virtual void setHasMoved(bool moved);
    void draw(SDL_FRect& rect);
    void setPosition(int r, int c);
    std::string stringPieceType() const;

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

#endif // PIECE_H
