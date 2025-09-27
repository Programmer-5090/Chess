#pragma once

#include <array>
#include <vector>

// Forward declarations to keep dependencies light
class Piece;
struct Move;
struct Position;
using PieceId = unsigned int; // piece id alias used by board caches
struct GameStateFlags;

class BoardState {
private:
    std::array<std::array<PieceId, 8>, 8> squares;
    GameStateFlags* flags = nullptr; // opaque flags pointer

public:
    PieceId getPieceAt(Position pos) const;
    BoardState withMove(const Move& move) const; // immutable updates
    bool isSquareEmpty(Position pos) const;
    std::vector<Position> getOccupiedSquares() const;
};