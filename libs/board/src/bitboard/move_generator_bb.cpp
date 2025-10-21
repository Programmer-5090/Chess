#include <chess/board/bitboard/move_generator_bb.h>
#include <chess/board/bitboard/precomputed_data.h>
#include <chess/board/bitboard/bitboard.h>
#include <chess/board/pieces/piece_const.h>

namespace chess {

MoveGeneratorBB::MoveGeneratorBB() {
    PrecomputedData::init();
}

std::vector<BBMove> MoveGeneratorBB::generateMoves(BitboardState& state, bool capturesOnly) {
    this->board = &state;
    this->genQuiets = !capturesOnly;
    this->moves.clear();
    this->moves.reserve(capturesOnly ? 32 : 218);
    
    init();
    calculateAttackData();
    generateKingMoves();
    
    if (inDoubleCheck) {
        return moves;
    }
    
    generateSlidingMoves();
    generateKnightMoves();
    generatePawnMoves();
    
    return moves;
}

void MoveGeneratorBB::init() {
    inCheck = false;
    inDoubleCheck = false;
    pinsExistInPosition = false;
    checkRayBitmask = 0;
    pinRayBitmask = 0;
    
    isWhiteToMove = board->whiteToMove;
    friendlyColour = board->whiteToMove ? COLOR_WHITE : COLOR_BLACK;
    opponentColour = board->whiteToMove ? COLOR_BLACK : COLOR_WHITE;
    friendlyKingSquare = board->kingSquare[board->whiteToMove ? 0 : 1];
    friendlyColourIndex = board->whiteToMove ? 0 : 1;
    opponentColourIndex = 1 - friendlyColourIndex;
}

void MoveGeneratorBB::genSlidingAttackMap() {
    opponentSlidingAttackMap = 0;
    
    for (int i = 0; i < board->rooks[opponentColourIndex].count(); i++) {
        updateSlidingAttackPiece(board->rooks[opponentColourIndex].squares[i], 0, 4);
    }
    
    for (int i = 0; i < board->queens[opponentColourIndex].count(); i++) {
        updateSlidingAttackPiece(board->queens[opponentColourIndex].squares[i], 0, 8);
    }
    
    for (int i = 0; i < board->bishops[opponentColourIndex].count(); i++) {
        updateSlidingAttackPiece(board->bishops[opponentColourIndex].squares[i], 4, 8);
    }
}

void MoveGeneratorBB::updateSlidingAttackPiece(int startSquare, int startDirIndex, int endDirIndex) {
    for (int directionIndex = startDirIndex; directionIndex < endDirIndex; directionIndex++) {
        int currentDirOffset = PrecomputedData::directionOffsets[directionIndex];
        for (int n = 0; n < PrecomputedData::numSquaresToEdge[startSquare][directionIndex]; n++) {
            int targetSquare = startSquare + currentDirOffset * (n + 1);
            int targetSquarePiece = board->square[targetSquare];
            opponentSlidingAttackMap |= (1ULL << targetSquare);
            if (targetSquare != friendlyKingSquare) {
                if (targetSquarePiece != PIECE_NONE) {
                    break;
                }
            }
        }
    }
}

void MoveGeneratorBB::calculateAttackData() {
    genSlidingAttackMap();
    
    int startDirIndex = 0;
    int endDirIndex = 8;
    
    if (board->queens[opponentColourIndex].count() == 0) {
        startDirIndex = (board->rooks[opponentColourIndex].count() > 0) ? 0 : 4;
        endDirIndex = (board->bishops[opponentColourIndex].count() > 0) ? 8 : 4;
    }
    
    for (int dir = startDirIndex; dir < endDirIndex; dir++) {
        bool isDiagonal = dir > 3;
        
        int n = PrecomputedData::numSquaresToEdge[friendlyKingSquare][dir];
        int directionOffset = PrecomputedData::directionOffsets[dir];
        bool isFriendlyPieceAlongRay = false;
        uint64_t rayMask = 0;
        
        for (int i = 0; i < n; i++) {
            int squareIndex = friendlyKingSquare + directionOffset * (i + 1);
            rayMask |= (1ULL << squareIndex);
            int piece = board->square[squareIndex];
            
            // This square contains a piece
            if (piece != PIECE_NONE) {
                if (isColor(piece, friendlyColour)) {
                    if (!isFriendlyPieceAlongRay) {
                        isFriendlyPieceAlongRay = true;
                    }
                    else {
                        break;
                    }
                }
                else {
                    int pieceType = typeOf(piece);
                    
                    if ((isDiagonal && (pieceType == PIECE_BISHOP || pieceType == PIECE_QUEEN)) ||
                        (!isDiagonal && (pieceType == PIECE_ROOK || pieceType == PIECE_QUEEN))) {
                        
                        if (isFriendlyPieceAlongRay) {
                            pinsExistInPosition = true;
                            pinRayBitmask |= rayMask;
                        }
                        else {
                            checkRayBitmask |= rayMask;
                            inDoubleCheck = inCheck;
                            inCheck = true;
                        }
                        break;
                    } else {
                        break;
                    }
                }
            }
        }
        
        if (inDoubleCheck) {
            break;
        }
    }
    
    opponentKnightAttacks = 0;
    bool isKnightCheck = false;
    
    for (int i = 0; i < board->knights[opponentColourIndex].count(); i++) {
        int startSquare = board->knights[opponentColourIndex].squares[i];
        opponentKnightAttacks |= PrecomputedData::knightAttackBitboards[startSquare];
        
        if (!isKnightCheck && getBit(opponentKnightAttacks, friendlyKingSquare)) {
            isKnightCheck = true;
            inDoubleCheck = inCheck;
            inCheck = true;
            checkRayBitmask |= (1ULL << startSquare);
        }
    }
    
    opponentPawnAttackMap = 0;
    bool isPawnCheck = false;
    
    for (int i = 0; i < board->pawns[opponentColourIndex].count(); i++) {
        int pawnSquare = board->pawns[opponentColourIndex].squares[i];
        uint64_t pawnAttacks = PrecomputedData::pawnAttackBitboards[pawnSquare][opponentColourIndex];
        opponentPawnAttackMap |= pawnAttacks;
        
        if (!isPawnCheck && getBit(pawnAttacks, friendlyKingSquare)) {
            isPawnCheck = true;
            inDoubleCheck = inCheck;
            inCheck = true;
            checkRayBitmask |= (1ULL << pawnSquare);
        }
    }
    
    int enemyKingSquare = board->kingSquare[opponentColourIndex];
    
    opponentAttackMapNoPawns = opponentSlidingAttackMap | opponentKnightAttacks | 
                               PrecomputedData::kingAttackBitboards[enemyKingSquare];
    opponentAttackMap = opponentAttackMapNoPawns | opponentPawnAttackMap;
}


void MoveGeneratorBB::generateKingMoves() {
    const int* kingMoves = PrecomputedData::kingMoves[friendlyKingSquare];
    int numMoves = PrecomputedData::numKingMoves[friendlyKingSquare];
    
    for (int i = 0; i < numMoves; i++) {
        int targetSquare = kingMoves[i];
        int pieceOnTargetSquare = board->square[targetSquare];
        
        if (isColor(pieceOnTargetSquare, friendlyColour)) {
            continue;
        }
        
        bool isCapture = isColor(pieceOnTargetSquare, opponentColour);
        if (!isCapture) {
            if (!genQuiets || squareIsInCheckRay(targetSquare)) {
                continue;
            }
        }
        
        if (!squareIsAttacked(targetSquare)) {
            moves.emplace_back(friendlyKingSquare, targetSquare);
            
            if (!inCheck && !isCapture) {
                int f_square = isWhiteToMove ? 5 : 61;
                
                if (targetSquare == f_square && hasKingsideCastleRight()) {
                    int castleKingsideSquare = targetSquare + 1;
                    if (board->square[castleKingsideSquare] == PIECE_NONE) {
                        if (!squareIsAttacked(castleKingsideSquare)) {
                            moves.emplace_back(friendlyKingSquare, castleKingsideSquare, BBMove::Castling);
                        }
                    }
                }
                
                int d_square = isWhiteToMove ? 3 : 59;

                if (targetSquare == d_square && hasQueensideCastleRight()) {
                    int castleQueensideSquare = targetSquare - 1;
                    if (board->square[castleQueensideSquare] == PIECE_NONE && 
                        board->square[castleQueensideSquare - 1] == PIECE_NONE) {
                        if (!squareIsAttacked(castleQueensideSquare)) {
                            moves.emplace_back(friendlyKingSquare, castleQueensideSquare, BBMove::Castling);
                        }
                    }
                }
            }
        }
    }
}

void MoveGeneratorBB::generateSlidingMoves() {
    for (int i = 0; i < board->rooks[friendlyColourIndex].count(); i++) {
        generateSlidingPieceMoves(board->rooks[friendlyColourIndex].squares[i], 0, 4);
    }
    
    for (int i = 0; i < board->bishops[friendlyColourIndex].count(); i++) {
        generateSlidingPieceMoves(board->bishops[friendlyColourIndex].squares[i], 4, 8);
    }
    
    for (int i = 0; i < board->queens[friendlyColourIndex].count(); i++) {
        generateSlidingPieceMoves(board->queens[friendlyColourIndex].squares[i], 0, 8);
    }
}

void MoveGeneratorBB::generateSlidingPieceMoves(int startSquare, int startDirIndex, int endDirIndex) {
    bool isPinned = isPinnedFunc(startSquare);
    
    if (inCheck && isPinned) {
        return;
    }
    
    for (int directionIndex = startDirIndex; directionIndex < endDirIndex; directionIndex++) {
        int currentDirOffset = PrecomputedData::directionOffsets[directionIndex];
        
        if (isPinned && !isMovingAlongRay(currentDirOffset, friendlyKingSquare, startSquare)) {
            continue;
        }
        
        for (int n = 0; n < PrecomputedData::numSquaresToEdge[startSquare][directionIndex]; n++) {
            int targetSquare = startSquare + currentDirOffset * (n + 1);
            int targetSquarePiece = board->square[targetSquare];
            
            if (isColor(targetSquarePiece, friendlyColour)) {
                break;
            }
            bool isCapture = targetSquarePiece != PIECE_NONE;
            
            bool movePreventsCheck = squareIsInCheckRay(targetSquare);
            if (movePreventsCheck || !inCheck) {
                if (genQuiets || isCapture) {
                    moves.emplace_back(startSquare, targetSquare);
                }
            }
            // If square not empty or blocks check, can't continue
            if (isCapture || movePreventsCheck) {
                break;
            }
        }
    }
}

void MoveGeneratorBB::generateKnightMoves() {
    for (int i = 0; i < board->knights[friendlyColourIndex].count(); i++) {
        int startSquare = board->knights[friendlyColourIndex].squares[i];
        
        if (isPinnedFunc(startSquare)) {
            continue;
        }
        
        const int* knightMoves = PrecomputedData::knightMoves[startSquare];
        int numMoves = PrecomputedData::numKnightMoves[startSquare];
        
        for (int j = 0; j < numMoves; j++) {
            int targetSquare = knightMoves[j];
            int targetSquarePiece = board->square[targetSquare];
            bool isCapture = isColor(targetSquarePiece, opponentColour);
            if (genQuiets || isCapture) {
                if (isColor(targetSquarePiece, friendlyColour) || 
                    (inCheck && !squareIsInCheckRay(targetSquare))) {
                    continue;
                }
                moves.emplace_back(startSquare, targetSquare);
            }
        }
    }
}

void MoveGeneratorBB::generatePawnMoves() {
    int pawnOffset = (friendlyColour == COLOR_WHITE) ? 8 : -8;
    int startRank = isWhiteToMove ? 1 : 6;
    int finalRankBeforePromotion = isWhiteToMove ? 6 : 1;
    
    int enPassantFile = ((board->gameState >> 4) & 15) - 1;
    int enPassantSquare = -1;
    if (enPassantFile != -1) {
        enPassantSquare = 8 * (isWhiteToMove ? 5 : 2) + enPassantFile;
    }
    
    for (int i = 0; i < board->pawns[friendlyColourIndex].count(); i++) {
        int startSquare = board->pawns[friendlyColourIndex].squares[i];
        int rank = startSquare / 8;
        bool oneStepFromPromotion = rank == finalRankBeforePromotion;
        
        if (genQuiets) {
            int squareOneForward = startSquare + pawnOffset;
            
            if (board->square[squareOneForward] == PIECE_NONE) {
                if (!isPinnedFunc(startSquare) || isMovingAlongRay(pawnOffset, startSquare, friendlyKingSquare)) {
                    if (!inCheck || squareIsInCheckRay(squareOneForward)) {
                        if (oneStepFromPromotion) {
                            makePromotionMoves(startSquare, squareOneForward);
                        } else {
                            moves.emplace_back(startSquare, squareOneForward);
                        }
                    }
                    
                    if (rank == startRank) {
                        int squareTwoForward = squareOneForward + pawnOffset;
                        if (board->square[squareTwoForward] == PIECE_NONE) {
                            if (!inCheck || squareIsInCheckRay(squareTwoForward)) {
                                moves.emplace_back(startSquare, squareTwoForward, BBMove::PawnTwoForward);
                            }
                        }
                    }
                }
            }
        }
        
        for (int j = 0; j < 2; j++) {
            int* pawnAttackDirs = PrecomputedData::pawnAttackDirections[friendlyColourIndex];
            if (PrecomputedData::numSquaresToEdge[startSquare][pawnAttackDirs[j]] > 0) {
                int pawnCaptureDir = PrecomputedData::directionOffsets[pawnAttackDirs[j]];
                int targetSquare = startSquare + pawnCaptureDir;
                int targetPiece = board->square[targetSquare];
                
                if (isPinnedFunc(startSquare) && !isMovingAlongRay(pawnCaptureDir, friendlyKingSquare, startSquare)) {
                    continue;
                }
                
                if (isColor(targetPiece, opponentColour)) {
                    if (inCheck && !squareIsInCheckRay(targetSquare)) {
                        continue;
                    }
                    if (oneStepFromPromotion) {
                        makePromotionMoves(startSquare, targetSquare);
                    } else {
                        moves.emplace_back(startSquare, targetSquare);
                    }
                }
                
                // En passant requires special check for revealed check
                if (targetSquare == enPassantSquare) {
                    int epCapturedPawnSquare = targetSquare + (isWhiteToMove ? -8 : 8);
                    if (!inCheckAfterEnPassant(startSquare, targetSquare, epCapturedPawnSquare)) {
                        moves.emplace_back(startSquare, targetSquare, BBMove::EnPassantCapture);
                    }
                }
            }
        }
    }
}

void MoveGeneratorBB::makePromotionMoves(int fromSquare, int toSquare) {
    moves.emplace_back(fromSquare, toSquare, BBMove::PromoteToQueen);
    moves.emplace_back(fromSquare, toSquare, BBMove::PromoteToKnight);
    moves.emplace_back(fromSquare, toSquare, BBMove::PromoteToRook);
    moves.emplace_back(fromSquare, toSquare, BBMove::PromoteToBishop);
}

bool MoveGeneratorBB::isMovingAlongRay(int rayDir, int startSquare, int targetSquare) {
    int moveDir = PrecomputedData::directionLookup[targetSquare - startSquare + 63];
    return (rayDir == moveDir || -rayDir == moveDir);
}

bool MoveGeneratorBB::isPinnedFunc(int square) {
    return pinsExistInPosition && ((pinRayBitmask >> square) & 1) != 0;
}

bool MoveGeneratorBB::squareIsInCheckRay(int square) {
    return inCheck && ((checkRayBitmask >> square) & 1) != 0;
}

bool MoveGeneratorBB::hasKingsideCastleRight() {
    int mask = isWhiteToMove ? 1 : 4;
    return (board->gameState & mask) != 0;
}

bool MoveGeneratorBB::hasQueensideCastleRight() {
    int mask = isWhiteToMove ? 2 : 8;
    return (board->gameState & mask) != 0;
}

bool MoveGeneratorBB::squareIsAttacked(int square) {
    return getBit(opponentAttackMap, square);
}

bool MoveGeneratorBB::inCheckAfterEnPassant(int startSquare, int targetSquare, int epCapturedPawnSquare) {
    board->square[targetSquare] = board->square[startSquare];
    board->square[startSquare] = PIECE_NONE;
    board->square[epCapturedPawnSquare] = PIECE_NONE;
    
    bool inCheckAfterEpCapture = false;
    if (squareAttackedAfterEPCapture(epCapturedPawnSquare, startSquare)) {
        inCheckAfterEpCapture = true;
    }
    
    board->square[targetSquare] = PIECE_NONE;
    board->square[startSquare] = PIECE_PAWN | friendlyColour;
    board->square[epCapturedPawnSquare] = PIECE_PAWN | opponentColour;
    return inCheckAfterEpCapture;
}

bool MoveGeneratorBB::squareAttackedAfterEPCapture(int epCaptureSquare, int capturingPawnStartSquare) {
    if (getBit(opponentAttackMapNoPawns, friendlyKingSquare)) {
        return true;
    }
    
    int dirIndex = (epCaptureSquare < friendlyKingSquare) ? 2 : 3;
    for (int i = 0; i < PrecomputedData::numSquaresToEdge[friendlyKingSquare][dirIndex]; i++) {
        int squareIndex = friendlyKingSquare + PrecomputedData::directionOffsets[dirIndex] * (i + 1);
        int piece = board->square[squareIndex];
        if (piece != PIECE_NONE) {
            if (isColor(piece, friendlyColour)) {
                break;
            }
            else {
                int pieceType = typeOf(piece);
                if (pieceType == PIECE_ROOK || pieceType == PIECE_QUEEN) {
                    return true;
                } else {
                    break;
                }
            }
        }
    }
    
    for (int i = 0; i < 2; i++) {
        int* pawnAttackDirs = PrecomputedData::pawnAttackDirections[friendlyColourIndex];
        if (PrecomputedData::numSquaresToEdge[friendlyKingSquare][pawnAttackDirs[i]] > 0) {
            int piece = board->square[friendlyKingSquare + PrecomputedData::directionOffsets[pawnAttackDirs[i]]];
            if (piece == (PIECE_PAWN | opponentColour)) {
                return true;
            }
        }
    }
    
    return false;
}

} // namespace chess
