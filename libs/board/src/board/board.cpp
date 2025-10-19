#include <chess/board/board.h>
#include <chess/board/fen_util.h>
#include <chess/board/move_executor.h>
#include <chess/utils/logger.h>
#include <chess/board/piece_manager.h>
#include <chess/rendering/board_renderer.h>
#include <chess/board/pieces/pieces.h>
#include <chess/ui/controls/promotion_dialog.h>
#include <chess/utilities.h>
#include <chess/rendering/texture_cache.h>
#include <chess/utils/profiler.h>
#include <chess/board/bitboard/bitboard_init.h>
#include <chess/board/bitboard/board_state.h>
#include <chess/board/bitboard/move_generator_bb.h>
#include <chess/board/bitboard/move.h>
#include <chess/board/pieces/piece_const.h>

#include <unordered_map>
#include <cctype>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <cctype>
#include <chrono>
#include <algorithm>
#include <sstream>

using namespace std::chrono;

MakeUnmakeProfile g_muProfile;

Board::Board(int width, int height, float offSet) {
    screenHeight = height;
    screenWidth = width;
    this->offSet = offSet;
    startXPos = this->offSet;
    startYPos = this->offSet;
    endXPos = screenWidth - this->offSet;
    endYPos = screenHeight - this->offSet;
    this->squareSide = (static_cast<float>(screenWidth) - 2.0f * this->offSet) / 8.0f;
    pieceManager = std::make_unique<PieceManager>();
    moveExecutor = std::make_unique<MoveExecutor>(this);
    
    static bool bitboardInitialized = false;
    if (!bitboardInitialized) {
        chess::initBitboardSystem();
        bitboardInitialized = true;
    }
    bbState = std::make_unique<chess::BitboardState>();
    bbGenerator = std::make_unique<chess::MoveGeneratorBB>();
}

Board::~Board() {
}

void Board::loadFEN(const std::string& fen, SDL_Renderer* gameRenderer){
    FENUtil util;
    util.loadFEN(fen, *this, gameRenderer);
}

void Board::initializeBoard(SDL_Renderer* gameRenderer) {
    for (auto& row : pieceGrid) {
        row.fill(nullptr);
    }
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            boardGrid[i][j].x = startXPos + (j * squareSide);
            boardGrid[i][j].y = startYPos + (i * squareSide);
            boardGrid[i][j].w = squareSide;
            boardGrid[i][j].h = squareSide;
        }
    }
    
    if (gameRenderer) {
        TextureCache::setRenderer(gameRenderer);
        boardRenderer = std::make_unique<BoardRenderer>(gameRenderer);
        boardRenderer->initializeLayout(boardGrid, squareSide, isFlipped);
    } else {
        boardRenderer.reset();
    }
    
    loadFEN(this->startFEN, gameRenderer);
}

void Board::clearPieceGridAndPieces(){
    pieceManager->clear();
    for (auto& row : pieceGrid) {
        row.fill(nullptr);
    }
    whiteCapturedPieces.clear();
    blackCapturedPieces.clear();
}

void Board::setFlipped(bool flipped) {
    isFlipped = flipped;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            int displayRow = isFlipped ? (7 - i) : i;
            float x = startXPos + j * squareSide;
            float y = startYPos + displayRow * squareSide;
            boardGrid[i][j] = {x, y, squareSide, squareSide};
        }
    }
    
    if (boardRenderer) {
        boardRenderer->initializeLayout(boardGrid, squareSide, isFlipped);
    }
}

void Board::resetBoard(SDL_Renderer* gameRenderer) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            pieceGrid[i][j] = nullptr;
        }
    }
    
    whiteCapturedPieces.clear();
    blackCapturedPieces.clear();
    
    loadFEN(this->startFEN, gameRenderer);
}

void Board::updatePieceGrid() {
}

UndoMove Board::executeMove(const Move& move, bool trackUndo) {
    g_profiler.startTimer("Board::executeMove_wrapper");
    UndoMove u = moveExecutor->executeMove(move, trackUndo);
    g_profiler.endTimer("Board::executeMove_wrapper");
    return u;
}

void Board::undoMove(const Move& move, UndoMove& undo) {
    g_profiler.startTimer("Board::undoMove_wrapper");
    moveExecutor->undoMove(move, undo);
    g_profiler.endTimer("Board::undoMove_wrapper");
}

void Board::draw(SDL_Renderer* renderer, const std::pair<int, int>* selectedSquare, const std::vector<Move>* possibleMoves) {
    if (!boardRenderer) return;

    RenderContext context;
    context.selectedSquare = selectedSquare;
    context.possibleMoves = possibleMoves;
    context.showCoordinates = true;
    context.highlightLastMove = true;
    context.lastMove = getLastMovePtr();

    const auto& pieces = pieceManager->getAllPieces();

    boardRenderer->draw(pieces, context, this);
}

Piece* Board::getPieceAt(int r, int c) const {
    if (r >= 0 && r < 8 && c >= 0 && c < 8) {
        return pieceGrid[r][c];
    }
    return nullptr;
}

PieceManager* Board::getPieceManager() const {
    return pieceManager.get();
}

bool Board::screenToBoardCoords(int screenX, int screenY, int& boardR, int& boardC) const {
    if (screenX < startXPos || screenX > endXPos || screenY < startYPos || screenY > endYPos) {
        return false;
    }
    boardC = static_cast<int>((screenX - startXPos) / squareSide);
    int rawRow = static_cast<int>((screenY - startYPos) / squareSide);
    boardR = isFlipped ? (7 - rawRow) : rawRow;
    return (boardR >= 0 && boardR < 8 && boardC >= 0 && boardC < 8);
}

SDL_FRect Board::getSquareRect(int r, int c) const {
    if (r >= 0 && r < 8 && c >= 0 && c < 8) {
        return boardGrid[r][c];
    }
    return {0, 0, 0, 0};
}

std::vector<Move> Board::getAllLegalMoves(Color color, bool generateCastlingMoves) const {
    std::vector<Move> allLegalMoves;
    allLegalMoves.reserve(256);
    getAllLegalMoves(color, allLegalMoves, generateCastlingMoves);
    return allLegalMoves;
}

void Board::getAllLegalMoves(Color color, std::vector<Move>& out, bool generateCastlingMoves) const {
    out.clear();
    out.reserve(256);
    g_profiler.startTimer("getAllLegalMoves");

    const std::vector<Piece*>& pieces = pieceManager->getPieces(color);

    std::vector<Move> pieceMoves;
    pieceMoves = getAllPseudoLegalMoves(color, generateCastlingMoves);
    for (const Move& move : pieceMoves) {
        if (const_cast<Board*>(this)->checkIfMoveRemovesCheck(move)) {
            out.push_back(move);
        }
    }
    pieceMoves.clear();
    g_profiler.endTimer("getAllLegalMoves");
}

std::vector<Move> Board::getAllPseudoLegalMoves(Color color, bool generateCastlingMoves) const {
    std::vector<Move> allPseudoLegalMoves;
    allPseudoLegalMoves.reserve(256);
    getAllPseudoLegalMoves(color, allPseudoLegalMoves, generateCastlingMoves);
    return allPseudoLegalMoves;
}

void Board::getAllPseudoLegalMoves(Color color, std::vector<Move>& out, bool generateCastlingMoves) const {
    out.clear();
    out.reserve(256);
    g_profiler.startTimer("getAllPseudoLegalMoves");

    const std::vector<Piece*>& pieces = pieceManager->getPieces(color);

    std::vector<Move> pieceMoves;
    for (Piece* piece : pieces) {
        if (!piece) continue;
        piece->getPseudoLegalMoves(*this, pieceMoves, generateCastlingMoves);

        for (const Move& move : pieceMoves) {
            out.push_back(move);
        }
        pieceMoves.clear();
    }
    g_profiler.endTimer("getAllPseudoLegalMoves");
}

bool Board::isKingInCheck(Color color) const {
    g_profiler.startTimer("isKingInCheck");
    bool res = const_cast<Board*>(this)->isKingInCheck(color, nullptr);
    g_profiler.endTimer("isKingInCheck");
    return res;
}

bool Board::isKingInCheck(Color color, const Move* hypotheticalMove) {
    std::pair<int,int> kingPos{-1,-1};

    if (hypotheticalMove) {
        int r1 = hypotheticalMove->startPos.first;
        int c1 = hypotheticalMove->startPos.second;
        int r2 = hypotheticalMove->endPos.first;
        int c2 = hypotheticalMove->endPos.second;

        Piece* movingPiece = getPieceAt(r1, c1);
        Piece* capturedPiece = getPieceAt(r2, c2);
        
        pieceGrid[r1][c1] = nullptr;
        
        std::unique_ptr<Piece> tempPromotedPiece;
        Piece* finalPiece = movingPiece;
        
        if (hypotheticalMove->isPromotion && movingPiece && movingPiece->getType() == PAWN) {
            Color pieceColor = movingPiece->getColor();
            SDL_Renderer* renderer = movingPiece->getRenderer();
            
            switch (hypotheticalMove->promotionType) {
                case QUEEN:
                    tempPromotedPiece = std::make_unique<Queen>(pieceColor, QUEEN, renderer);
                    break;
                case ROOK:
                    tempPromotedPiece = std::make_unique<Rook>(pieceColor, ROOK, renderer);
                    break;
                case BISHOP:
                    tempPromotedPiece = std::make_unique<Bishop>(pieceColor, BISHOP, renderer);
                    break;
                case KNIGHT:
                    tempPromotedPiece = std::make_unique<Knight>(pieceColor, KNIGHT, renderer);
                    break;
                default:
                    tempPromotedPiece = std::make_unique<Queen>(pieceColor, QUEEN, renderer);
                    break;
            }
            
            if (tempPromotedPiece) {
                tempPromotedPiece->setPosition(r2, c2);
                finalPiece = tempPromotedPiece.get();
            }
        }
        
        pieceGrid[r2][c2] = finalPiece;

        if (movingPiece && movingPiece->getType() == KING) {
            kingPos = { r2, c2 };
        } else {
            Piece* king = pieceManager->findKing(color);
            if (king) kingPos = king->getPosition();
        }

        bool result = true;
        if (kingPos.first != -1) {
            result = isSquareAttacked(kingPos.first, kingPos.second, (color == WHITE ? BLACK : WHITE));
        }

        pieceGrid[r1][c1] = movingPiece;
        pieceGrid[r2][c2] = capturedPiece;

        return result;
    } else {
        Piece* king = pieceManager->findKing(color);
        if (!king) {
            LOG_ERROR(std::string("Error: No king of color ") + (color == WHITE ? "White" : "Black") + " found on the board.");
            return true;
        }
        auto [kr, kc] = king->getPosition();
        return isSquareAttacked(kr, kc, (color == WHITE ? BLACK : WHITE));
    }
}

const Move* Board::getLastMovePtr() const {
    if (moveExecutor) return moveExecutor->getLastMovePtr();
    return nullptr;
}

bool Board::isSquareAttacked(int r, int c, Color byColor) const {
    int dir = (byColor == BLACK ? +1 : -1);
    int pr = r - dir;
    for (int dc : {-1, +1}) {
        int pc = c + dc;
        if (pr >= 0 && pr < 8 && pc >= 0 && pc < 8) {
            Piece* p = getPieceAt(pr, pc);
            if (p && p->getColor() == byColor && p->getType() == PAWN) return true;
        }
    }

    static const int kdr[8] = {+2,+2,-2,-2,+1,+1,-1,-1};
    static const int kdc[8] = {+1,-1,+1,-1,+2,-2,+2,-2};
    for (int i = 0; i < 8; ++i) {
        int nr = r + kdr[i], nc = c + kdc[i];
        if (nr>=0&&nr<8&&nc>=0&&nc<8) {
            Piece* p = getPieceAt(nr,nc);
            if (p && p->getColor()==byColor && p->getType()==KNIGHT) return true;
        }
    }

    for (int dr=-1; dr<=1; ++dr) for (int dc=-1; dc<=1; ++dc) {
        if (dr==0 && dc==0) continue;
        int nr=r+dr, nc=c+dc;
        if (nr>=0&&nr<8&&nc>=0&&nc<8) {
            Piece* p = getPieceAt(nr,nc);
            if (p && p->getColor()==byColor && p->getType()==KING) return true;
        }
    }

    auto ray = [&](int dr, int dc, PieceType t1, PieceType t2){
        int nr=r+dr, nc=c+dc;
        while (nr>=0&&nr<8&&nc>=0&&nc<8) {
            Piece* p = getPieceAt(nr,nc);
            if (p) {
                if (p->getColor()!=byColor) return false;
                PieceType pt = p->getType();
                return (pt==t1 || pt==t2);
            }
            nr+=dr; nc+=dc;
        }
        return false;
    };

    if (ray(+1,0,ROOK,QUEEN)) return true;
    if (ray(-1,0,ROOK,QUEEN)) return true;
    if (ray(0,+1,ROOK,QUEEN)) return true;
    if (ray(0,-1,ROOK,QUEEN)) return true;
    if (ray(+1,+1,BISHOP,QUEEN)) return true;
    if (ray(+1,-1,BISHOP,QUEEN)) return true;
    if (ray(-1,+1,BISHOP,QUEEN)) return true;
    if (ray(-1,-1,BISHOP,QUEEN)) return true;

    return false;
}

bool Board::checkIfMoveRemovesCheck(const Move& move) {
    Piece* movingPiece = getPieceAt(move.startPos.first, move.startPos.second);
    if (!movingPiece) return false;

    Color moverColor = movingPiece->getColor();
    bool stillInCheck = isKingInCheck(moverColor, &move);
    return !stillInCheck;
}

bool Board::isCheckMate(Color color) {
    if (!isKingInCheck(color)) {
        return false;
    }

    std::vector<Move> pseudoLegalMoves = getAllLegalMoves(color, false);

    for (const auto& move : pseudoLegalMoves) {
        if (checkIfMoveRemovesCheck(move)) {
            return false;
        }
    }
    return true;
}

bool Board::isStaleMate(Color color) {
    if (isKingInCheck(color)) {
        return false;
    }

    std::vector<Move> pseudoLegalMoves = getAllLegalMoves(color, false);

    for (const auto& move : pseudoLegalMoves) {
        if (checkIfMoveRemovesCheck(move)) {
            return false;
        }
    }
    return true;
}

void Board::logCapturedPieces(Color capturer) const {
    const auto& capturedList = (capturer == BLACK) ? whiteCapturedPieces : blackCapturedPieces;
    if (capturedList.empty()) return;

    std::string list = (capturer == BLACK ? "Black has captured: " : "White has captured: ");
    for (size_t i = 0; i < capturedList.size(); ++i) {
        if (capturedList[i]) {
            list += capturedList[i]->stringPieceType();
            if (i < capturedList.size() - 1) list += ", ";
        }
    }
    Logger::log(LogLevel::INFO, list, __FILE__, __LINE__);
}

void Board::updatePiecePositionInManager(Piece* piece) {
    if (!piece) return;
    pieceManager->movePiece(piece->id, piece->getPosition());
}

void Board::handlePawnPromotion(const Piece* pawn, int row, int col) {
    if (!pawn || pawn->getType() != PAWN) return;
    Color color = pawn->getColor();
    if ((color == WHITE && row == 0) || (color == BLACK && row == 7)) {
        SDL_Renderer* pieceRenderer = pawn->getRenderer();
        showPromotionDialog(row, col, color, pieceRenderer);
    }
}

void Board::clearEnPassantFlags(Color colorToClear) {
    const std::vector<Piece*>& pieces = pieceManager->getPieces(colorToClear);
    for (Piece* p : pieces) {
        if (!p) continue;
        if (p->getType() == PAWN) static_cast<Pawn*>(p)->setEnPassantCaptureEligible(false);
    }
    return;
}
 
void Board::promotePawnTo(int row, int col, Color color, PieceType pieceType, SDL_Renderer* renderer) {
    std::unique_ptr<Piece> newPiece;
    switch (pieceType) {
        case QUEEN:  newPiece = std::make_unique<Queen>(color, QUEEN, renderer); break;
        case ROOK:   newPiece = std::make_unique<Rook>(color, ROOK, renderer); break;
        case BISHOP: newPiece = std::make_unique<Bishop>(color, BISHOP, renderer); break;
        case KNIGHT: newPiece = std::make_unique<Knight>(color, KNIGHT, renderer); break;
        default:     newPiece = std::make_unique<Queen>(color, QUEEN, renderer); break;
    }

    newPiece->setPosition(row, col);
    newPiece->setHasMoved(true);

    if (pieceGrid[row][col]) {
        Piece* old = pieceGrid[row][col];
        {
            std::ostringstream oss;
            oss << "promotePawnTo: removing existing piece id=" << old->id
                << " type=" << old->stringPieceType()
                << " at (" << row << "," << col << ")";
            Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);
        }
        pieceManager->removePiece(old->id);
        pieceGrid[row][col] = nullptr;
    }

    Piece* rawNew = newPiece.get();
    pieceManager->addPiece(std::move(newPiece));
    pieceGrid[row][col] = rawNew;
    updatePiecePositionInManager(pieceGrid[row][col]);
}
 
void Board::showPromotionDialog(int row, int col, Color color, SDL_Renderer* renderer) {
    int boardX = static_cast<int>(startXPos + col * squareSide);
    int boardY = static_cast<int>(startYPos + row * squareSide);
    
    promotionDialog = std::make_unique<UIPromotionDialog>(
        boardX, boardY, squareSide, screenWidth, color, renderer
    );
    
    promotionDialog->setOnPromotionSelected([this, row, col, color, renderer](PieceType selectedType) {
        this->promotePawnTo(row, col, color, selectedType, renderer);
    });
    
    promotionDialog->show();
}
 
void Board::updatePromotionDialog(Input& input) {
    if (promotionDialog && promotionDialog->visible) {
        promotionDialog->update(input);
    }
}
 
void Board::renderPromotionDialog(SDL_Renderer* renderer) {
    if (promotionDialog && promotionDialog->visible) {
        promotionDialog->render(renderer);
    }
}
 
bool Board::isPromotionDialogActive() const {
    return promotionDialog && promotionDialog->visible;
}

bool Board::isPinnedPiece(int pieceRow, int pieceCol, Color pieceColor) const {
    Piece* king = pieceManager->findKing(pieceColor);
    if (!king) return false;
    
    auto [kingRow, kingCol] = king->getPosition();
    
    int rowDiff = pieceRow - kingRow;
    int colDiff = pieceCol - kingCol;
    
    if (rowDiff != 0 && colDiff != 0 && abs(rowDiff) != abs(colDiff)) {
        return false;
    }
    
    int rowDir = (rowDiff == 0) ? 0 : (rowDiff > 0 ? 1 : -1);
    int colDir = (colDiff == 0) ? 0 : (colDiff > 0 ? 1 : -1);
    
    int checkRow = kingRow + rowDir;
    int checkCol = kingCol + colDir;
    
    while (checkRow != pieceRow || checkCol != pieceCol) {
        if (checkRow < 0 || checkRow >= 8 || checkCol < 0 || checkCol >= 8) {
            return false;
        }
        
        Piece* p = getPieceAt(checkRow, checkCol);
        if (p) {
            return false;
        }
        
        checkRow += rowDir;
        checkCol += colDir;
    }
    
    checkRow = pieceRow + rowDir;
    checkCol = pieceCol + colDir;
    
    while (checkRow >= 0 && checkRow < 8 && checkCol >= 0 && checkCol < 8) {
        Piece* p = getPieceAt(checkRow, checkCol);
        if (p) {
            if (p->getColor() != pieceColor) {
                PieceType type = p->getType();
                
                bool canAttackLine = false;
                if (rowDir == 0 || colDir == 0) {
                    canAttackLine = (type == ROOK || type == QUEEN);
                } else {
                    canAttackLine = (type == BISHOP || type == QUEEN);
                }
                
                if (canAttackLine) {
                    return true;
                }
            }
            break;
        }
        checkRow += rowDir;
        checkCol += colDir;
    }
    
    return false;
}

std::string Board::getCurrentFEN() const {
    std::ostringstream fen;
    
    for (int rank = 7; rank >= 0; rank--) {
        int emptyCount = 0;
        for (int file = 0; file < 8; file++) {
            Piece* piece = pieceGrid[rank][file];
            if (piece == nullptr) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen << emptyCount;
                    emptyCount = 0;
                }
                
                char pieceChar = '?';
                switch (piece->getType()) {
                    case PAWN:   pieceChar = 'p'; break;
                    case ROOK:   pieceChar = 'r'; break;
                    case KNIGHT: pieceChar = 'n'; break;
                    case BISHOP: pieceChar = 'b'; break;
                    case QUEEN:  pieceChar = 'q'; break;
                    case KING:   pieceChar = 'k'; break;
                    default: break;
                }
                
                if (piece->getColor() == WHITE) {
                    pieceChar = std::toupper(pieceChar);
                }
                
                fen << pieceChar;
            }
        }
        if (emptyCount > 0) {
            fen << emptyCount;
        }
        if (rank > 0) {
            fen << '/';
        }
    }
    
    fen << ' ' << (currentPlayer == WHITE ? 'w' : 'b');
    
    fen << ' ' << '-';
    
    fen << ' ' << '-';
    
    fen << ' ' << halfMoveClock;
    
    fen << ' ' << fullMoveNumber;
    
    return fen.str();
}

void Board::syncBitboardState() {
    bbState->clear();
    
    int whitePieceCount = 0, blackPieceCount = 0;
    
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Piece* piece = pieceGrid[row][col];
            if (!piece) continue;
            
            int bbRank = 7 - row;
            int sq = bbRank * 8 + col;
            
            Color pieceColor = piece->getColor();
            PieceType pieceType = piece->getType();
            
            if (pieceColor == WHITE) whitePieceCount++;
            else blackPieceCount++;
            
            int bbPiece = 0;
            switch (pieceType) {
                case PAWN:   bbPiece = chess::PIECE_PAWN; break;
                case KNIGHT: bbPiece = chess::PIECE_KNIGHT; break;
                case BISHOP: bbPiece = chess::PIECE_BISHOP; break;
                case ROOK:   bbPiece = chess::PIECE_ROOK; break;
                case QUEEN:  bbPiece = chess::PIECE_QUEEN; break;
                case KING:   bbPiece = chess::PIECE_KING; break;
            }
            
            int colorBit = (pieceColor == WHITE) ? chess::COLOR_WHITE : chess::COLOR_BLACK;
            int colorIdx = (pieceColor == WHITE) ? 0 : 1;
            bbPiece |= colorBit;
            
            bbState->square[sq] = bbPiece;
            
            switch (chess::typeOf(bbPiece)) {
                case chess::PIECE_PAWN:   bbState->pawns[colorIdx].add(sq); break;
                case chess::PIECE_KNIGHT: bbState->knights[colorIdx].add(sq); break;
                case chess::PIECE_BISHOP: bbState->bishops[colorIdx].add(sq); break;
                case chess::PIECE_ROOK:   bbState->rooks[colorIdx].add(sq); break;
                case chess::PIECE_QUEEN:  bbState->queens[colorIdx].add(sq); break;
                case chess::PIECE_KING:   bbState->kingSquare[colorIdx] = sq; break;
            }
        }
    }
    
    bbState->whiteToMove = (currentPlayer == WHITE);
    
    uint32_t castlingRights = 0;
    
    Piece* whiteKing = pieceGrid[7][4];
    Piece* whiteKingsideRook = pieceGrid[7][7];
    if (whiteKing && whiteKing->getType() == KING && !whiteKing->getHasMoved() &&
        whiteKingsideRook && whiteKingsideRook->getType() == ROOK && !whiteKingsideRook->getHasMoved()) {
        castlingRights |= chess::CR_WHITE_K;
    }
    
    Piece* whiteQueensideRook = pieceGrid[7][0];
    if (whiteKing && whiteKing->getType() == KING && !whiteKing->getHasMoved() &&
        whiteQueensideRook && whiteQueensideRook->getType() == ROOK && !whiteQueensideRook->getHasMoved()) {
        castlingRights |= chess::CR_WHITE_Q;
    }
    
    Piece* blackKing = pieceGrid[0][4];
    Piece* blackKingsideRook = pieceGrid[0][7];
    if (blackKing && blackKing->getType() == KING && !blackKing->getHasMoved() &&
        blackKingsideRook && blackKingsideRook->getType() == ROOK && !blackKingsideRook->getHasMoved()) {
        castlingRights |= chess::CR_BLACK_K;
    }
    
    Piece* blackQueensideRook = pieceGrid[0][0];
    if (blackKing && blackKing->getType() == KING && !blackKing->getHasMoved() &&
        blackQueensideRook && blackQueensideRook->getType() == ROOK && !blackQueensideRook->getHasMoved()) {
        castlingRights |= chess::CR_BLACK_Q;
    }
    
    bbState->gameState = castlingRights;
    
    const std::vector<Piece*>& whitePawns = pieceManager->getPieces(WHITE);
    const std::vector<Piece*>& blackPawns = pieceManager->getPieces(BLACK);
    
    for (Piece* piece : (currentPlayer == WHITE ? blackPawns : whitePawns)) {
        if (piece->getType() == PAWN) {
            Pawn* pawn = static_cast<Pawn*>(piece);
            if (pawn->getEnPassantCaptureEligible()) {
                auto pos = pawn->getPosition();
                int col = pos.second;
                chess::setEPFile(bbState->gameState, col);
                break;
            }
        }
    }
    
    bbState->fiftyMoveCounter = halfMoveClock;
    bbState->plyCount = (fullMoveNumber - 1) * 2 + (currentPlayer == BLACK ? 1 : 0);
    
    bbState->zobristKey = chess::Zobrist::calculateZobristKey(*bbState);
}

Move Board::bbMoveToMove(const chess::BBMove& bbMove) const {
    Move oldMove;
    
    int from = bbMove.startSquare();
    int to = bbMove.targetSquare();
    
    int fromRank = from / 8;
    int fromFile = from % 8;
    int fromRow = 7 - fromRank;
    
    int toRank = to / 8;
    int toFile = to % 8;
    int toRow = 7 - toRank;
    
    oldMove.startPos = {fromRow, fromFile};
    oldMove.endPos = {toRow, toFile};
    
    oldMove.piece = pieceGrid[fromRow][fromFile];
    
    chess::BBMove::Flag flag = bbMove.flag();
    
    if (flag == chess::BBMove::Castling) {
        if (to > from) {
            oldMove.castlingType = CastlingType::KING_SIDE;
        } else {
            oldMove.castlingType = CastlingType::QUEEN_SIDE;
        }
    } else {
        oldMove.castlingType = CastlingType::NONE;
    }
    
    oldMove.isPromotion = bbMove.isPromotion();
    if (oldMove.isPromotion) {
        switch (flag) {
            case chess::BBMove::PromoteToQueen:  oldMove.promotionType = QUEEN; break;
            case chess::BBMove::PromoteToRook:   oldMove.promotionType = ROOK; break;
            case chess::BBMove::PromoteToBishop: oldMove.promotionType = BISHOP; break;
            case chess::BBMove::PromoteToKnight: oldMove.promotionType = KNIGHT; break;
            default: oldMove.promotionType = QUEEN; break;
        }
    }
    
    Piece* targetPiece = pieceGrid[to / 8][to % 8];
    oldMove.capturedPiece = targetPiece;
    
    return oldMove;
}

std::vector<Move> Board::getAllPseudoLegalMovesBB(Color color, bool generateCastlingMoves) {
    std::vector<Move> moves;
    getAllPseudoLegalMovesBB(color, moves, generateCastlingMoves);
    return moves;
}

void Board::getAllPseudoLegalMovesBB(Color color, std::vector<Move>& out, bool generateCastlingMoves) {
    out.clear();
    out.reserve(256);
    
    g_profiler.startTimer("getAllPseudoLegalMovesBB");
    
    syncBitboardState();
    
    bool originalSideToMove = bbState->whiteToMove;
    bbState->whiteToMove = (color == WHITE);
    
    std::vector<chess::BBMove> bbMoves = bbGenerator->generateMoves(*bbState, false);
    
    bbState->whiteToMove = originalSideToMove;
    
    for (const auto& bbMove : bbMoves) {
        Move move = bbMoveToMove(bbMove);
        out.push_back(move);
    }
    
    g_profiler.endTimer("getAllPseudoLegalMovesBB");
}
