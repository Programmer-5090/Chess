#ifndef BBMOVE_EXEC_H
#define BBMOVE_EXEC_H

#include <cstdint>
#include <chess/board/bitboard/move.h>
#include <chess/board/bitboard/board_state.h>

namespace chess {

struct UndoState {
    uint32_t previousGameState;
    uint64_t previousZobrist;
    int capturedPiece;
    int previousFiftyMove;
    int previousPlyCount;
};

class BBMoveExecutor {
public:
    BBMoveExecutor(BitboardState& state) : state(state) {}
    
    UndoState makeMove(const BBMove& move);
    void unmakeMove(const BBMove& move, const UndoState& undo);
    
private:
    BitboardState& state;
};

} // namespace chess

#endif // BBMOVE_EXEC_H
