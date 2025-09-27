#include "board.h"
#include "logger.h"
#include "board/pieceManager.h"
#include "board/boardRenderer.h"
#include "pieces/pieces.h"
#include "ui/uiPromotionDialog.h"
#include "utilities.h"
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
    lastMove = Move(); // Initialize lastMove to a default Move
    pieceManager = std::make_unique<PieceManager>();
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
    // Create and initialize the board renderer
    boardRenderer = std::make_unique<BoardRenderer>(gameRenderer);
    boardRenderer->initializeLayout(boardGrid, squareSide, isFlipped);
    
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

void Board::draw(SDL_Renderer* renderer, const std::pair<int, int>* selectedSquare, const std::vector<Move>* possibleMoves) {
    if (!boardRenderer) return;

    // Prepare context for rendering
    RenderContext context;
    context.selectedSquare = selectedSquare;
    context.possibleMoves = possibleMoves;
    context.showCoordinates = true; // Could be made configurable
    context.highlightLastMove = true; // Could be made configurable
    context.lastMove = &lastMove;

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

void Board::movePiece(const Move& move) {
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;

    if (!(r1 >= 0 && r1 < 8 && c1 >= 0 && c1 < 8 &&
          r2 >= 0 && r2 < 8 && c2 >= 0 && c2 < 8 && pieceGrid[r1][c1])) {
        // Invalid move parameters or no piece at start
        return;
    }

    std::unique_ptr<Piece> pieceTaken;

    // Determine if a piece is captured and take ownership
    if (move.capturedPiece) {
        int capturedPieceRow = move.capturedPiece->getPosition().first;
        int capturedPieceCol = move.capturedPiece->getPosition().second;

        // Verify that the board actually has this piece at the expected location
                if (capturedPieceRow >= 0 && capturedPieceRow < 8 &&
                    capturedPieceCol >= 0 && capturedPieceCol < 8 &&
                    pieceGrid[capturedPieceRow][capturedPieceCol] &&
                    pieceGrid[capturedPieceRow][capturedPieceCol] == move.capturedPiece) {

                    // Remove ownership from PieceManager and take ownership into pieceTaken
                    pieceTaken = pieceManager->removePiece(move.capturedPiece->id);
                    pieceGrid[capturedPieceRow][capturedPieceCol] = nullptr;
                }
    } else if (pieceGrid[r2][c2]) {
        // If move.capturedPiece was null, but destination is occupied by an opponent
        if (pieceGrid[r1][c1]->getColor() != pieceGrid[r2][c2]->getColor()) {
            // Remove ownership from PieceManager and take ownership
            pieceTaken = pieceManager->removePiece(pieceGrid[r2][c2]->id);
            pieceGrid[r2][c2] = nullptr;
        }
    }

    // Perform the move: move the piece from (r1,c1) to (r2,c2)
    // If destination had a piece that wasn't captured via move.capturedPiece (defensive), remove it from caches
    if (pieceGrid[r2][c2]) {
        Piece* destP = pieceGrid[r2][c2];
        if (destP) {
            // Defensive: ensure PieceManager no longer owns this piece (capture case handled above)
            {
                std::ostringstream oss;
                oss << "movePiece: defensive remove from manager id=" << destP->id
                    << " type=" << destP->stringPieceType()
                    << " at (" << r2 << "," << c2 << ")";
                Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);
            }
            // If it still exists in manager, remove it
            auto removed = pieceManager->removePiece(destP->id);
            (void)removed;
        }
    }
    // Move pointer on the non-owning grid
    pieceGrid[r2][c2] = pieceGrid[r1][c1];
    pieceGrid[r1][c1] = nullptr;

    // Update the moved piece's internal state
    if (pieceGrid[r2][c2]) {
        pieceGrid[r2][c2]->setPosition(r2, c2);
        pieceGrid[r2][c2]->setHasMoved(true);
        // Notify PieceManager about the new position
        pieceManager->movePiece(pieceGrid[r2][c2]->id, {r2, c2});
    }

    // Add the captured piece (if any) to the appropriate list
    if (pieceTaken) {
        if (move.piece && move.piece->getColor() == BLACK) {
            whiteCapturedPieces.push_back(std::move(pieceTaken));
        } else if (move.piece && move.piece->getColor() == WHITE) {
            blackCapturedPieces.push_back(std::move(pieceTaken));
        }
    }

    // Display captured pieces
    if (move.piece && move.piece->getColor() == BLACK) {
        if (!whiteCapturedPieces.empty()) {
            std::string list = "Black has captured: ";
            for (size_t i = 0; i < whiteCapturedPieces.size(); ++i) {
                if (whiteCapturedPieces[i]) {
                    list += whiteCapturedPieces[i]->stringPieceType();
                    if (i < whiteCapturedPieces.size() - 1) {
                        list += ", ";
                    }
                }
            }
            Logger::log(LogLevel::INFO, list, __FILE__, __LINE__);
        }
    } else if (move.piece) {
        logCapturedPieces(WHITE);
    }

    // Handle castling using DRY helper
    if (move.castling) {
        if (move.isKingSide) castleRook(r1, 7, 5);
        else if (move.isQueenSide) castleRook(r1, 0, 3);
        if (pieceGrid[r2][c2] && pieceGrid[r2][c2]->getType() == KING) {
            static_cast<King*>(pieceGrid[r2][c2])->setIsCastlingEligible(false);
        }
    }
    // Handle Pawn Promotion via DRY helper
    handlePawnPromotion(move.piece, r2, c2);
}

void Board::applyMoveWithUndo(const Move& move, UndoMove& undo) {
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;

    Piece* movingPiece = pieceGrid[r1][c1];

    Color movingColor = movingPiece->getColor();

    long long localApply = 0;

    // Clear en passant flags
    auto t0 = high_resolution_clock::now();
    clearEnPassantFlags(movingColor);
    auto t1 = high_resolution_clock::now();
    long long dt = duration_cast<microseconds>(t1 - t0).count();
    g_muProfile.clearEnPassantFlags += dt;
    localApply += dt;

    undo.wasCastling = move.castling;
    undo.wasKingSide = move.isKingSide;
    undo.wasQueenSide = move.isQueenSide;
    undo.wasCapture = false;

    // ======================== START OF FIX ========================
    
    // Determine the piece to capture. This handles regular captures, en passant,
    // and cases where move.capturedPiece might not have been set.
    // Use a const-pointer here since Move stores const Piece* (we don't mutate
    // the captured piece through this temporary). Conversions from non-const
    // Piece* (from the grid) to const Piece* are allowed.
    const Piece* pieceToCapture = move.capturedPiece; // Prioritize explicit info (for en passant)
    if (!pieceToCapture && pieceGrid[r2][c2]) {
        // If no explicit capture, check destination square for an opponent's piece
        if (movingPiece->getColor() != pieceGrid[r2][c2]->getColor()) {
            pieceToCapture = pieceGrid[r2][c2];
        }
    }

    // Capture handling
    t0 = high_resolution_clock::now();
    if (pieceToCapture) {
        undo.wasCapture = true;
        // Record the logical position (best effort)
        undo.capturedPiecePos = pieceToCapture->getPosition();

        // Try to find the current live instance of the captured piece via its id
        Piece* currentCaptured = pieceManager->getPieceById(pieceToCapture->id);
        int capturedRow = -1, capturedCol = -1;

        if (currentCaptured) {
            capturedRow = currentCaptured->getPosition().first;
            capturedCol = currentCaptured->getPosition().second;
        } else {
            // Fallback: maybe the piece resides at the recorded position in the grid
            capturedRow = undo.capturedPiecePos.first;
            capturedCol = undo.capturedPiecePos.second;
            if (!(capturedRow >= 0 && capturedRow < 8 && capturedCol >= 0 && capturedCol < 8)) {
                // Give up if no sensible location
                Logger::log(LogLevel::ERROR, "Capture piece not found (no live instance) in applyMoveWithUndo", __FILE__, __LINE__);
                undo.wasCapture = false;
            } else {
                // Use whatever is on the grid (could be nullptr)
                currentCaptured = pieceGrid[capturedRow][capturedCol];
            }
        }

        if (undo.wasCapture && currentCaptured) {
            // LOG: about to remove captured piece from manager
            {
                std::ostringstream oss;
                oss << "applyMoveWithUndo: removing captured piece id=" << currentCaptured->id
                    << " type=" << currentCaptured->stringPieceType()
                    << " at (" << capturedRow << "," << capturedCol << ")";
                Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);
            }

            // Remove from manager and store the owned pointer in undo
            undo.capturedPiece = pieceManager->removePiece(currentCaptured->id);
            if (capturedRow >= 0 && capturedRow < 8 && capturedCol >= 0 && capturedCol < 8) {
                pieceGrid[capturedRow][capturedCol] = nullptr;
            }
        } else if (undo.wasCapture) {
            // We thought there was a capture, but couldn't locate the piece to remove
            Logger::log(LogLevel::ERROR, "Capture piece mismatch in applyMoveWithUndo (unable to locate current piece)", __FILE__, __LINE__);
            undo.wasCapture = false; // Don't try to restore if capture was invalid
        }
    }
    
    // ========================= END OF FIX =========================

    t1 = high_resolution_clock::now();
    dt = duration_cast<microseconds>(t1 - t0).count();
    g_muProfile.captureHandling += dt;
    localApply += dt;
    t1 = high_resolution_clock::now();
    dt = duration_cast<microseconds>(t1 - t0).count();
    g_muProfile.captureHandling += dt;
    localApply += dt;

    // Move piece
    t0 = high_resolution_clock::now();
    undo.movedPiecePrevHasMoved = movingPiece->getHasMoved();
    pieceGrid[r2][c2] = movingPiece;
    pieceGrid[r1][c1] = nullptr;
    // Update the moved piece's internal state
    movingPiece->setPosition(r2, c2);
    movingPiece->setHasMoved(true);
    
    t1 = high_resolution_clock::now();
    dt = duration_cast<microseconds>(t1 - t0).count();
    g_muProfile.movePiece += dt;
    localApply += dt;

    // Castling bookkeeping
    t0 = high_resolution_clock::now();
    if (move.castling) {
        undo.rookRow = r1;
        if (move.isKingSide) {
            undo.rookFromCol = 7; undo.rookToCol = 5;
        } else if (move.isQueenSide) {
            undo.rookFromCol = 0; undo.rookToCol = 3;
        }
        if (undo.rookFromCol != -1 && pieceGrid[undo.rookRow][undo.rookFromCol]) {
            undo.rookPrevHasMoved = pieceGrid[undo.rookRow][undo.rookFromCol]->getHasMoved();
        }
    }
    t1 = high_resolution_clock::now();
    dt = duration_cast<microseconds>(t1 - t0).count();
    g_muProfile.castlingBookkeeping += dt;
    localApply += dt;

    // Promotion handling
    t0 = high_resolution_clock::now();

    // If the Move explicitly indicates a promotion, handle it here so we can
    // create the promoted piece immediately and update caches correctly.
    if (move.isPromotion) {
        // Remove pawn from manager and store in undo
        undo.promotedPawn = pieceManager->removePiece(movingPiece->id);
        pieceGrid[r2][c2] = nullptr; // Remove pawn from board grid

        // Create the promoted piece and place it on the square
        std::unique_ptr<Piece> promotedPiece;
        Color promotionColor = undo.promotedPawn ? undo.promotedPawn->getColor() : WHITE;
        SDL_Renderer* renderer = undo.promotedPawn ? undo.promotedPawn->getRenderer() : nullptr;

        switch (move.promotionType) {
            case QUEEN:
                promotedPiece = std::make_unique<Queen>(promotionColor, QUEEN, renderer);
                break;
            case ROOK:
                promotedPiece = std::make_unique<Rook>(promotionColor, ROOK, renderer);
                break;
            case BISHOP:
                promotedPiece = std::make_unique<Bishop>(promotionColor, BISHOP, renderer);
                break;
            case KNIGHT:
                promotedPiece = std::make_unique<Knight>(promotionColor, KNIGHT, renderer);
                break;
            default:
                promotedPiece = std::make_unique<Queen>(promotionColor, QUEEN, renderer);
                break;
        }

        promotedPiece->setPosition(r2, c2);
        promotedPiece->setHasMoved(true);
        // Register the promoted piece with the PieceManager (transfer ownership)
        Piece* rawPromoted = promotedPiece.get();
        pieceManager->addPiece(std::move(promotedPiece));
        // Store the non-owning raw pointer in the board grid
        pieceGrid[r2][c2] = rawPromoted;
 
        // Mark undo info
        undo.wasPromotion = true;
        undo.originalPromotionType = move.promotionType;

    }
    // Skip the regular handlePawnPromotion call if move.isPromotion is true
    else {
        handlePawnPromotion(pieceGrid[r2][c2], r2, c2);
    }

    t1 = high_resolution_clock::now();
    dt = duration_cast<microseconds>(t1 - t0).count();
    g_muProfile.promotionHandling += dt;
    localApply += dt;

    g_muProfile.applyTime += localApply;
    g_muProfile.applyCalls++;

    lastMove = move; // Update last move for highlighting
}

void Board::unmakeMove(const Move& move, UndoMove& undo) {
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;

    // Capture a pointer to whatever was on the destination square *before* we change anything.
    // This is important to identify promoted pieces that were placed there.
    Piece* pieceOnEndSquare = pieceGrid[r2][c2];

    long long localUnmake = 0;

    // 1) Handle castling-specific undo (rook move restoration)
    if (undo.wasCastling) {
        auto t0 = high_resolution_clock::now();
        // For castling we still want to move the king back and restore rook manually
        undoPieceMove(r1, c1, r2, c2, undo.movedPiecePrevHasMoved);
        if (undo.rookToCol != -1 && pieceGrid[undo.rookRow][undo.rookToCol]) {
            pieceGrid[undo.rookRow][undo.rookFromCol] = pieceGrid[undo.rookRow][undo.rookToCol];
            pieceGrid[undo.rookRow][undo.rookToCol] = nullptr;
            if (pieceGrid[undo.rookRow][undo.rookFromCol]) {
                Rook* actualRook = dynamic_cast<Rook*>(pieceGrid[undo.rookRow][undo.rookFromCol]);
                if (actualRook) {
                    actualRook->setPosition(undo.rookRow, undo.rookFromCol);
                    actualRook->setHasMoved(undo.rookPrevHasMoved);
                    actualRook->setIsCastlingEligible(!undo.rookPrevHasMoved);
                }
            }
        }
        auto t1 = high_resolution_clock::now();
        long long dt = duration_cast<microseconds>(t1 - t0).count();
        g_muProfile.unmakeCastling += dt;
        localUnmake += dt;
    }
    // 2) Non-castling moves: move the piece back exactly once using the helper
    else {
        auto t0 = high_resolution_clock::now();
        undoPieceMove(r1, c1, r2, c2, undo.movedPiecePrevHasMoved);
        auto t1 = high_resolution_clock::now();
        long long dt = duration_cast<microseconds>(t1 - t0).count();
        g_muProfile.unmakeMoveBack += dt;
        localUnmake += dt;
    }

    // --- PROMOTION undo & capture restore (safer ordering) ---
    if (undo.wasPromotion) {
        // Store the removed promoted piece temporarily to prevent destruction
        std::unique_ptr<Piece> removedPromoted;
        
        // If there was a promoted piece originally placed on the destination square,
        // remove it from the board grid first (to avoid dangling pointers), then
        // remove ownership from the PieceManager.
        if (pieceOnEndSquare) {
            std::ostringstream oss;
            oss << "unmakeMove: removing promoted piece id=" << pieceOnEndSquare->id
                << " type=" << pieceOnEndSquare->stringPieceType()
                << " at (" << r2 << "," << c2 << ")";
            Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);

            // Clear grid before removing from manager to avoid dangling pointer.
            pieceGrid[r2][c2] = nullptr;

            // Remove from manager and store temporarily to prevent destruction.
            removedPromoted = pieceManager->removePiece(pieceOnEndSquare->id);
        }

        // Restore the original pawn back to the start square and re-register ownership
        if (undo.promotedPawn) {
            pieceGrid[r1][c1] = undo.promotedPawn.get();
            pieceManager->addPiece(std::move(undo.promotedPawn));

            if (pieceGrid[r1][c1]) {
                pieceGrid[r1][c1]->setPosition(r1, c1);
                pieceGrid[r1][c1]->setHasMoved(undo.movedPiecePrevHasMoved);
                updatePiecePositionInManager(pieceGrid[r1][c1]);
            }
        } else {
            // Defensive: if we don't have the pawn (shouldn't happen), make origin empty
            pieceGrid[r1][c1] = nullptr;
        }
        
        // Now let removedPromoted be destroyed safely (it goes out of scope here)
    } else {
        // If no promotion, destination square should already have been cleared by undoPieceMove.
        // Just ensure it's nullptr (defensive).
        pieceGrid[r2][c2] = nullptr;
    }

    // Restore a captured piece (if any). Do this last so we don't overwrite the restored mover/pawn.
    if (undo.wasCapture && undo.capturedPiece) {
        int capturedRow = undo.capturedPiecePos.first;
        int capturedCol = undo.capturedPiecePos.second;

        // Defensive: if something is unexpectedly in that square, log and clear it first.
        if (pieceGrid[capturedRow][capturedCol]) {
            std::ostringstream o2;
            o2 << "unmakeMove: overwriting non-null grid at captured square (" << capturedRow << "," << capturedCol << ")";
            Logger::log(LogLevel::WARN, o2.str(), __FILE__, __LINE__);
            pieceGrid[capturedRow][capturedCol] = nullptr;
        }

        // Verify the captured piece is still valid before restoring
        if (undo.capturedPiece) {
            // Put raw pointer into grid then transfer ownership back into the manager
            pieceGrid[capturedRow][capturedCol] = undo.capturedPiece.get();
            pieceManager->addPiece(std::move(undo.capturedPiece));
            if (pieceGrid[capturedRow][capturedCol]) {
                pieceGrid[capturedRow][capturedCol]->setPosition(capturedRow, capturedCol);
                updatePiecePositionInManager(pieceGrid[capturedRow][capturedCol]);
            }
        }
    }

    g_muProfile.unmakeTime += localUnmake;
    g_muProfile.unmakeCalls++;

    lastMove = Move(); // Clear last move; could be improved by restoring previous lastMove if needed
}



std::vector<Move> Board::getAllLegalMoves(Color color, bool generateCastlingMoves) const {
    std::vector<Move> allLegalMoves;
    allLegalMoves.reserve(256); // Reserve space to minimize reallocations

    // Get pieces of a specific color using the manager
    const std::vector<Piece*>& pieces = pieceManager->getPieces(color);

    for (Piece* piece : pieces) {
        if (!piece) continue;
        std::vector<Move> pieceMoves = piece->getPseudoLegalMoves(*this, generateCastlingMoves);
        allLegalMoves.insert(allLegalMoves.end(), pieceMoves.begin(), pieceMoves.end());
    }
    return allLegalMoves;
}

// Keep the original const entrypoint but forward to non-const implementation
bool Board::isKingInCheck(Color color) const {
    // forward to non-const overload (safe because we pass nullptr => no temporary mutation)
    return const_cast<Board*>(this)->isKingInCheck(color, nullptr);
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

// DRY: unified castling logic
void Board::castleRook(int row, int fromCol, int toCol) {
    Piece* fromP = pieceGrid[row][fromCol];
    if (!fromP) return;
    Rook* actualRook = dynamic_cast<Rook*>(fromP);
    if (actualRook) {
        actualRook->setHasMoved(true);
        actualRook->setIsCastlingEligible(false);
        actualRook->setPosition(row, toCol);
    }
    // Move the raw pointer on the non-owning grid
    pieceGrid[row][toCol] = fromP;
    pieceGrid[row][fromCol] = nullptr;
    // Notify manager/cache about new position
    updatePiecePositionInManager(pieceGrid[row][toCol]);
}
 
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
 
std::unique_ptr<Piece> Board::removePieceFromManagerById(unsigned int id) {
    return pieceManager->removePiece(id);
}

void Board::addPieceToManager(Piece* piece) {
    if (!piece) return;
    pieceManager->addPiece(std::unique_ptr<Piece>(piece));
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
 
// DRY: unified undo move of a piece
void Board::undoPieceMove(int r1, int c1, int r2, int c2, bool prevHasMoved) {
    // Move the raw pointer back and clear the source
    pieceGrid[r1][c1] = pieceGrid[r2][c2];
    pieceGrid[r2][c2] = nullptr;
    if (pieceGrid[r1][c1]) {
        pieceGrid[r1][c1]->setPosition(r1, c1);
        pieceGrid[r1][c1]->setHasMoved(prevHasMoved);
        updatePiecePositionInManager(pieceGrid[r1][c1]);
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
