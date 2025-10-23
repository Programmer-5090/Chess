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
        return square[sq];
    }
    return PIECE_NONE;
}

int BitboardState::getPieceTypeAt(int r, int c) const {
    return typeOf(getPieceAt(r, c));
}

int BitboardState::getPieceColorAt(int r, int c) const {
    int piece = getPieceAt(r, c);
    return colorOf(piece);
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

std::string BitboardState::toFEN() const {
    std::ostringstream ss;
    
    for (int rank = 7; rank >= 0; --rank) {
        int emptyCount = 0;
        for (int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            int piece = square[sq];
            if (piece == PIECE_NONE) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    ss << emptyCount;
                    emptyCount = 0;
                }
                int type = typeOf(piece);
                int color = colorOf(piece);
                char pieceChar = ' ';
                switch (type) {
                    case PIECE_PAWN:   pieceChar = 'p'; break;
                    case PIECE_KNIGHT: pieceChar = 'n'; break;
                    case PIECE_BISHOP: pieceChar = 'b'; break;
                    case PIECE_ROOK:   pieceChar = 'r'; break;
                    case PIECE_QUEEN:  pieceChar = 'q'; break;
                    case PIECE_KING:   pieceChar = 'k'; break;
                }
                if (color == COLOR_WHITE) pieceChar = std::toupper(pieceChar);
                ss << pieceChar;
            }
        }
        if (emptyCount > 0) {
            ss << emptyCount;
        }
        if (rank > 0) ss << '/';
    }
    
    ss << ' ' << (whiteToMove ? 'w' : 'b') << ' ';
    
    std::string castlingStr;
    if (gameState & CR_WHITE_K) castlingStr += 'K';
    if (gameState & CR_WHITE_Q) castlingStr += 'Q';
    if (gameState & CR_BLACK_K) castlingStr += 'k';
    if (gameState & CR_BLACK_Q) castlingStr += 'q';
    if (castlingStr.empty()) castlingStr = "-";
    ss << castlingStr << ' ';
    
    int epFile = getEPFile(gameState);
    if (epFile >= 0 && epFile < 8) {
        char fileChar = 'a' + epFile;
        int epRank = whiteToMove ? 5 : 2;
        ss << fileChar << epRank << ' ';
    } else {
        ss << "- ";
    }

    return ss.str();
}

} // namespace chess
