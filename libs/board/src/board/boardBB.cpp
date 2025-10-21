#include <chess/board/boardBB.h>
#include <chess/utils/logger.h>
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
#include <chess/board/bitboard/move_exec.h>
#include <chess/board/pieces/piece_const.h>

#include <unordered_map>
#include <cctype>
#include <algorithm>
#include <sstream>



BoardBB::BoardBB(int width, int height, float offSet) {
    screenHeight = height;
    screenWidth = width;
    this->offSet = offSet;
    startXPos = this->offSet;
    startYPos = this->offSet;
    endXPos = screenWidth - this->offSet;
    endYPos = screenHeight - this->offSet;
    this->squareSide = (static_cast<float>(screenWidth) - 2.0f * this->offSet) / 8.0f;

    chess::initBitboardSystem();    
    bbState = std::make_unique<chess::BitboardState>();
    bbGenerator = std::make_unique<chess::MoveGeneratorBB>();
    moveExecutor = std::make_unique<chess::MoveExecutorBB>(*bbState);
}

BoardBB::~BoardBB() {
}

void BoardBB::loadFEN(const std::string& fen, SDL_Renderer* gameRenderer){
    bbState->loadFromFEN(fen);
    currentPlayer = bbState->whiteToMove ? WHITE : BLACK;
    // ensure UI renderer is set and sync UI pieces to bbState
    if (gameRenderer) {
        uiRenderer = gameRenderer;
        syncUIFromBBState(gameRenderer);
    }
}

void BoardBB::initializeBoard(SDL_Renderer* gameRenderer) {
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
    
    uiRenderer = gameRenderer;
    for (auto &row : pieceGrid) for (auto &cell : row) cell.reset();

    loadFEN(this->startFEN, gameRenderer);
}

void BoardBB::setFlipped(bool flipped) {
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

void BoardBB::resetBoard(SDL_Renderer* gameRenderer) {
    bbState->clear();
    loadFEN(this->startFEN, gameRenderer);
}

chess::UndoState BoardBB::executeMove(const chess::BBMove& move, bool trackUndo) {
    chess::UndoState u = moveExecutor->makeMove(move);
    if (trackUndo) {
        halfMoveClock = bbState->fiftyMoveCounter;
        fullMoveNumber = (bbState->plyCount / 2) + 1;
        // Sync UI pieces after the bitboard move
        if (uiRenderer) syncUIFromBBState(uiRenderer);
        return u;
    }
    return chess::UndoState{};
}

void BoardBB::undoMove(const chess::BBMove& move, chess::UndoState& undo) {
    moveExecutor->undoMove(move, undo);
    halfMoveClock = bbState->fiftyMoveCounter;
    fullMoveNumber = (bbState->plyCount / 2) + 1;
    if (uiRenderer) syncUIFromBBState(uiRenderer);
}

void BoardBB::syncUIFromBBState(SDL_Renderer* gameRenderer) {
    if (!bbState) return;

    // Clear existing UI pieces (conservative approach)
    for (auto &row : pieceGrid) for (auto &cell : row) cell.reset();

    auto addPieceAt = [&](int sq, Color color, PieceType type) {
        int rank = sq / 8;
        int file = sq % 8;
        int row = 7 - rank; // convert bb rank to UI row
        int col = file;

        std::unique_ptr<Piece> newPiece;
        switch (type) {
            case PAWN:   newPiece = std::make_unique<Pawn>(color, PAWN, gameRenderer); break;
            case KNIGHT: newPiece = std::make_unique<Knight>(color, KNIGHT, gameRenderer); break;
            case BISHOP: newPiece = std::make_unique<Bishop>(color, BISHOP, gameRenderer); break;
            case ROOK:   newPiece = std::make_unique<Rook>(color, ROOK, gameRenderer); break;
            case QUEEN:  newPiece = std::make_unique<Queen>(color, QUEEN, gameRenderer); break;
            case KING:   newPiece = std::make_unique<King>(color, KING, gameRenderer); break;
            default:     newPiece = std::make_unique<Pawn>(color, PAWN, gameRenderer); break;
        }
        newPiece->setPosition(row, col);
        pieceGrid[row][col] = std::move(newPiece);
    };

    // iterate over bitboard state arrays and add pieces
    for (int colorIdx = 0; colorIdx <= 1; ++colorIdx) {
        Color color = (colorIdx == 0) ? WHITE : BLACK;

        for (int sq : bbState->pawns[colorIdx].squares) addPieceAt(sq, color, PAWN);
        for (int sq : bbState->knights[colorIdx].squares) addPieceAt(sq, color, KNIGHT);
        for (int sq : bbState->bishops[colorIdx].squares) addPieceAt(sq, color, BISHOP);
        for (int sq : bbState->rooks[colorIdx].squares) addPieceAt(sq, color, ROOK);
        for (int sq : bbState->queens[colorIdx].squares) addPieceAt(sq, color, QUEEN);
        int ksq = bbState->kingSquare[colorIdx];
        if (ksq >= 0) addPieceAt(ksq, color, KING);
    }
}

void BoardBB::draw(SDL_Renderer* renderer, const std::pair<int, int>* selectedSquare, const std::vector<Move>* possibleMoves) {
    if (!boardRenderer) return;
    // Use bitboard-backed draw path
    boardRenderer->drawPieces(*bbState);
}

int BoardBB::getPieceAt(int r, int c) const {
    return bbState->getPieceAt(r, c);
}

bool BoardBB::screenToBoardCoords(int screenX, int screenY, int& boardR, int& boardC) const {
    if (screenX < startXPos || screenX > endXPos || screenY < startYPos || screenY > endYPos) {
        return false;
    }
    boardC = static_cast<int>((screenX - startXPos) / squareSide);
    int rawRow = static_cast<int>((screenY - startYPos) / squareSide);
    boardR = isFlipped ? (7 - rawRow) : rawRow;
    return (boardR >= 0 && boardR < 8 && boardC >= 0 && boardC < 8);
}

SDL_FRect BoardBB::getSquareRect(int r, int c) const {
    if (r >= 0 && r < 8 && c >= 0 && c < 8) {
        return boardGrid[r][c];
    }
    return {0, 0, 0, 0};
}

std::vector<chess::BBMove> BoardBB::getAllLegalMoves(Color color) const {
    if (color == WHITE){
        whiteMoves = bbGenerator->generateMoves(*bbState);
        return whiteMoves;
    }
    else {
        blackMoves = bbGenerator->generateMoves(*bbState);
        return blackMoves;
    }
}



int BoardBB::getLastState() const {
    return bbState->zobristHistory.empty() ? 0 : bbState->zobristHistory.back();
}

int BoardBB::getPieceAt(int r, int c) const {
    return bbState->getPieceAt(r, c);
}


bool BoardBB::isCheckMate(Color color) {
    if (color == WHITE && bbState->whiteToMove == true && whiteMoves.empty() 
        && bbGenerator->getInCheck()) {
        return true;
    }
    if (color == BLACK && bbState->whiteToMove == false && blackMoves.empty() 
        && bbGenerator->getInCheck()) {
        return true;
    }
    return false;
}

bool BoardBB::isStaleMate(Color color) {
    if (color == WHITE && bbState->whiteToMove == true && whiteMoves.empty() 
        && !bbGenerator->getInCheck()) {
        return true;
    }
    if (color == BLACK && bbState->whiteToMove == false && blackMoves.empty() 
        && !bbGenerator->getInCheck()) {
        return true;
    }
    return false;
}


void BoardBB::handlePawnPromotion(const Piece* pawn, int row, int col) {
    if (!pawn || pawn->getType() != PAWN) return;
    Color color = pawn->getColor();
    if ((color == WHITE && row == 0) || (color == BLACK && row == 7)) {
        SDL_Renderer* pieceRenderer = pawn->getRenderer();
        showPromotionDialog(row, col, color, pieceRenderer);
    }
}

void BoardBB::promotePawnTo(int row, int col, Color color, PieceType pieceType, SDL_Renderer* renderer) {
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
        Piece* old = pieceGrid[row][col].get();
        {
            std::ostringstream oss;
            oss << "promotePawnTo: removing existing piece id=" << old->id
                << " type=" << old->stringPieceType()
                << " at (" << row << "," << col << ")";
            Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);
        }
        pieceGrid[row][col].reset();
    }

    pieceGrid[row][col] = std::move(newPiece);
}

void BoardBB::showPromotionDialog(int row, int col, Color color, SDL_Renderer* renderer) {
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

void BoardBB::updatePromotionDialog(Input& input) {
    if (promotionDialog && promotionDialog->visible) {
        promotionDialog->update(input);
    }
}
 
void BoardBB::renderPromotionDialog(SDL_Renderer* renderer) {
    if (promotionDialog && promotionDialog->visible) {
        promotionDialog->render(renderer);
    }
}

bool BoardBB::isPromotionDialogActive() const {
    return promotionDialog && promotionDialog->visible;
}