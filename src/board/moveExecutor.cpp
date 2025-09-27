#include "moveExecutor.h"
#include "board.h"
#include "enums.h" // For PieceType, Color
#include "pieces/pieces.h"

// Move structure
struct Move {
    std::pair<int, int> startPos;
    std::pair<int, int> endPos;
    const Piece* piece;
    const Piece* capturedPiece;
    CastlingType castlingType;
    bool isPromotion;
    PieceType promotionType;

    // Constructor for convenience
    Move(std::pair<int, int> start,
         std::pair<int, int> end,
         const Piece* movedPiece,
         const Piece* takenPiece = nullptr,
         CastlingType castling = CastlingType::NONE,
         bool isPromotion_ = false,
         PieceType promotionType_ = QUEEN)
            : startPos(start), endPos(end), piece(movedPiece), capturedPiece(takenPiece),
              castlingType(castling), isPromotion(isPromotion_), promotionType(promotionType_) {}

    // Default constructor
    Move() : startPos({-1, -1}), endPos({-1, -1}), piece(nullptr), capturedPiece(nullptr),
             castlingType(CastlingType::NONE), isPromotion(false), promotionType(QUEEN) {}

    bool isCastling() const { return castlingType != CastlingType::NONE; }
};

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

// Reversible state for unmaking a move
struct UndoMove {
    bool movedPiecePrevHasMoved = false;
    bool rookPrevHasMoved = false;
    bool wasCapture = false;
    CastlingType castlingType = CastlingType::NONE;
    bool wasEnPassant = false;
    bool wasPromotion = false;
    PieceType originalPromotionType = PAWN;
    std::unique_ptr<Piece> promotedPawn;
    std::pair<int, int> capturedPiecePos;
    std::unique_ptr<Piece> capturedPiece;
    
    // Castling state
    int rookRow = -1;
    int rookFromCol = -1;
    int rookToCol = -1;
};

MoveExecutor::MoveExecutor(Board* board) : board(board) {}

// Helper method to restore piece to manager and grid
void MoveExecutor::restorePieceToManager(std::unique_ptr<Piece> piece, int row, int col) {
    if (!piece) return;
    
    pieceGrid[row][col] = piece.get();
    piece->setPosition(row, col);
    pieceManager->addPiece(std::move(piece));
    board->updatePiecePositionInManager(pieceGrid[row][col]);
}

// Helper method to remove and capture piece
std::unique_ptr<Piece> MoveExecutor::captureAndRemovePiece(const Piece* pieceToCapture, std::pair<int, int>& capturedPos) {
    if (!pieceToCapture) return nullptr;
    
    capturedPos = pieceToCapture->getPosition();
    Piece* currentCaptured = pieceManager->getPieceById(pieceToCapture->id);
    
    if (!currentCaptured) {
        Logger::log(LogLevel::ERROR, "Capture piece not found in captureAndRemovePiece", __FILE__, __LINE__);
        return nullptr;
    }
    
    int capturedRow = currentCaptured->getPosition().first;
    int capturedCol = currentCaptured->getPosition().second;
    
    std::ostringstream oss;
    oss << "executeMove: capturing piece id=" << currentCaptured->id
        << " type=" << currentCaptured->stringPieceType()
        << " at (" << capturedRow << "," << capturedCol << ")";
    Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);
    
    auto capturedPiece = pieceManager->removePiece(currentCaptured->id);
    pieceGrid[capturedRow][capturedCol] = nullptr;
    
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
    
    if (undo.rookFromCol != -1 && pieceGrid[undo.rookRow][undo.rookFromCol]) {
        undo.rookPrevHasMoved = pieceGrid[undo.rookRow][undo.rookFromCol]->getHasMoved();
        
        // Move the rook
        pieceGrid[undo.rookRow][undo.rookToCol] = pieceGrid[undo.rookRow][undo.rookFromCol];
        pieceGrid[undo.rookRow][undo.rookFromCol] = nullptr;
        pieceGrid[undo.rookRow][undo.rookToCol]->setPosition(undo.rookRow, undo.rookToCol);
        pieceGrid[undo.rookRow][undo.rookToCol]->setHasMoved(true);
    }
}

// Helper method to create promoted piece
std::unique_ptr<Piece> MoveExecutor::createPromotedPiece(PieceType promotionType, Color color, SDL_Renderer* renderer) {
    switch (promotionType) {
        case QUEEN:
            return std::make_unique<Queen>(color, QUEEN, renderer);
        case ROOK:
            return std::make_unique<Rook>(color, ROOK, renderer);
        case BISHOP:
            return std::make_unique<Bishop>(color, BISHOP, renderer);
        case KNIGHT:
            return std::make_unique<Knight>(color, KNIGHT, renderer);
        default:
            return std::make_unique<Queen>(color, QUEEN, renderer);
    }
}

UndoMove MoveExecutor::executeMove(const Move& move, bool trackUndo) {
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;
    
    UndoMove undo;
    Piece* movingPiece = pieceGrid[r1][c1];
    Color movingColor = movingPiece->getColor();
    
    long long localApply = 0;
    
    // Clear en passant flags
    // auto t0 = high_resolution_clock::now();
    board->clearEnPassantFlags(movingColor);
    // auto t1 = high_resolution_clock::now();
    // long long dt = duration_cast<microseconds>(t1 - t0).count();
    // g_muProfile.clearEnPassantFlags += dt;
    // localApply += dt;
    
    undo.castlingType = move.castlingType;
    
    // Determine and handle capture
    // auto t0 = high_resolution_clock::now();
    const Piece* pieceToCapture = move.capturedPiece;
    if (!pieceToCapture && pieceGrid[r2][c2]) {
        if (movingPiece->getColor() != pieceGrid[r2][c2]->getColor()) {
            pieceToCapture = pieceGrid[r2][c2];
        }
    }
    
    if (pieceToCapture) {
        undo.wasCapture = true;
        undo.capturedPiece = captureAndRemovePiece(pieceToCapture, undo.capturedPiecePos);
    }
    // auto t1 = high_resolution_clock::now();
    // long long dt = duration_cast<microseconds>(t1 - t0).count();
    // g_muProfile.captureHandling += dt;
    // localApply += dt;

     // update captured piece lists
    if (undo.wasCapture && undo.capturedPiece) {
        if (movingColor == BLACK) {
                board->addCapturedPiece(WHITE, std::move(undo.capturedPiece));
        } else {
            board->addCapturedPiece(BLACK, std::move(undo.capturedPiece));
        }
    }

    // Display captured pieces
    if (move.piece && move.piece->getColor() == BLACK) {
        if (!board->getCapturedPieces(WHITE).empty()) {
            std::string list = "Black has captured: ";
            for (size_t i = 0; i < board->getCapturedPieces(WHITE).size(); ++i) {
                if (board->getCapturedPieces(WHITE)[i]) {
                    list += board->getCapturedPieces(WHITE)[i]->stringPieceType();
                    if (i < board->getCapturedPieces(WHITE).size() - 1) {
                        list += ", ";
                    }
                }
            }
            Logger::log(LogLevel::INFO, list, __FILE__, __LINE__);
        }
    } else if (move.piece) {
        board->logCapturedPieces(WHITE);
    }

    
    // Move piece
    // auto t0 = high_resolution_clock::now();
    undo.movedPiecePrevHasMoved = movingPiece->getHasMoved();
    pieceGrid[r2][c2] = movingPiece;
    pieceGrid[r1][c1] = nullptr;
    movingPiece->setPosition(r2, c2);
    movingPiece->setHasMoved(true);
    // auto t1 = high_resolution_clock::now();
    // long long dt = duration_cast<microseconds>(t1 - t0).count();
    // g_muProfile.movePiece += dt;
    // localApply += dt;

    
    // Handle castling
    // auto t0 = high_resolution_clock::now();
    if (move.isCastling()) {
        executeCastlingRookMove(r1, move.castlingType, undo);
    }
    // auto t1 = high_resolution_clock::now();
    // long long dt = duration_cast<microseconds>(t1 - t0).count();
    // g_muProfile.castlingBookkeeping += dt;
    // localApply += dt;
    
    // Handle promotion
    // auto t0 = high_resolution_clock::now();
    if (move.isPromotion) {
        undo.wasPromotion = true;
        undo.originalPromotionType = move.promotionType;
        
        // Remove pawn from manager and store in undo
        undo.promotedPawn = pieceManager->removePiece(movingPiece->id);
        pieceGrid[r2][c2] = nullptr;
        
        // Create and place promoted piece
        Color promotionColor = undo.promotedPawn->getColor();
        SDL_Renderer* renderer = undo.promotedPawn->getRenderer();
        
        auto promotedPiece = createPromotedPiece(move.promotionType, promotionColor, renderer);
        promotedPiece->setPosition(r2, c2);
        promotedPiece->setHasMoved(true);
        
        Piece* rawPromoted = promotedPiece.get();
        pieceManager->addPiece(std::move(promotedPiece));
        pieceGrid[r2][c2] = rawPromoted;
    } else {
        handlePawnPromotion(pieceGrid[r2][c2], r2, c2);
    }
    // auto t1 = high_resolution_clock::now();
    // long long dt = duration_cast<microseconds>(t1 - t0).count();
    // g_muProfile.promotionHandling += dt;
    // localApply += dt;
    
    // g_muProfile.applyTime += localApply;
    // g_muProfile.applyCalls++;
    
    moveHistory.push_back(move);
    if (trackUndo) {
        return undo;
    }
    else {
        UndoMove emptyUndo;
        return emptyUndo;
    }
}

void MoveExecutor::undoMove(const Move& move, UndoMove& undo) {
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;
    
    Piece* pieceOnEndSquare = pieceGrid[r2][c2];
    
    long long localUnmake = 0;
    
    // Handle castling-specific undo
    if (undo.castlingType != CastlingType::NONE) {
        // auto t0 = high_resolution_clock::now();
        undoPieceMove(r1, c1, r2, c2, undo.movedPiecePrevHasMoved);
        
        // Restore rook
        if (undo.rookToCol != -1 && pieceGrid[undo.rookRow][undo.rookToCol]) {
            pieceGrid[undo.rookRow][undo.rookFromCol] = pieceGrid[undo.rookRow][undo.rookToCol];
            pieceGrid[undo.rookRow][undo.rookToCol] = nullptr;
            
            Rook* rook = dynamic_cast<Rook*>(pieceGrid[undo.rookRow][undo.rookFromCol]);
            if (rook) {
                rook->setPosition(undo.rookRow, undo.rookFromCol);
                rook->setHasMoved(undo.rookPrevHasMoved);
                rook->setIsCastlingEligible(!undo.rookPrevHasMoved);
            }
        }
        // auto t1 = high_resolution_clock::now();
        // long long dt = duration_cast<microseconds>(t1 - t0).count();
        // g_muProfile.unmakeCastling += dt;
        // localUnmake += dt;
    } else {
        // auto t0 = high_resolution_clock::now();
        undoPieceMove(r1, c1, r2, c2, undo.movedPiecePrevHasMoved);
        // auto t1 = high_resolution_clock::now();
        // long long dt = duration_cast<microseconds>(t1 - t0).count();
        // g_muProfile.unmakeMoveBack += dt;
        // localUnmake += dt;
    }
    
    // Handle promotion undo
    if (undo.wasPromotion) {
        // Remove promoted piece
        if (pieceOnEndSquare) {
            std::ostringstream oss;
            oss << "undoMove: removing promoted piece id=" << pieceOnEndSquare->id
                << " type=" << pieceOnEndSquare->stringPieceType()
                << " at (" << r2 << "," << c2 << ")";
            Logger::log(LogLevel::INFO, oss.str(), __FILE__, __LINE__);
            
            pieceGrid[r2][c2] = nullptr;
            auto removedPromoted = pieceManager->removePiece(pieceOnEndSquare->id);
        }
        
        // Restore original pawn
        if (undo.promotedPawn) {
            restorePieceToManager(std::move(undo.promotedPawn), r1, c1);
            pieceGrid[r1][c1]->setHasMoved(undo.movedPiecePrevHasMoved);
        }
    } else {
        pieceGrid[r2][c2] = nullptr;
    }
    
    // Restore captured piece
    if (undo.wasCapture && undo.capturedPiece) {
        int capturedRow = undo.capturedPiecePos.first;
        int capturedCol = undo.capturedPiecePos.second;
        restorePieceToManager(std::move(undo.capturedPiece), capturedRow, capturedCol);
    }
    
    // g_muProfile.unmakeTime += localUnmake;
    // g_muProfile.unmakeCalls++;
    
    // Fix: Remove the last move from history instead of adding it
    if (!moveHistory.empty()) {
        moveHistory.pop_back();
    }
}

void MoveExecutor::undoPieceMove(int r1, int c1, int r2, int c2, bool prevHasMoved) {
    pieceGrid[r1][c1] = pieceGrid[r2][c2];
    pieceGrid[r2][c2] = nullptr;
    
    if (pieceGrid[r1][c1]) {
        pieceGrid[r1][c1]->setPosition(r1, c1);
        pieceGrid[r1][c1]->setHasMoved(prevHasMoved);
        board->updatePiecePositionInManager(pieceGrid[r1][c1]);
    }
}