#include "board.h"
#include "board/moveExecutor.h"
#include "logger.h"
#include "board/pieceManager.h"
#include "board/boardRenderer.h"
#include "pieces/pieces.h"
#include "ui/uiPromotionDialog.h"
#include "utilities.h"
#include "../include/perfProfiler.h"
#include <unordered_map>
#include <cctype>
#include <chrono>
#include <algorithm>
#include <sstream>

using namespace std::chrono;

MakeUnmakeProfile g_muProfile; // defined for fine-grained profiling

Board::Board(int width, int height, float offSet) {
    screenHeight = height;
    screenWidth = width;
    this->offSet = offSet;
    startXPos = this->offSet;
    startYPos = this->offSet;
    endXPos = screenWidth - this->offSet;
    endYPos = screenHeight - this->offSet;
    // Calculate the side length of each square
    this->squareSide = (static_cast<float>(screenWidth) - 2.0f * this->offSet) / 8.0f;
    pieceManager = std::make_unique<PieceManager>();
    // Create MoveExecutor and give it access to this board's internals via pointer
    moveExecutor = std::make_unique<MoveExecutor>(this);
}

Board::~Board() {
    // Unique pointers will automatically clean up
}

void Board::loadFEN(const std::string& fen, SDL_Renderer* gameRenderer){
    std::unordered_map<char, std::pair<Color, PieceType>> pieceFromSymbol = {
        {'P', {WHITE, PAWN}}, {'p', {BLACK, PAWN}},
        {'R', {WHITE, ROOK}}, {'r', {BLACK, ROOK}},
        {'N', {WHITE, KNIGHT}}, {'n', {BLACK, KNIGHT}},
        {'B', {WHITE, BISHOP}}, {'b', {BLACK, BISHOP}},
        {'Q', {WHITE, QUEEN}}, {'q', {BLACK, QUEEN}},
        {'K', {WHITE, KING}}, {'k', {BLACK, KING}}
    };
    clearPieceGridAndPieces();

    std::string fenBoard = splitString(fen, ' ')[0];
    int row = 0, col = 0;
    for (char c : fenBoard) {
        if (c == '/') {
            row++;
            col = 0;
        } else if (isdigit(c)) {
            col += c - '0'; // Skip empty squares
        } else {
            if (pieceFromSymbol.find(c) != pieceFromSymbol.end()) {
                Color color = pieceFromSymbol[c].first;
                PieceType type = pieceFromSymbol[c].second;
                // Create piece based on type
                std::unique_ptr<Piece> newPiece;
                switch (type) {
                    case PAWN:
                        newPiece = std::make_unique<Pawn>(color, type, gameRenderer);
                        break;
                    case ROOK:
                        newPiece = std::make_unique<Rook>(color, type, gameRenderer);
                        break;
                    case KNIGHT:
                        newPiece = std::make_unique<Knight>(color, type, gameRenderer);
                        break;
                    case BISHOP:
                        newPiece = std::make_unique<Bishop>(color, type, gameRenderer);
                        break;
                    case QUEEN:
                        newPiece = std::make_unique<Queen>(color, type, gameRenderer);
                        break;
                    case KING:
                        newPiece = std::make_unique<King>(color, type, gameRenderer);
                        break;
                }
                newPiece->setPosition(row, col);
                            // Register ownership with the PieceManager and store a non-owning pointer in the grid
                            Piece* rawPtr = newPiece.get();
                            pieceManager->addPiece(std::move(newPiece));
                            pieceGrid[row][col] = rawPtr;
                col++;
            }
        }
    }

    this->startFEN = fen;

#ifdef DEBUG
    {
        std::ostringstream oss;
        oss << "loadFEN: loaded pieces map size=" << pieceManager->getAllPieceMap().size();
        Logger::log(LogLevel::DEBUG, oss.str(), __FILE__, __LINE__);
    }
#endif
}

void Board::initializeBoard(SDL_Renderer* gameRenderer) {
    for (auto& row : pieceGrid) {
        row.fill(nullptr);
    }
    for (int i = 0; i < 8; i++) { // Rows
        for (int j = 0; j < 8; j++) { // Columns
            boardGrid[i][j].x = startXPos + (j * squareSide);
            boardGrid[i][j].y = startYPos + (i * squareSide);
            boardGrid[i][j].w = squareSide;
            boardGrid[i][j].h = squareSide;
        }
    }
    // Create and initialize the board renderer only when a valid renderer is provided.
    // In headless/perft-only runs we may pass nullptr to avoid any SDL texture or
    // renderer work; pieces will be created without textures in that case.
    if (gameRenderer) {
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
    // Recompute boardGrid y positions so that row 7 is at bottom when flipped
    isFlipped = flipped;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            int visualRow = flipped ? (7 - i) : i;
            boardGrid[i][j].x = startXPos + (j * squareSide);
            boardGrid[i][j].y = startYPos + (visualRow * squareSide);
            boardGrid[i][j].w = squareSide;
            boardGrid[i][j].h = squareSide;
        }
    }
}

void Board::resetBoard(SDL_Renderer* gameRenderer) {
    // Clear all pieces
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            pieceGrid[i][j] = nullptr;
        }
    }
    
    // Clear captured pieces
    whiteCapturedPieces.clear();
    blackCapturedPieces.clear();
    
    // Reset to starting position with valid renderer
    loadFEN(this->startFEN, gameRenderer);  // Pass the renderer
}

void Board::updatePieceGrid() {
    // Placeholder for future board state updates
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

    // Prepare context for rendering
    RenderContext context;
    context.selectedSquare = selectedSquare;
    context.possibleMoves = possibleMoves;
    context.showCoordinates = true; // Could be made configurable
    context.highlightLastMove = true; // Could be made configurable
    context.lastMove = getLastMovePtr();

    const auto& pieces = pieceManager->getAllPieces();

    // Delegate drawing to BoardRenderer
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
        return false; // Click is outside the board grid
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
    allLegalMoves.reserve(256); // Reserve space to minimize reallocations
    g_profiler.startTimer("getAllLegalMoves");

    // Get pieces of a specific color using the manager
    const std::vector<Piece*>& pieces = pieceManager->getPieces(color);

    for (Piece* piece : pieces) {
        if (!piece) continue;
        std::vector<Move> pieceMoves = piece->getPseudoLegalMoves(*this, generateCastlingMoves);
        allLegalMoves.insert(allLegalMoves.end(), pieceMoves.begin(), pieceMoves.end());
    }
    g_profiler.endTimer("getAllLegalMoves");
    return allLegalMoves;
}

// Keep the original const entrypoint but forward to non-const implementation
bool Board::isKingInCheck(Color color) const {
    // forward to non-const overload (safe because we pass nullptr => no temporary mutation)
    g_profiler.startTimer("isKingInCheck");
    bool res = const_cast<Board*>(this)->isKingInCheck(color, nullptr);
    g_profiler.endTimer("isKingInCheck");
    return res;
}

// Non-const overload: optionally evaluate king-in-check after applying a hypothetical move
bool Board::isKingInCheck(Color color, const Move* hypotheticalMove) {
    // Determine king position under either the current board or after the hypothetical move.
    std::pair<int,int> kingPos{-1,-1};

    if (hypotheticalMove) {
        int r1 = hypotheticalMove->startPos.first;
        int c1 = hypotheticalMove->startPos.second;
        int r2 = hypotheticalMove->endPos.first;
        int c2 = hypotheticalMove->endPos.second;

        Piece* movingPiece = getPieceAt(r1, c1);
        // Apply the move temporarily on the non-owning grid
        Piece* capturedPiece = getPieceAt(r2, c2);
        pieceGrid[r1][c1] = nullptr;
        pieceGrid[r2][c2] = movingPiece;

        if (movingPiece && movingPiece->getType() == KING) {
            kingPos = { r2, c2 };
        } else {
            Piece* king = pieceManager->findKing(color);
            if (king) kingPos = king->getPosition();
        }

        // If no king found, treat as 'in check' defensively
        bool result = true;
        if (kingPos.first != -1) {
            result = isSquareAttacked(kingPos.first, kingPos.second, (color == WHITE ? BLACK : WHITE));
        }

        // Revert the temporary move
        pieceGrid[r1][c1] = movingPiece;
        pieceGrid[r2][c2] = capturedPiece;

        return result;
    } else {
        // No hypothetical move: regular check detection
        Piece* king = pieceManager->findKing(color);
        if (!king) {
            LOG_ERROR(std::string("Error: No king of color ") + (color == WHITE ? "White" : "Black") + " found on the board.");
            return true; // treat missing king as in-check / loss
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
    // Pawns
    int dir = (byColor == BLACK ? +1 : -1);
    int pr = r - dir; // pawn would be one step behind the target in its moving direction
    for (int dc : {-1, +1}) {
        int pc = c + dc; // FIX: attacker column is c +/- 1 (was c - dc which inverted)
        if (pr >= 0 && pr < 8 && pc >= 0 && pc < 8) {
            Piece* p = getPieceAt(pr, pc);
            if (p && p->getColor() == byColor && p->getType() == PAWN) return true;
        }
    }

    // Knights
    static const int kdr[8] = {+2,+2,-2,-2,+1,+1,-1,-1};
    static const int kdc[8] = {+1,-1,+1,-1,+2,-2,+2,-2};
    for (int i = 0; i < 8; ++i) {
        int nr = r + kdr[i], nc = c + kdc[i];
        if (nr>=0&&nr<8&&nc>=0&&nc<8) {
            Piece* p = getPieceAt(nr,nc);
            if (p && p->getColor()==byColor && p->getType()==KNIGHT) return true;
        }
    }

    // King (adjacent)
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

    // Rook/Queen rays (orthogonal)
    if (ray(+1,0,ROOK,QUEEN)) return true;
    if (ray(-1,0,ROOK,QUEEN)) return true;
    if (ray(0,+1,ROOK,QUEEN)) return true;
    if (ray(0,-1,ROOK,QUEEN)) return true;
    // Bishop/Queen rays (diagonal)
    if (ray(+1,+1,BISHOP,QUEEN)) return true;
    if (ray(+1,-1,BISHOP,QUEEN)) return true;
    if (ray(-1,+1,BISHOP,QUEEN)) return true;
    if (ray(-1,-1,BISHOP,QUEEN)) return true;

    return false;
}

bool Board::checkIfMoveRemovesCheck(const Move& move) {
    Piece* movingPiece = getPieceAt(move.startPos.first, move.startPos.second);
    if (!movingPiece) return false; // invalid move

    Color moverColor = movingPiece->getColor();
    // Use the unified isKingInCheck that can evaluate a hypothetical move.
    bool stillInCheck = isKingInCheck(moverColor, &move);
    return !stillInCheck;
}

bool Board::isCheckMate(Color color) {
    if (!isKingInCheck(color)) {
        return false; // Not in check, so cannot be checkmate.
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
        return false; // In check, so cannot be stalemate.
    }

    std::vector<Move> pseudoLegalMoves = getAllLegalMoves(color, false);

    for (const auto& move : pseudoLegalMoves) {
        if (checkIfMoveRemovesCheck(move)) {
            return false;
        }
    }
    return true;
}

// --- Helper methods added in Board ---

// DRY: shared capture log
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
    // Forward to PieceManager
    pieceManager->movePiece(piece->id, piece->getPosition());
}

// DRY: unified promotion handler
void Board::handlePawnPromotion(const Piece* pawn, int row, int col) {
    if (!pawn || pawn->getType() != PAWN) return;
    Color color = pawn->getColor();
    if ((color == WHITE && row == 0) || (color == BLACK && row == 7)) {
        SDL_Renderer* pieceRenderer = pawn->getRenderer();
        showPromotionDialog(row, col, color, pieceRenderer);
    }
}

void Board::clearEnPassantFlags(Color colorToClear) {
    // Use the authoritative PieceManager collection for the color.
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

    // If there's an existing piece (pawn) on the square, remove it from the manager
    if (pieceGrid[row][col]) {
        Piece* old = pieceGrid[row][col];
        {
            std::ostringstream oss;
            oss << "promotePawnTo: removing existing piece id=" << old->id
                << " type=" << old->stringPieceType()
                << " at (" << row << "," << col << ")";
            Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);
        }
        // remove and discard ownership from manager
        pieceManager->removePiece(old->id);
        pieceGrid[row][col] = nullptr;
    }

    // Transfer ownership to PieceManager and keep a raw pointer in the non-owning grid
    Piece* rawNew = newPiece.get();
    pieceManager->addPiece(std::move(newPiece));
    pieceGrid[row][col] = rawNew;
    updatePiecePositionInManager(pieceGrid[row][col]);
}
 
void Board::showPromotionDialog(int row, int col, Color color, SDL_Renderer* renderer) {
    // Calculate board position in screen coordinates
    int boardX = static_cast<int>(startXPos + col * squareSide);
    int boardY = static_cast<int>(startYPos + row * squareSide);
    
    // Create promotion dialog with callback
    promotionDialog = std::make_unique<UIPromotionDialog>(
        boardX, boardY, squareSide, screenWidth, color, renderer
    );
    
    // Set callback to handle piece selection
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
