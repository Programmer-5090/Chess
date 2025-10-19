#include <chess/board/bitboard/zoborist.h>
#include <chess/board/bitboard/board_state.h>
#include <chess/board/pieces/piece_const.h>

namespace chess {

// Static member definitions
uint64_t Zobrist::piecesArray[8][2][64] = {};
uint64_t Zobrist::castlingRightsArray[16] = {};
uint64_t Zobrist::enPassantFileArray[9] = {};
uint64_t Zobrist::sideToMoveValue = 0;
bool Zobrist::initialized = false;

std::mt19937_64& Zobrist::getRNG() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    return gen;
}

uint64_t Zobrist::randomUInt64() {
    std::uniform_int_distribution<uint64_t> dist;
    return dist(getRNG());
}

void Zobrist::init() {
    if (initialized) return;
    
    // Initialize piece square values
    for (int pieceType = 0; pieceType < 8; ++pieceType) {
        for (int color = 0; color < 2; ++color) {
            for (int square = 0; square < 64; ++square) {
                piecesArray[pieceType][color][square] = randomUInt64();
            }
        }
    }
    
    // Initialize castling rights
    for (int i = 0; i < 16; ++i) {
        castlingRightsArray[i] = randomUInt64();
    }
    
    // Initialize en passant files
    for (int i = 0; i < 9; ++i) {
        enPassantFileArray[i] = randomUInt64();
    }
    
    // Initialize side to move
    sideToMoveValue = randomUInt64();
    
    initialized = true;
}

uint64_t Zobrist::calculateZobristKey(const BitboardState& state) {
    uint64_t key = 0;
    
    // Hash all pieces on board
    for (int sq = 0; sq < 64; ++sq) {
        int pieceValue = state.square[sq];
        if (pieceValue != PIECE_NONE) {
            int pieceType = typeOf(pieceValue);
            int colorIdx = isColor(pieceValue, COLOR_WHITE) ? 0 : 1;
            key ^= piecesArray[pieceType][colorIdx][sq];
        }
    }
    
    // Hash castling rights
    uint32_t castleRights = state.gameState & 15;
    key ^= castlingRightsArray[castleRights];
    
    // Hash en passant
    int epFile = getEPFile(state.gameState);
    if (epFile >= 0) {
        key ^= enPassantFileArray[epFile];
    }
    
    // Hash side to move
    if (!state.whiteToMove) {
        key ^= sideToMoveValue;
    }
    
    return key;
}

} // namespace chess
