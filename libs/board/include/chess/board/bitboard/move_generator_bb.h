#ifndef MOVE_GENERATOR_BB_H
#define MOVE_GENERATOR_BB_H

#include <vector>
#include <cstdint>
#include <chess/board/bitboard/move.h>
#include <chess/board/bitboard/board_state.h>

// Forward declarations
class BoardBB;

namespace chess {

class MoveGeneratorBB {
public:
    MoveGeneratorBB();

    friend class BoardBB;
    
    std::vector<BBMove> generateMoves(BitboardState& state, bool capturesOnly = false);

    bool getInCheck() const { return inCheck; }
    
private:
    std::vector<BBMove> moves;
    bool isWhiteToMove;
    int friendlyColour;
    int opponentColour;
    int friendlyKingSquare;
    int friendlyColourIndex;
    int opponentColourIndex;
    
    bool inCheck;
    bool inDoubleCheck;
    bool pinsExistInPosition;
    uint64_t checkRayBitmask;
    uint64_t pinRayBitmask;
    uint64_t opponentKnightAttacks;
    uint64_t opponentAttackMapNoPawns;
    uint64_t opponentAttackMap;
    uint64_t opponentPawnAttackMap;
    uint64_t opponentSlidingAttackMap;
    
    bool genQuiets;
    BitboardState* board;
    
    void init();
    void calculateAttackData();
    void genSlidingAttackMap();
    void updateSlidingAttackPiece(int startSquare, int startDirIndex, int endDirIndex);
    
    void generateKingMoves();
    void generateSlidingMoves();
    void generateSlidingPieceMoves(int startSquare, int startDirIndex, int endDirIndex);
    void generateKnightMoves();
    void generatePawnMoves();
    void makePromotionMoves(int fromSquare, int toSquare);
    
    bool isMovingAlongRay(int rayDir, int startSquare, int targetSquare);
    bool isPinnedFunc(int square);
    bool squareIsInCheckRay(int square);
    bool hasKingsideCastleRight();
    bool hasQueensideCastleRight();
    bool squareIsAttacked(int square);
    bool inCheckAfterEnPassant(int startSquare, int targetSquare, int epCapturedPawnSquare);
    bool squareAttackedAfterEPCapture(int epCaptureSquare, int capturingPawnStartSquare);
};

} // namespace chess

#endif // MOVE_GENERATOR_BB_H

