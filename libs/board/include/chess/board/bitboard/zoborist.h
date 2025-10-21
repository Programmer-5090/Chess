#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <cstdint>
#include <array>
#include <random>

namespace chess {
class Zobrist {
public:
    static void init();
    
    // Get zobrist value for piece on square
    // pieceType: PIECE_PAWN, PIECE_KNIGHT, etc. (1-6)
    // colorIndex: 0 for white, 1 for black
    // square: 0-63
    static uint64_t piece(int pieceType, int colorIndex, int square) {
        return piecesArray[pieceType][colorIndex][square];
    }
    
    // Get zobrist value for castling rights (0-15)
    static uint64_t castlingRights(int rights) {
        return castlingRightsArray[rights];
    }
    
    // Get zobrist value for en passant file (0-7)
    static uint64_t enPassantFile(int file) {
        return enPassantFileArray[file];
    }
    
    // Get zobrist value for side to move
    static uint64_t sideToMove() {
        return sideToMoveValue;
    }
    
    // Calculate full zobrist key from scratch
    static uint64_t calculateZobristKey(const struct BitboardState& state);

private:
    // [pieceType 1-6][colorIndex 0-1][square 0-63]
    static uint64_t piecesArray[8][2][64];
    
    // [castlingRights 0-15]
    static uint64_t castlingRightsArray[16];
    
    // [file 0-7]
    static uint64_t enPassantFileArray[9];
    
    // Side to move toggle
    static uint64_t sideToMoveValue;
    
    static bool initialized;
    
    // Generate random 64-bit number
    static uint64_t randomUInt64();
    static std::mt19937_64& getRNG();
};

} // namespace chess

#endif // ZOBRIST_H