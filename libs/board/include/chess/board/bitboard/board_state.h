#ifndef BOARD_STATE_H
#define BOARD_STATE_H
#include <array>
#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>
#include <chess/board/pieces/piece_const.h>

namespace chess {

struct PieceList {
    std::vector<int> squares;
    int count() const { return static_cast<int>(squares.size()); }
    void add(int sq) { squares.push_back(sq); }
    void remove(int sq) {
        auto it = std::find(squares.begin(), squares.end(), sq);
        if (it != squares.end()) squares.erase(it);
    }
    void move(int from, int to) {
        auto it = std::find(squares.begin(), squares.end(), from);
        if (it != squares.end()) *it = to;
    }
    void clear() { squares.clear(); }
};

struct BitboardState {
    std::array<int,64> square{};
    
    PieceList pawns[2];
    PieceList knights[2];
    PieceList bishops[2];
    PieceList rooks[2];
    PieceList queens[2];
    
    int kingSquare[2]{};
    bool whiteToMove = true;
    
    uint32_t gameState = 0;
    uint64_t zobristKey = 0;
    
    std::vector<uint64_t> repetitionHistory;
    std::vector<uint64_t> zobristHistory;
    
    int plyCount = 0;
    int fiftyMoveCounter = 0;
    
    void clear();
    int getPieceAt(int r, int c) const;
    void loadFromFEN(const std::string& fen);
};

constexpr uint32_t CR_WHITE_K = 1U;
constexpr uint32_t CR_WHITE_Q = 2U;
constexpr uint32_t CR_BLACK_K = 4U;
constexpr uint32_t CR_BLACK_Q = 8U;

constexpr uint32_t WHITE_CASTLE_MASK = ~(CR_WHITE_K | CR_WHITE_Q);
constexpr uint32_t BLACK_CASTLE_MASK = ~(CR_BLACK_K | CR_BLACK_Q);

inline int getEPFile(uint32_t state) {
    return static_cast<int>((state >> 4) & 15) - 1;
}
inline void setEPFile(uint32_t& state, int file) {
    state &= ~(15U << 4);
    state |= ((file + 1) & 15) << 4;
}
inline int getCapturedPiece(uint32_t state) {
    return (state >> 8) & 63;
}
inline void setCapturedPiece(uint32_t& state, int pieceType) {
    state &= ~(63U << 8);
    state |= (pieceType & 63) << 8;
}
inline int getFiftyMoveCounter(uint32_t state) {
    return state >> 14;
}
inline void setFiftyMoveCounter(uint32_t& state, int counter) {
    state &= 0x3FFF;
    state |= (counter & 0x3FFFF) << 14;
}

inline int toIndex(int row, int col) { return row * 8 + col; }
inline int toRow(int idx) { return idx / 8; }
inline int toCol(int idx) { return idx % 8; }
inline int colorIdx(bool whiteToMove) { return whiteToMove ? 0 : 1; }

} // namespace chess
#endif // BOARD_STATE_H