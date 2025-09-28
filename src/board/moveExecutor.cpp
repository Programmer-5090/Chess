#include "board/moveExecutor.h"
#include "board.h"
#include "board/pieceManager.h"
#include "enums.h" // For PieceType, Color
#include "pieces/pieces.h"
#include "perfProfiler.h"


// RAII helper class for managing undo state
class UndoStateManager {
private:
    UndoMove& undo;

public:
    UndoStateManager(UndoMove& u) : undo(u) {}
    
    ~UndoStateManager() {
        // Automatic cleanup if needed
    }
};

// Move and UndoMove are defined in the header now

MoveExecutor::MoveExecutor(Board* board) : board(board) {}

// Return pointer to last move in history (or nullptr)
const Move* MoveExecutor::getLastMovePtr() const {
    if (moveHistory.empty()) return nullptr;
    return &moveHistory.back();
}

// Helper method to restore piece to manager and grid
void MoveExecutor::restorePieceToManager(std::unique_ptr<Piece> piece, int row, int col) {
    if (!piece) return;
    // Access managers via board
    auto pm = board->getPieceManager();
    // Ensure the piece has the correct position before re-adding it to the manager
    piece->setPosition(row, col);

    // Capture the piece id to look up the raw pointer after insertion
    PieceId id = piece->id;

    // Add back into the manager (takes ownership)
    g_profiler.startTimer("restore_pm_addPiece");
    pm->addPiece(std::move(piece));
    g_profiler.endTimer("restore_pm_addPiece");

    // Retrieve the raw pointer by id (robust even if addPiece adjusted id)
    g_profiler.startTimer("restore_pm_getById");
    Piece* restored = pm->getPieceById(id);
    g_profiler.endTimer("restore_pm_getById");
    if (!restored) {
        // If the id was changed by addPiece, try to find by matching position/type/color
        g_profiler.startTimer("restore_scan_all");
        const auto& all = pm->getAllPieces();
        for (Piece* p : all) {
            if (!p) continue;
            if (p->getPosition() == std::make_pair(row, col)) {
                restored = p;
                break;
            }
        }
        g_profiler.endTimer("restore_scan_all");
    }

    if (restored) {
        // Ensure board grid points at the restored piece and manager caches are consistent
        g_profiler.startTimer("restore_update_grid_and_manager");
        board->pieceGrid[row][col] = restored;
        restored->setPosition(row, col);
        board->updatePiecePositionInManager(restored);
        g_profiler.endTimer("restore_update_grid_and_manager");
    } else {
        Logger::log(LogLevel::WARN, "restorePieceToManager: failed to locate restored piece after add", __FILE__, __LINE__);
    }
}

// Helper method to remove and capture piece
std::unique_ptr<Piece> MoveExecutor::captureAndRemovePiece(const Piece* pieceToCapture, std::pair<int, int>& capturedPos) {
    if (!pieceToCapture) return nullptr;
    capturedPos = pieceToCapture->getPosition();
    auto pm = board->getPieceManager();
    // Legacy behavior: try manager lookup by id first
    Piece* currentCaptured = pm->getPieceById(pieceToCapture->id);
    int capturedRow = -1, capturedCol = -1;

    if (currentCaptured) {
        capturedRow = currentCaptured->getPosition().first;
        capturedCol = currentCaptured->getPosition().second;
    } else {
        // Fallback: check the board grid for pointer-equality with the Move's capturedPiece
        capturedRow = capturedPos.first;
        capturedCol = capturedPos.second;
        if (capturedRow >= 0 && capturedRow < 8 && capturedCol >= 0 && capturedCol < 8) {
            if (board->pieceGrid[capturedRow][capturedCol] == pieceToCapture) {
                currentCaptured = board->pieceGrid[capturedRow][capturedCol];
            }
        }
    }

    if (!currentCaptured) {
        // Last resort: try to find a matching piece by position/type/color in manager's list
        const auto& all = pm->getAllPieces();
        for (Piece* p : all) {
            if (!p) continue;
            if (p->getPosition() == std::make_pair(capturedRow, capturedCol)
                && p->getType() == pieceToCapture->getType()
                && p->getColor() == pieceToCapture->getColor()) {
                currentCaptured = p;
                capturedRow = p->getPosition().first;
                capturedCol = p->getPosition().second;
                break;
            }
        }
    }

    if (!currentCaptured) {
        Logger::log(LogLevel::WARN, "Capture piece not found in captureAndRemovePiece; treating as no-capture", __FILE__, __LINE__);
        return nullptr;
    }
    std::ostringstream oss;
    oss << "executeMove: capturing piece id=" << currentCaptured->id
        << " type=" << currentCaptured->stringPieceType()
        << " at (" << capturedRow << "," << capturedCol << ")";
    Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);

    // Prefer to remove by id from the manager if possible
    std::unique_ptr<Piece> capturedPiece = nullptr;

    g_profiler.startTimer("capture_lookup_pm_hasId");
    bool pmHas = pm->getPieceById(currentCaptured->id) != nullptr;
    g_profiler.endTimer("capture_lookup_pm_hasId");

    if (pmHas) {
        g_profiler.startTimer("capture_remove_by_id");
        capturedPiece = pm->removePiece(currentCaptured->id);
        g_profiler.endTimer("capture_remove_by_id");
    } else {
        // If the manager doesn't have it but the grid has the pointer, attempt to match by position/type/color
        g_profiler.startTimer("capture_lookup_scan_all");
        const auto& all = pm->getAllPieces();
        PieceId foundId = 0;
        bool found = false;
        for (Piece* p : all) {
            if (!p) continue;
            if (p->getPosition() == std::make_pair(capturedRow, capturedCol)
                && p->getType() == currentCaptured->getType()
                && p->getColor() == currentCaptured->getColor()) {
                foundId = p->id;
                found = true;
                break;
            }
        }
        g_profiler.endTimer("capture_lookup_scan_all");

        if (found) {
            g_profiler.startTimer("capture_remove_foundId");
            capturedPiece = pm->removePiece(foundId);
            g_profiler.endTimer("capture_remove_foundId");
        } else {
            Logger::log(LogLevel::WARN, "Captured piece not present in PieceManager; clearing grid slot", __FILE__, __LINE__);
        }
    }

    g_profiler.startTimer("capture_clear_grid_slot");
    if (capturedRow >= 0 && capturedRow < 8 && capturedCol >= 0 && capturedCol < 8) {
        board->pieceGrid[capturedRow][capturedCol] = nullptr;
    }
    g_profiler.endTimer("capture_clear_grid_slot");

    return capturedPiece;
}

// Helper method to handle castling rook movement
void MoveExecutor::executeCastlingRookMove(int kingRow, CastlingType castlingType, UndoMove& undo) {
    undo.rookRow = kingRow;
    
    if (castlingType == CastlingType::KING_SIDE) {
        undo.rookFromCol = 7;
        undo.rookToCol = 5;
    } else if (castlingType == CastlingType::QUEEN_SIDE) {
        undo.rookFromCol = 0;
        undo.rookToCol = 3;
    }
    
    if (undo.rookFromCol != -1 && board->pieceGrid[undo.rookRow][undo.rookFromCol]) {
        undo.rookPrevHasMoved = board->pieceGrid[undo.rookRow][undo.rookFromCol]->getHasMoved();

        // Move the rook
        board->pieceGrid[undo.rookRow][undo.rookToCol] = board->pieceGrid[undo.rookRow][undo.rookFromCol];
        board->pieceGrid[undo.rookRow][undo.rookFromCol] = nullptr;
        board->pieceGrid[undo.rookRow][undo.rookToCol]->setPosition(undo.rookRow, undo.rookToCol);
        board->pieceGrid[undo.rookRow][undo.rookToCol]->setHasMoved(true);
    }
}

// Helper method to create promoted piece
std::unique_ptr<Piece> MoveExecutor::createPromotedPiece(PieceType promotionType, Color color, SDL_Renderer* renderer) {
    g_profiler.startTimer("move_exec_createPromotedPiece");
    switch (promotionType) {
        case QUEEN: {
            g_profiler.startTimer("piece_ctor_Queen");
            auto newQueen = std::make_unique<Queen>(color, QUEEN, renderer);
            g_profiler.endTimer("piece_ctor_Queen");
            g_profiler.endTimer("move_exec_createPromotedPiece");
            return newQueen;
        }
        case ROOK: {
            g_profiler.startTimer("piece_ctor_Rook");
            auto newRook = std::make_unique<Rook>(color, ROOK, renderer);
            g_profiler.endTimer("piece_ctor_Rook");
            g_profiler.endTimer("move_exec_createPromotedPiece");
            return newRook;
        }
        case BISHOP: {
            g_profiler.startTimer("piece_ctor_Bishop");
            auto newBishop = std::make_unique<Bishop>(color, BISHOP, renderer);
            g_profiler.endTimer("piece_ctor_Bishop");
            g_profiler.endTimer("move_exec_createPromotedPiece");
            return newBishop;
        }
        case KNIGHT: {
            g_profiler.startTimer("piece_ctor_Knight");
            auto newKnight = std::make_unique<Knight>(color, KNIGHT, renderer);
            g_profiler.endTimer("piece_ctor_Knight");
            g_profiler.endTimer("move_exec_createPromotedPiece");
            return newKnight;
        }
        default: {
            g_profiler.startTimer("piece_ctor_Queen");
            auto defaultQueen = std::make_unique<Queen>(color, QUEEN, renderer);
            g_profiler.endTimer("piece_ctor_Queen");
            g_profiler.endTimer("move_exec_createPromotedPiece");
            return defaultQueen;
        }
    }
}

UndoMove MoveExecutor::executeMove(const Move& move, bool trackUndo) {
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;
    
    UndoMove undo;
    Piece* movingPiece = nullptr;
    if (r1 >= 0 && r1 < 8 && c1 >= 0 && c1 < 8) {
        movingPiece = board->pieceGrid[r1][c1];
    }
    if (!movingPiece) {
        // Attempt to recover: maybe PieceManager has the piece but grid wasn't updated
        PieceManager* pm = board->getPieceManager();
        const auto& all = pm->getAllPieces();
        for (Piece* p : all) {
            if (!p) continue;
            if (p->getPosition() == std::make_pair(r1, c1)) {
                movingPiece = p;
                // place pointer into grid to keep behavior consistent
                board->pieceGrid[r1][c1] = movingPiece;
                Logger::log(LogLevel::WARN, "executeMove: recovered movingPiece from PieceManager for start square", __FILE__, __LINE__);
                break;
            }
        }
    }
    if (!movingPiece) {
        std::ostringstream oss;
        oss << "executeMove: start square empty or out-of-range; aborting move at (" << r1 << "," << c1 << ")";
        if (move.piece) oss << " move.piece id=" << move.piece->id;
        Logger::log(LogLevel::ERROR, oss.str(), __FILE__, __LINE__);
        // Return empty undo to indicate nothing changed
        return undo;
    }
    Color movingColor = movingPiece->getColor();
    
    
    // Clear en passant flags
    board->clearEnPassantFlags(movingColor);

    undo.castlingType = move.castlingType;
    
    // Determine and handle capture using authoritative board/manager state only.
    int capturedRow = -1, capturedCol = -1;
    // En-passant detection: pawn moves diagonally into empty destination
    if (movingPiece->getType() == PAWN && c1 != c2 && board->pieceGrid[r2][c2] == nullptr) {
        capturedRow = r1; // captured pawn sits on the moving pawn's source row
        capturedCol = c2;
    } else {
        // Normal capture (destination square)
        capturedRow = r2;
        capturedCol = c2;
    }

    undo.capturedPiecePos = {capturedRow, capturedCol};
    PieceManager* pm = board->getPieceManager();
    Piece* currentCaptured = nullptr;

    if (capturedRow >= 0 && capturedRow < 8 && capturedCol >= 0 && capturedCol < 8) {
        currentCaptured = board->pieceGrid[capturedRow][capturedCol];
        // If grid cell empty, try manager lookup by position
        if (!currentCaptured) {
            const auto& all = pm->getAllPieces();
            for (Piece* p : all) {
                if (!p) continue;
                if (p->getPosition() == std::make_pair(capturedRow, capturedCol)
                    && p->getColor() != movingColor) {
                    currentCaptured = p;
                    break;
                }
            }
        }
    }

    if (currentCaptured) {
        g_profiler.startTimer("move_exec_capture");
        undo.wasCapture = true;
        std::ostringstream oss;
        oss << "executeMove: capturing piece id=" << currentCaptured->id
            << " type=" << currentCaptured->stringPieceType()
            << " at (" << capturedRow << "," << capturedCol << ")";
        Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);

        // Remove from PieceManager by id
        if (pm->getPieceById(currentCaptured->id)) {
            undo.capturedPiece = pm->removePiece(currentCaptured->id);
        } else {
            // Unexpected: manager doesn't have the piece, but we at least clear grid
            Logger::log(LogLevel::WARN, "Captured piece present on grid but missing from PieceManager; clearing grid slot", __FILE__, __LINE__);
        }

        if (capturedRow >= 0 && capturedRow < 8 && capturedCol >= 0 && capturedCol < 8) {
            board->pieceGrid[capturedRow][capturedCol] = nullptr;
        }
        g_profiler.endTimer("move_exec_capture");
    } else {
        // No capture found; leave undo.wasCapture == false
    }

     // update captured piece lists
    // if (undo.wasCapture && undo.capturedPiece) {
    //     if (movingColor == BLACK) {
    //         board->addCapturedPiece(WHITE, std::move(undo.capturedPiece));
    //     } else {
    //         board->addCapturedPiece(BLACK, std::move(undo.capturedPiece));
    //     }
    // }

    // // Display captured pieces
    // if (move.piece && move.piece->getColor() == BLACK) {
    //     if (!board->getCapturedPieces(WHITE).empty()) {
    //         std::string list = "Black has captured: ";
    //         for (size_t i = 0; i < board->getCapturedPieces(WHITE).size(); ++i) {
    //             if (board->getCapturedPieces(WHITE)[i]) {
    //                 list += board->getCapturedPieces(WHITE)[i]->stringPieceType();
    //                 if (i < board->getCapturedPieces(WHITE).size() - 1) {
    //                     list += ", ";
    //                 }
    //             }
    //         }
    //         Logger::log(LogLevel::INFO, list, __FILE__, __LINE__);
    //     }
    // } else if (move.piece) {
    //     board->logCapturedPieces(WHITE);
    // }

    
    // Move piece
    undo.movedPiecePrevHasMoved = movingPiece->getHasMoved();
    g_profiler.startTimer("move_exec_grid_update");
    board->pieceGrid[r2][c2] = movingPiece;
    board->pieceGrid[r1][c1] = nullptr;
    movingPiece->setPosition(r2, c2);
    movingPiece->setHasMoved(true);
    // Reflect position change in manager (may be a no-op if caches are consistent)
    board->updatePiecePositionInManager(movingPiece);
    g_profiler.endTimer("move_exec_grid_update");

    
    // Handle castling
    if (move.isCastling()) {
        g_profiler.startTimer("move_exec_castling");
        executeCastlingRookMove(r1, move.castlingType, undo);
        g_profiler.endTimer("move_exec_castling");
    }
    
    // Handle promotion
    if (move.isPromotion) {
        g_profiler.startTimer("move_exec_promotion");
        undo.wasPromotion = true;
        undo.originalPromotionType = move.promotionType;
        // Remove pawn from manager and store in undo
        undo.promotedPawn = board->getPieceManager()->removePiece(movingPiece->id);
        board->pieceGrid[r2][c2] = nullptr;

        // Create and place promoted piece
        Color promotionColor = undo.promotedPawn->getColor();
        SDL_Renderer* renderer = undo.promotedPawn->getRenderer();

        auto promotedPiece = createPromotedPiece(move.promotionType, promotionColor, renderer);
        promotedPiece->setPosition(r2, c2);
        promotedPiece->setHasMoved(true);

        Piece* rawPromoted = promotedPiece.get();
        board->getPieceManager()->addPiece(std::move(promotedPiece));
        board->pieceGrid[r2][c2] = rawPromoted;
        g_profiler.endTimer("move_exec_promotion");
    } else {
        g_profiler.startTimer("move_exec_promotion_check");
        board->handlePawnPromotion(board->pieceGrid[r2][c2], r2, c2);
        g_profiler.endTimer("move_exec_promotion_check");
    }
    
    g_profiler.startTimer("move_exec_history_push");
    moveHistory.push_back(move);
    g_profiler.endTimer("move_exec_history_push");
    if (trackUndo) {
        return undo;
    }
    else {
        UndoMove emptyUndo;
        return emptyUndo;
    }
}

void MoveExecutor::undoMove(const Move& move, UndoMove& undo) {
    g_profiler.startTimer("undo_move_total");
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;
    
    Piece* pieceOnEndSquare = board->pieceGrid[r2][c2];
    
    long long localUnmake = 0;
    
    // Handle castling-specific undo
    if (undo.castlingType != CastlingType::NONE) {
        g_profiler.startTimer("undo_exec_piece_move");
        undoPieceMove(r1, c1, r2, c2, undo.movedPiecePrevHasMoved);
        g_profiler.endTimer("undo_exec_piece_move");

        // Restore rook
        g_profiler.startTimer("undo_exec_castling_restore_rook");
        if (undo.rookToCol != -1 && board->pieceGrid[undo.rookRow][undo.rookToCol]) {
            board->pieceGrid[undo.rookRow][undo.rookFromCol] = board->pieceGrid[undo.rookRow][undo.rookToCol];
            board->pieceGrid[undo.rookRow][undo.rookToCol] = nullptr;

            Rook* rook = dynamic_cast<Rook*>(board->pieceGrid[undo.rookRow][undo.rookFromCol]);
            if (rook) {
                rook->setPosition(undo.rookRow, undo.rookFromCol);
                rook->setHasMoved(undo.rookPrevHasMoved);
                rook->setIsCastlingEligible(!undo.rookPrevHasMoved);
            }
        }
        g_profiler.endTimer("undo_exec_castling_restore_rook");
    } else {
        g_profiler.startTimer("undo_exec_piece_move");
        undoPieceMove(r1, c1, r2, c2, undo.movedPiecePrevHasMoved);
        g_profiler.endTimer("undo_exec_piece_move");
    }
    
    // Handle promotion undo
    if (undo.wasPromotion) {
        g_profiler.startTimer("undo_exec_remove_promoted");
        // Remove promoted piece
        if (pieceOnEndSquare) {
            std::ostringstream oss;
            oss << "undoMove: removing promoted piece id=" << pieceOnEndSquare->id
                << " type=" << pieceOnEndSquare->stringPieceType()
                << " at (" << r2 << "," << c2 << ")";
            Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);
            
            board->pieceGrid[r2][c2] = nullptr;
            auto removedPromoted = board->getPieceManager()->removePiece(pieceOnEndSquare->id);
        }
        g_profiler.endTimer("undo_exec_remove_promoted");

        // Restore original pawn
        if (undo.promotedPawn) {
            g_profiler.startTimer("undo_exec_restore_promoted_pawn");
            restorePieceToManager(std::move(undo.promotedPawn), r1, c1);
            board->pieceGrid[r1][c1]->setHasMoved(undo.movedPiecePrevHasMoved);
            g_profiler.endTimer("undo_exec_restore_promoted_pawn");
        }
    } else {
        g_profiler.startTimer("undo_exec_clear_end_square");
        board->pieceGrid[r2][c2] = nullptr;
        g_profiler.endTimer("undo_exec_clear_end_square");
    }
    
    // Restore captured piece
    if (undo.wasCapture && undo.capturedPiece) {
        g_profiler.startTimer("undo_exec_restore_capture");
        int capturedRow = undo.capturedPiecePos.first;
        int capturedCol = undo.capturedPiecePos.second;
        restorePieceToManager(std::move(undo.capturedPiece), capturedRow, capturedCol);
        g_profiler.endTimer("undo_exec_restore_capture");
    }
    ;
    
    // Fix: Remove the last move from history instead of adding it
    if (!moveHistory.empty()) {
        moveHistory.pop_back();
    }
    g_profiler.endTimer("undo_move_total");
}

void MoveExecutor::undoPieceMove(int r1, int c1, int r2, int c2, bool prevHasMoved) {
    board->pieceGrid[r1][c1] = board->pieceGrid[r2][c2];
    board->pieceGrid[r2][c2] = nullptr;

    if (board->pieceGrid[r1][c1]) {
        board->pieceGrid[r1][c1]->setPosition(r1, c1);
        board->pieceGrid[r1][c1]->setHasMoved(prevHasMoved);
        board->updatePiecePositionInManager(board->pieceGrid[r1][c1]);
    }
}