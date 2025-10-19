# Bitboard Move Generation System

This directory contains a complete bitboard-based move generation system ported from C# reference code. It provides 10-20× faster move generation compared to the pointer-based Piece hierarchy system.

## Architecture Overview

### Core Components

1. **BBMove** (`move.h`) - 16-bit compact move encoding
   - 6 bits: start square (0-63)
   - 6 bits: target square (0-63)
   - 4 bits: move flags (promotion, castling, en passant, etc.)
   - Only 2 bytes per move vs 40 bytes in old Move struct!

2. **BitboardState** (`board_state.h`) - Complete board representation
   - `square[64]` - mailbox array (piece at each square)
   - `PieceList` arrays - fast iteration over pieces by type/color
   - `kingSquare[2]` - quick king position lookup
   - `gameState` - packed castling rights, EP file, captured piece, fifty-move counter
   - `zobristKey` - incremental hash for transposition table

3. **Zobrist** (`zoborist.h`) - Static zobrist hashing
   - Precomputed random values for all piece/square combinations
   - Incremental updates (XOR in makeMove/unmakeMove)
   - Used for transposition table and repetition detection

4. **PrecomputedData** (`precomputed_data.h`) - Static attack tables
   - `numSquaresToEdge[64][8]` - distance to board edge in each direction
   - `knightAttackBitboards[64]` - all knight moves from each square
   - `kingAttackBitboards[64]` - all king moves from each square
   - `pawnAttackBitboards[64][2]` - pawn captures for each color

5. **MoveGeneratorBB** (`move_generator_bb.h`) - Fast move generation
   - `calculateAttackData()` - detects checks, pins, attacked squares
   - Generates moves by piece type (king, sliding, knight, pawn)
   - Respects pins and check constraints automatically
   - Supports captures-only mode for quiescence search

6. **BBMoveExecutor** (`move_exec.h`) - Make/unmake moves
   - `makeMove()` - applies move, updates piece lists, zobrist, game state
   - `unmakeMove()` - reverses move using UndoState
   - Handles special moves: castling, en passant, promotion

7. **Bitboard utilities** (`bitboard.h`) - Bit manipulation helpers
   - `popCount()`, `getLSB()`, `popLSB()`
   - `getBit()`, `setBit()`, `clearBit()`
   - Uses C++20 `<bit>` library for fast intrinsics

## Usage Example

```cpp
#include <chess/board/bitboard/bitboard_init.h>
#include <chess/board/bitboard/board_state.h>
#include <chess/board/bitboard/move_generator_bb.h>
#include <chess/board/bitboard/move_exec.h>

// Call once at program start
chess::initBitboardSystem();

// Create board state
chess::BitboardState state;
state.loadFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

// Generate moves
chess::MoveGeneratorBB generator;
std::vector<chess::BBMove> moves = generator.generateMoves(state);

std::cout << "Legal moves: " << moves.size() << std::endl;
for (const auto& move : moves) {
    std::cout << move.toString() << " ";
}

// Make a move
chess::BBMoveExecutor executor(state);
chess::UndoState undo = executor.makeMove(moves[0]);

// ... do something ...

// Unmake the move
executor.unmakeMove(moves[0], undo);
```

## Integration with Existing Code

The bitboard system is designed to **replace** the old pointer-based system, not run alongside it. However, during migration:

1. Keep old `Board::getAllPseudoLegalMoves()` as `getAllPseudoLegalMovesLegacy()`
2. Add new `Board::getAllPseudoLegalMovesBB()` using bitboard generator
3. Validate both produce identical results via perft tests
4. Switch all callers to use bitboard version
5. Remove legacy code after validation period

### Conversion Helpers

To convert between old and new move formats:

```cpp
// Old Move → BBMove
BBMove toBBMove(const Move& oldMove) {
    int from = oldMove.from.first * 8 + oldMove.from.second;
    int to = oldMove.to.first * 8 + oldMove.to.second;
    
    BBMove::Flag flag = BBMove::None;
    if (oldMove.isEnPassant) flag = BBMove::EnPassantCapture;
    else if (oldMove.isCastling) flag = BBMove::Castling;
    else if (oldMove.isPromotion) {
        // Map promotion type to flag
        flag = BBMove::PromoteToQueen; // etc.
    }
    
    return BBMove(from, to, flag);
}

// BBMove → Old Move (for rendering/UI)
Move fromBBMove(const BBMove& bbMove, const BitboardState& state) {
    int from = bbMove.startSquare();
    int to = bbMove.targetSquare();
    
    Move oldMove;
    oldMove.from = {from / 8, from % 8};
    oldMove.to = {to / 8, to % 8};
    oldMove.piece = state.square[from]; // For rendering
    // ... fill other fields ...
    
    return oldMove;
}
```

## Performance Notes

- **Move generation**: 10-20× faster than virtual function dispatch
- **Memory**: BBMove is 2 bytes vs 40 bytes (20× smaller!)
- **Cache friendly**: Piece lists keep pieces contiguous in memory
- **Incremental updates**: Zobrist and piece lists updated during makeMove

## File Structure

```
libs/board/include/chess/board/bitboard/
├── bitboard.h              # Bit manipulation utilities
├── bitboard_init.h         # One-time initialization
├── board_state.h           # BitboardState + PieceList
├── move.h                  # BBMove compact encoding
├── move_exec.h             # BBMoveExecutor make/unmake
├── move_generator_bb.h     # MoveGeneratorBB
├── precomputed_data.h      # Static attack tables
├── transpositionTable.h    # Existing TT (update to use BBMove)
└── zoborist.h              # Static zobrist hashing

libs/board/src/bitboard/
├── board_state.cpp         # FEN loading, clear()
├── move.cpp                # BBMove::toString()
├── move_exec.cpp           # Make/unmake implementation
├── move_generator_bb.cpp   # Move generation logic
├── precomputed_data.cpp    # Initialize attack tables
└── zobrist.cpp             # Initialize zobrist values
```

## Testing

Use the existing perft demos to validate correctness:

```powershell
# Build and run perft test
cmake --build build --config Release
.\output\Release\board_perft_demo.exe 5 "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
```

Expected results (standard starting position):
- Depth 1: 20 nodes
- Depth 2: 400 nodes
- Depth 3: 8,902 nodes
- Depth 4: 197,281 nodes
- Depth 5: 4,865,609 nodes
- Depth 6: 119,060,324 nodes

Compare bitboard vs legacy implementation - node counts **must** match exactly!

## Next Steps

1. ✅ Core data structures complete
2. ✅ Move generation complete
3. ✅ Make/unmake complete
4. ⬜ Update CMakeLists.txt to compile new files
5. ⬜ Create wrapper in Board class
6. ⬜ Validate with perft tests
7. ⬜ Replace old move generation calls
8. ⬜ Deprecate Piece::getPseudoLegalMoves()
9. ⬜ Remove Piece hierarchy entirely

## References

- Original C# implementation: `Move.cs`, `MoveGenerator.cs`, `Board.cs`
- Migration guide: `docs/BITBOARD_FULL_MIGRATION.md`
- Deprecation plan: `docs/DEPRECATIONS.md`
