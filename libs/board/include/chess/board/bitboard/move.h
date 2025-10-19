#ifndef MOVE_H
#define MOVE_H
#include <cstdint>
#include <string>

namespace chess {

struct BBMove {
    static constexpr uint16_t START_MASK  = 0b0000000000111111;
    static constexpr uint16_t TARGET_MASK = 0b0000111111000000;
    static constexpr uint16_t FLAG_MASK   = 0b1111000000000000;
    
    enum Flag : uint16_t {
        None = 0,
        EnPassantCapture = 1,
        Castling = 2,
        PromoteToQueen = 3,
        PromoteToKnight = 4,
        PromoteToRook = 5,
        PromoteToBishop = 6,
        PawnTwoForward = 7
    };
    
    uint16_t value;
    
    BBMove() : value(0) {}
    BBMove(uint16_t v) : value(v) {}
    BBMove(int start, int target, Flag flag = None) 
        : value(start | (target << 6) | (static_cast<uint16_t>(flag) << 12)) {}
    
    int startSquare() const { return value & START_MASK; }
    int targetSquare() const { return (value & TARGET_MASK) >> 6; }
    Flag flag() const { return static_cast<Flag>(value >> 12); }
    
    bool isPromotion() const {
        Flag f = flag();
        return f >= PromoteToQueen && f <= PromoteToBishop;
    }
    
    // Note: This requires passing BitboardState to check capture
    // For now, just check if flag indicates it's not a special move
    bool isCapture() const {
        // Can't determine without board state, would need separate parameter
        return false; // Placeholder - should be determined at generation time
    }
    
    std::string toString() const; // e.g., "e2-e4"
};
} // namespace chess
#endif // MOVE_H