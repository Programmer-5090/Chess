#include <chess/board/bitboard/board_state.h>
#include <chess/board/bitboard/zoborist.h>
#include <chess/board/pieces/piece_const.h>
#include <sstream>
#include <cctype>

namespace chess {

void BitboardState::clear() {
    square.fill(PIECE_NONE);
    
    for (int i = 0; i < 2; ++i) {
        pawns[i].clear();
        knights[i].clear();
        bishops[i].clear();
        rooks[i].clear();
        queens[i].clear();
        kingSquare[i] = -1;
    }
    
    whiteToMove = true;
    gameState = 0;
    zobristKey = 0;
    repetitionHistory.clear();
    plyCount = 0;
    fiftyMoveCounter = 0;
}

int BitboardState::getPieceAt(int r, int c) const {
    if (r >= 0 && r < 8 && c >= 0 && c < 8) {
        int rank = 7 - r;
        int sq = rank * 8 + c;
        return typeOf(square[sq]);
    }
    return PIECE_NONE;
}

void BitboardState::loadFromFEN(const std::string& fen) {
    clear();
    
    std::istringstream ss(fen);
    std::string position, turn, castling, enPassant;
    int halfmove, fullmove;
    
    ss >> position >> turn >> castling >> enPassant >> halfmove >> fullmove;
    
    // FEN format: rank 8 (a8-h8) down to rank 1 (a1-h1)
    int rank = 7, file = 0;
    for (char c : position) {
        if (c == '/') {
            rank--;
            file = 0;
        } else if (std::isdigit(c)) {
            file += (c - '0');
        } else {
            int sq = rank * 8 + file;
            int pieceType = PIECE_NONE;
            int color = std::isupper(c) ? COLOR_WHITE : COLOR_BLACK;
            int colorIdx = (color == COLOR_WHITE) ? 0 : 1;
            
            char lower = std::tolower(c);
            switch (lower) {
                case 'p': pieceType = PIECE_PAWN; break;
                case 'n': pieceType = PIECE_KNIGHT; break;
                case 'b': pieceType = PIECE_BISHOP; break;
                case 'r': pieceType = PIECE_ROOK; break;
                case 'q': pieceType = PIECE_QUEEN; break;
                case 'k': pieceType = PIECE_KING; break;
            }
            
            int piece = pieceType | color;
            square[sq] = piece;
            
            switch (pieceType) {
                case PIECE_PAWN:   pawns[colorIdx].add(sq); break;
                case PIECE_KNIGHT: knights[colorIdx].add(sq); break;
                case PIECE_BISHOP: bishops[colorIdx].add(sq); break;
                case PIECE_ROOK:   rooks[colorIdx].add(sq); break;
                case PIECE_QUEEN:  queens[colorIdx].add(sq); break;
                case PIECE_KING:   kingSquare[colorIdx] = sq; break;
            }
            
            file++;
        }
    }
    
    whiteToMove = (turn == "w");
    
    gameState = 0;
    for (char c : castling) {
        switch (c) {
            case 'K': gameState |= CR_WHITE_K; break;
            case 'Q': gameState |= CR_WHITE_Q; break;
            case 'k': gameState |= CR_BLACK_K; break;
            case 'q': gameState |= CR_BLACK_Q; break;
        }
    }
    
    if (enPassant != "-" && enPassant.length() >= 2) {
        int epFile = enPassant[0] - 'a';
        setEPFile(gameState, epFile);
    }
    
    fiftyMoveCounter = halfmove;
    setFiftyMoveCounter(gameState, halfmove);
    
    zobristKey = Zobrist::calculateZobristKey(*this);
}

} // namespace chess
