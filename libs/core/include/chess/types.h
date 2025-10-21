#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <utility>

namespace chess {

// Forward declarations
class Board;
class Piece;

// Common types
using Position = std::pair<int, int>;
using Row = int;
using Col = int;

// Game state types  
enum class Color {
    WHITE,
    BLACK,
    NONE
};

enum class GameResult {
    ONGOING,
    WHITE_WINS,
    BLACK_WINS,
    DRAW,
    STALEMATE
};

// Coordinate system
struct Square {
    Row row;
    Col col;
    
    Square() : row(0), col(0) {}
    Square(Row r, Col c) : row(r), col(c) {}
    
    bool operator==(const Square& other) const {
        return row == other.row && col == other.col;
    }
    
    bool operator!=(const Square& other) const {
        return !(*this == other);
    }
    
    bool isValid() const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
};

// Move representation
struct Move {
    Square from;
    Square to;
    bool isCapture = false;
    bool isCastling = false;
    bool isEnPassant = false;
    bool isPromotion = false;
    
    Move() = default;
    Move(Square f, Square t) : from(f), to(t) {}
    Move(Row fromRow, Col fromCol, Row toRow, Col toCol) 
        : from(fromRow, fromCol), to(toRow, toCol) {}
        
    bool operator==(const Move& other) const {
        return from == other.from && to == other.to;
    }
};

} // namespace chess

#endif // TYPES_H