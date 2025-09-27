#pragma once

#include <unordered_map>
#include <vector>

struct Position;
struct Move;

class MoveCache {
private:
    std::unordered_map<Position, std::vector<Move>> cachedMoves;
    bool isDirty = true;

public:
    const std::vector<Move>& getMovesFrom(Position pos);
    void invalidate(Position pos);
    void invalidateAll();
};