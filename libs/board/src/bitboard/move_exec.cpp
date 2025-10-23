#include <chess/board/bitboard/move_exec.h>
#include <chess/board/bitboard/move.h>
#include <chess/board/bitboard/board_state.h>
#include <chess/board/bitboard/zoborist.h>
#include <chess/board/pieces/piece_const.h>

namespace chess {

UndoState BBMoveExecutor::makeMove(const BBMove& move) {    
    UndoState undo;
    undo.previousGameState = state.gameState;
    undo.previousZobrist = state.zobristKey;
    undo.previousFiftyMove = state.fiftyMoveCounter;
    undo.previousPlyCount = state.plyCount;
    
    int from = move.startSquare();
    int to = move.targetSquare();
    int movePiece = state.square[from];
    int movePieceType = typeOf(movePiece);
    int colorIdx = isColor(movePiece, COLOR_WHITE) ? 0 : 1;
    int opponentIdx = 1 - colorIdx;
    
    int capturedPiece = state.square[to];
    undo.capturedPiece = typeOf(capturedPiece);
    
    if (capturedPiece != PIECE_NONE && move.flag() != BBMove::EnPassantCapture) {
        switch (undo.capturedPiece) {
            case PIECE_PAWN:   state.pawns[opponentIdx].remove(to); break;
            case PIECE_KNIGHT: state.knights[opponentIdx].remove(to); break;
            case PIECE_BISHOP: state.bishops[opponentIdx].remove(to); break;
            case PIECE_ROOK:   state.rooks[opponentIdx].remove(to); break;
            case PIECE_QUEEN:  state.queens[opponentIdx].remove(to); break;
        }
        state.zobristKey ^= Zobrist::piece(undo.capturedPiece, opponentIdx, to);
    }
    
    int oldEP = getEPFile(state.gameState);
    if (oldEP >= 0) {
        state.zobristKey ^= Zobrist::enPassantFile(oldEP);
    }
    
    state.zobristKey ^= Zobrist::piece(movePieceType, colorIdx, from);
    if (movePieceType == PIECE_KING) {
        state.kingSquare[colorIdx] = to;
    } else {
        switch (movePieceType) {
            case PIECE_PAWN:   state.pawns[colorIdx].move(from, to); break;
            case PIECE_KNIGHT: state.knights[colorIdx].move(from, to); break;
            case PIECE_BISHOP: state.bishops[colorIdx].move(from, to); break;
            case PIECE_ROOK:   state.rooks[colorIdx].move(from, to); break;
            case PIECE_QUEEN:  state.queens[colorIdx].move(from, to); break;
        }
    }
    
    int pieceOnTarget = movePiece;
    
    if (move.isPromotion()) {
        state.pawns[colorIdx].remove(to);
        int promoteType = PIECE_QUEEN;
        switch (move.flag()) {
            case BBMove::PromoteToQueen:  promoteType = PIECE_QUEEN; break;
            case BBMove::PromoteToRook:   promoteType = PIECE_ROOK; break;
            case BBMove::PromoteToBishop: promoteType = PIECE_BISHOP; break;
            case BBMove::PromoteToKnight: promoteType = PIECE_KNIGHT; break;
            default: break;
        }
        pieceOnTarget = promoteType | (colorIdx == 0 ? COLOR_WHITE : COLOR_BLACK);
        
        switch (promoteType) {
            case PIECE_QUEEN:  state.queens[colorIdx].add(to); break;
            case PIECE_ROOK:   state.rooks[colorIdx].add(to); break;
            case PIECE_BISHOP: state.bishops[colorIdx].add(to); break;
            case PIECE_KNIGHT: state.knights[colorIdx].add(to); break;
        }
        state.zobristKey ^= Zobrist::piece(PIECE_PAWN, colorIdx, from);
        state.zobristKey ^= Zobrist::piece(promoteType, colorIdx, to);
    } else if (move.flag() == BBMove::Castling) {
        int rookFrom, rookTo;
        if (to > from) {
            rookFrom = colorIdx == 0 ? 7 : 63;
            rookTo = colorIdx == 0 ? 5 : 61;
        } else {
            rookFrom = colorIdx == 0 ? 0 : 56;
            rookTo = colorIdx == 0 ? 3 : 59;
        }
        int rook = state.square[rookFrom];
        state.square[rookTo] = rook;
        state.square[rookFrom] = PIECE_NONE;
        state.rooks[colorIdx].move(rookFrom, rookTo);
        state.zobristKey ^= Zobrist::piece(PIECE_ROOK, colorIdx, rookFrom);
        state.zobristKey ^= Zobrist::piece(PIECE_ROOK, colorIdx, rookTo);
    } else if (move.flag() == BBMove::EnPassantCapture) {
        int capturedSq = colorIdx == 0 ? to - 8 : to + 8;
        int capturedPawn = state.square[capturedSq];
        undo.capturedPiece = PIECE_PAWN;
        state.square[capturedSq] = PIECE_NONE;
        state.pawns[opponentIdx].remove(capturedSq);
        state.zobristKey ^= Zobrist::piece(PIECE_PAWN, opponentIdx, capturedSq);
    }
    
    state.square[to] = pieceOnTarget;
    state.square[from] = PIECE_NONE;
    state.zobristKey ^= Zobrist::piece(typeOf(pieceOnTarget), colorIdx, to);
    
    setEPFile(state.gameState, -1);
    if (move.flag() == BBMove::PawnTwoForward) {
        int epFile = toCol(from);
        setEPFile(state.gameState, epFile);
        state.zobristKey ^= Zobrist::enPassantFile(epFile);
    }
    
    uint32_t oldCastleRights = state.gameState & 15;
    if (movePieceType == PIECE_KING) {
        state.gameState &= (colorIdx == 0 ? WHITE_CASTLE_MASK : BLACK_CASTLE_MASK);
    }
    if (from == 0 || to == 0) state.gameState &= ~CR_WHITE_Q;
    if (from == 7 || to == 7) state.gameState &= ~CR_WHITE_K;
    if (from == 56 || to == 56) state.gameState &= ~CR_BLACK_Q;
    if (from == 63 || to == 63) state.gameState &= ~CR_BLACK_K;
    
    uint32_t newCastleRights = state.gameState & 15;
    if (oldCastleRights != newCastleRights) {
        state.zobristKey ^= Zobrist::castlingRights(oldCastleRights);
        state.zobristKey ^= Zobrist::castlingRights(newCastleRights);
    }
    
    state.whiteToMove = !state.whiteToMove;
    state.zobristKey ^= Zobrist::sideToMove();
    
    state.plyCount++;
    if (movePieceType == PIECE_PAWN || capturedPiece != PIECE_NONE) {
        state.fiftyMoveCounter = 0;
        state.repetitionHistory.clear();
    } else {
        state.fiftyMoveCounter++;
    }
    
    if (!state.repetitionHistory.empty() && state.plyCount > 1) {
        state.repetitionHistory.push_back(state.zobristKey);
        state.zobristHistory.push_back(state.zobristKey);
    }
    
    return undo;
}

void BBMoveExecutor::unmakeMove(const BBMove& move, const UndoState& undo) {
    state.whiteToMove = !state.whiteToMove;
    
    int from = move.startSquare();
    int to = move.targetSquare();
    int movedPiece = state.square[to];
    int movedPieceType = typeOf(movedPiece);
    int colorIdx = isColor(movedPiece, COLOR_WHITE) ? 0 : 1;
    int opponentIdx = 1 - colorIdx;
    
    if (move.isPromotion()) {
        int promoteType = movedPieceType;
        switch (promoteType) {
            case PIECE_QUEEN:  state.queens[colorIdx].remove(to); break;
            case PIECE_ROOK:   state.rooks[colorIdx].remove(to); break;
            case PIECE_BISHOP: state.bishops[colorIdx].remove(to); break;
            case PIECE_KNIGHT: state.knights[colorIdx].remove(to); break;
        }
        state.pawns[colorIdx].add(from);
        movedPieceType = PIECE_PAWN;
        movedPiece = PIECE_PAWN | (colorIdx == 0 ? COLOR_WHITE : COLOR_BLACK);
    } else if (move.flag() == BBMove::Castling) {
        int rookFrom, rookTo;
        if (to > from) {
            rookFrom = colorIdx == 0 ? 7 : 63;
            rookTo = colorIdx == 0 ? 5 : 61;
        } else {
            rookFrom = colorIdx == 0 ? 0 : 56;
            rookTo = colorIdx == 0 ? 3 : 59;
        }
        int rook = state.square[rookTo];
        state.square[rookFrom] = rook;
        state.square[rookTo] = PIECE_NONE;
        state.rooks[colorIdx].move(rookTo, rookFrom);
    } else if (move.flag() == BBMove::EnPassantCapture) {
        int capturedSq = colorIdx == 0 ? to - 8 : to + 8;
        int capturedPawn = PIECE_PAWN | (opponentIdx == 0 ? COLOR_WHITE : COLOR_BLACK);
        state.square[capturedSq] = capturedPawn;
        state.pawns[opponentIdx].add(capturedSq);
    }
    
    state.square[from] = movedPiece;
    state.square[to] = PIECE_NONE;
    
    if (movedPieceType == PIECE_KING) {
        state.kingSquare[colorIdx] = from;
    } else if (!move.isPromotion()) {
        switch (movedPieceType) {
            case PIECE_PAWN:   state.pawns[colorIdx].move(to, from); break;
            case PIECE_KNIGHT: state.knights[colorIdx].move(to, from); break;
            case PIECE_BISHOP: state.bishops[colorIdx].move(to, from); break;
            case PIECE_ROOK:   state.rooks[colorIdx].move(to, from); break;
            case PIECE_QUEEN:  state.queens[colorIdx].move(to, from); break;
        }
    }
    
    if (undo.capturedPiece != PIECE_NONE && move.flag() != BBMove::EnPassantCapture) {
        int capturedPiece = undo.capturedPiece | (opponentIdx == 0 ? COLOR_WHITE : COLOR_BLACK);
        state.square[to] = capturedPiece;
        
        switch (undo.capturedPiece) {
            case PIECE_PAWN:   state.pawns[opponentIdx].add(to); break;
            case PIECE_KNIGHT: state.knights[opponentIdx].add(to); break;
            case PIECE_BISHOP: state.bishops[opponentIdx].add(to); break;
            case PIECE_ROOK:   state.rooks[opponentIdx].add(to); break;
            case PIECE_QUEEN:  state.queens[opponentIdx].add(to); break;
        }
    }
    
    state.gameState = undo.previousGameState;
    state.zobristKey = undo.previousZobrist;
    state.fiftyMoveCounter = undo.previousFiftyMove;
    state.plyCount = undo.previousPlyCount;
    
    if (!state.repetitionHistory.empty()) {
        state.repetitionHistory.pop_back();
    }
    if (!state.zobristHistory.empty()) {
        state.zobristHistory.pop_back();
    }
}

} // namespace chess
