#include <chess/board/move_executor.h>
#include <chess/board/board.h>
#include <chess/board/piece_manager.h>
#include <chess/enums.h>
#include <chess/board/pieces/pieces.h>
#include <chess/utils/logger.h>
#include <chess/utils/profiler.h>


// RAII undo state helper
class UndoStateManager {
private:
    UndoMove& undo;

public:
    UndoStateManager(UndoMove& u) : undo(u) {}
    ~UndoStateManager() {}
};

MoveExecutor::MoveExecutor(Board* board) : board(board) {}

const Move* MoveExecutor::getLastMovePtr() const {
    if (moveHistory.empty()) return nullptr;
    return &moveHistory.back();
}

void MoveExecutor::restorePieceToManager(std::unique_ptr<Piece> piece, int row, int col) {
    if (!piece) return;
    auto pm = board->getPieceManager();
    piece->setPosition(row, col);

    PieceId id = piece->id;

    g_profiler.startTimer("restore_pm_addPiece");
    pm->addPiece(std::move(piece));
    g_profiler.endTimer("restore_pm_addPiece");

    // Retrieve piece by ID (handles ID changes from addPiece)
    g_profiler.startTimer("restore_pm_getById");
    Piece* restored = pm->getPieceById(id);
    g_profiler.endTimer("restore_pm_getById");
    if (!restored) {
        // Fallback: find by position if ID changed
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
        g_profiler.startTimer("restore_update_grid_and_manager");
        board->pieceGrid[row][col] = restored;
        restored->setPosition(row, col);
        board->updatePiecePositionInManager(restored);
        g_profiler.endTimer("restore_update_grid_and_manager");
    } else {
        Logger::log(LogLevel::WARN, "restorePieceToManager: failed to locate restored piece after add", __FILE__, __LINE__);
    }
}

std::unique_ptr<Piece> MoveExecutor::captureAndRemovePiece(const Piece* pieceToCapture, std::pair<int, int>& capturedPos) {
    if (!pieceToCapture) return nullptr;
    capturedPos = pieceToCapture->getPosition();
    auto pm = board->getPieceManager();
    
    // Locate piece to capture using multiple fallback strategies for robustness
    Piece* currentCaptured = pm->getPieceById(pieceToCapture->id);
    int capturedRow = -1, capturedCol = -1;

    if (currentCaptured) {
        capturedRow = currentCaptured->getPosition().first;
        capturedCol = currentCaptured->getPosition().second;
    } else {
        // Fallback: check grid for pointer match
        capturedRow = capturedPos.first;
        capturedCol = capturedPos.second;
        if (capturedRow >= 0 && capturedRow < 8 && capturedCol >= 0 && capturedCol < 8) {
            if (board->pieceGrid[capturedRow][capturedCol] == pieceToCapture) {
                currentCaptured = board->pieceGrid[capturedRow][capturedCol];
            }
        }
    }

    if (!currentCaptured) {
        // Last resort: match by position/type/color
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

    std::unique_ptr<Piece> capturedPiece = nullptr;

    g_profiler.startTimer("capture_lookup_pm_hasId");
    bool pmHas = pm->getPieceById(currentCaptured->id) != nullptr;
    g_profiler.endTimer("capture_lookup_pm_hasId");

    if (pmHas) {
        g_profiler.startTimer("capture_remove_by_id");
        capturedPiece = pm->removePiece(currentCaptured->id);
        g_profiler.endTimer("capture_remove_by_id");
    } else {
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

        board->pieceGrid[undo.rookRow][undo.rookToCol] = board->pieceGrid[undo.rookRow][undo.rookFromCol];
        board->pieceGrid[undo.rookRow][undo.rookFromCol] = nullptr;
        board->pieceGrid[undo.rookRow][undo.rookToCol]->setPosition(undo.rookRow, undo.rookToCol);
        board->pieceGrid[undo.rookRow][undo.rookToCol]->setHasMoved(true);
    }
}

// Create promoted piece
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
        // Recovery attempt: sync discrepancy between grid and piece manager
        PieceManager* pm = board->getPieceManager();
        const auto& all = pm->getAllPieces();
        for (Piece* p : all) {
            if (!p) continue;
            if (p->getPosition() == std::make_pair(r1, c1)) {
                movingPiece = p;
                board->pieceGrid[r1][c1] = movingPiece;  // Sync grid with manager
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
    
    
    // Reset en passant eligibility for all pawns of moving color (only one pawn can be capturable per turn)
    board->clearEnPassantFlags(movingColor);

    undo.castlingType = move.castlingType;
    
    // Determine capture type and location using current board state
    int capturedRow = -1, capturedCol = -1;
    
    // En passant capture: pawn moves diagonally to empty square, captures pawn on same rank
    if (movingPiece->getType() == PAWN && c1 != c2 && board->pieceGrid[r2][c2] == nullptr) {
        capturedRow = r1; // Captured pawn is on moving pawn's starting rank
        capturedCol = c2; // Same file as destination
    } else {
        // Normal capture: piece on destination square
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
    
    // Store king's previous castling eligibility if it's a king
    if (movingPiece->getType() == KING) {
        const King* king = static_cast<const King*>(movingPiece);
        undo.kingPrevCastlingEligible = king->getIsCastlingEligible();
    }
    
    g_profiler.startTimer("move_exec_grid_update");
    board->pieceGrid[r2][c2] = movingPiece;
    board->pieceGrid[r1][c1] = nullptr;
    movingPiece->setPosition(r2, c2);
    movingPiece->setHasMoved(true);
    
    // Disable king's castling eligibility when king moves
    if (movingPiece->getType() == KING) {
        King* king = static_cast<King*>(movingPiece);
        king->setIsCastlingEligible(false);
    }
    
    // Mark pawn as capturable via en passant if it just moved two squares
    if (movingPiece->getType() == PAWN && abs(r1 - r2) == 2) {
        static_cast<Pawn*>(movingPiece)->setEnPassantCaptureEligible(true);
    }
    
    // Reflect position change in manager (may be a no-op if caches are consistent)
    board->updatePiecePositionInManager(movingPiece);
    g_profiler.endTimer("move_exec_grid_update");

    
    // Handle castling
    if (move.isCastling()) {
        g_profiler.startTimer("move_exec_castling");
        executeCastlingRookMove(r1, move.castlingType, undo);
        g_profiler.endTimer("move_exec_castling");
    }
    
    // Handle pawn promotion: replace pawn with chosen piece type
    if (move.isPromotion) {
        g_profiler.startTimer("move_exec_promotion");
        undo.wasPromotion = true;
        undo.originalPromotionType = move.promotionType;
        
        // Remove pawn and save for undo
        undo.promotedPawn = board->getPieceManager()->removePiece(movingPiece->id);
        board->pieceGrid[r2][c2] = nullptr;

        // Create promoted piece and place on board
        Color promotionColor = undo.promotedPawn->getColor();
        SDL_Renderer* renderer = undo.promotedPawn->getRenderer();

        auto promotedPiece = createPromotedPiece(move.promotionType, promotionColor, renderer);
        promotedPiece->setPosition(r2, c2);
        promotedPiece->setHasMoved(true);

        Piece* rawPromoted = promotedPiece.get();
        board->getPieceManager()->addPiece(std::move(promotedPiece));
        board->pieceGrid[r2][c2] = rawPromoted;
        g_profiler.endTimer("move_exec_promotion");
    }
    // Note: Removed automatic promotion check - all promotions should be explicit in perft
    
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
    
    Piece* pieceOnEndSquare = board->pieceGrid[r2][c2];  // Piece currently on destination square
    
    long long localUnmake = 0;
    
    // Restore piece to original position (castling requires additional rook restoration)
    if (undo.castlingType != CastlingType::NONE) {
        g_profiler.startTimer("undo_exec_piece_move");
        undoPieceMove(r1, c1, r2, c2, undo.movedPiecePrevHasMoved, undo.kingPrevCastlingEligible);
        g_profiler.endTimer("undo_exec_piece_move");

        // Restore rook to original castling position
        g_profiler.startTimer("undo_exec_castling_restore_rook");
        if (undo.rookToCol != -1 && board->pieceGrid[undo.rookRow][undo.rookToCol]) {
            board->pieceGrid[undo.rookRow][undo.rookFromCol] = board->pieceGrid[undo.rookRow][undo.rookToCol];
            board->pieceGrid[undo.rookRow][undo.rookToCol] = nullptr;

            Rook* rook = dynamic_cast<Rook*>(board->pieceGrid[undo.rookRow][undo.rookFromCol]);
            if (rook) {
                rook->setPosition(undo.rookRow, undo.rookFromCol);
                rook->setHasMoved(undo.rookPrevHasMoved);
                rook->setIsCastlingEligible(!undo.rookPrevHasMoved);  // Restore castling rights
            }
        }
        g_profiler.endTimer("undo_exec_castling_restore_rook");
    } else {
        g_profiler.startTimer("undo_exec_piece_move");
        undoPieceMove(r1, c1, r2, c2, undo.movedPiecePrevHasMoved, undo.kingPrevCastlingEligible);
        g_profiler.endTimer("undo_exec_piece_move");
    }
    
    // Reverse promotion: remove promoted piece and restore original pawn
    if (undo.wasPromotion) {
        g_profiler.startTimer("undo_exec_remove_promoted");
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

        // Restore the original pawn to its starting position
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

void MoveExecutor::undoPieceMove(int r1, int c1, int r2, int c2, bool prevHasMoved, bool kingPrevCastlingEligible) {
    board->pieceGrid[r1][c1] = board->pieceGrid[r2][c2];
    board->pieceGrid[r2][c2] = nullptr;

    if (board->pieceGrid[r1][c1]) {
        board->pieceGrid[r1][c1]->setPosition(r1, c1);
        board->pieceGrid[r1][c1]->setHasMoved(prevHasMoved);
        
        // Restore king's castling eligibility if it's a king
        if (board->pieceGrid[r1][c1]->getType() == KING) {
            King* king = static_cast<King*>(board->pieceGrid[r1][c1]);
            king->setIsCastlingEligible(kingPrevCastlingEligible);
        }
        
        board->updatePiecePositionInManager(board->pieceGrid[r1][c1]);
    }
}