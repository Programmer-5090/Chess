#include <chess/board/board.h>
#include <chess/board/move_executor.h>
#include <chess/utils/logger.h>
#include <chess/board/piece_manager.h>
#include <chess/rendering/board_renderer.h>
#include <chess/board/pieces/pieces.h>
// Correct promotion dialog include (lives under chess/ui/controls)
#include <chess/ui/controls/promotion_dialog.h>
#include <chess/utilities.h>
#include <chess/rendering/texture_cache.h>
#include <chess/utils/profiler.h>
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
    // FEN character to piece type/color mapping for piece placement parsing
    std::unordered_map<char, std::pair<Color, PieceType>> pieceFromSymbol = {
        {'P', {WHITE, PAWN}}, {'p', {BLACK, PAWN}},
        {'R', {WHITE, ROOK}}, {'r', {BLACK, ROOK}},
        {'N', {WHITE, KNIGHT}}, {'n', {BLACK, KNIGHT}},
        {'B', {WHITE, BISHOP}}, {'b', {BLACK, BISHOP}},
        {'Q', {WHITE, QUEEN}}, {'q', {BLACK, QUEEN}},
        {'K', {WHITE, KING}}, {'k', {BLACK, KING}}
    };
    clearPieceGridAndPieces();

    // Parse piece placement (field 1): iterate through FEN board representation
    std::string fenBoard = splitString(fen, ' ')[0];
    int row = 0, col = 0;
    for (char c : fenBoard) {
        if (c == '/') {
            row++;  // Move to next rank
            col = 0;
        } else if (isdigit(c)) {
            col += c - '0'; // Skip empty squares (digit represents count)
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

    // Parse castling rights from FEN (field 3): K=White kingside, Q=White queenside, k=Black kingside, q=Black queenside
    std::vector<std::string> fenParts = splitString(fen, ' ');
    if (fenParts.size() >= 3) {
        std::string castlingRights = fenParts[2];
        
        // Set castling eligibility for kings and rooks based on FEN rights string
        // Must check piece positions to determine which rook corresponds to which castle
        for (auto& row : pieceGrid) {
            for (Piece* piece : row) {
                if (piece) {
                    if (piece->getType() == KING) {
                        King* king = static_cast<King*>(piece);
                        // Check if this king can castle based on FEN
                        if (piece->getColor() == WHITE) {
                            // White king can castle if 'K' or 'Q' is in castling rights
                            king->setIsCastlingEligible(castlingRights.find('K') != std::string::npos || 
                                                       castlingRights.find('Q') != std::string::npos);
                        } else {
                            // Black king can castle if 'k' or 'q' is in castling rights  
                            king->setIsCastlingEligible(castlingRights.find('k') != std::string::npos || 
                                                       castlingRights.find('q') != std::string::npos);
                        }
                    } else if (piece->getType() == ROOK) {
                        Rook* rook = static_cast<Rook*>(piece);
                        // Determine which rook this is and set eligibility accordingly
                        int row = piece->getPosition().first;
                        int col = piece->getPosition().second;
                        
                        if (piece->getColor() == WHITE && row == 7) {
                            // White rooks on back rank
                            if (col == 0) {
                                // Queenside rook - eligible if 'Q' in castling rights
                                rook->setIsCastlingEligible(castlingRights.find('Q') != std::string::npos);
                            } else if (col == 7) {
                                // Kingside rook - eligible if 'K' in castling rights
                                rook->setIsCastlingEligible(castlingRights.find('K') != std::string::npos);
                            } else {
                                // Rook not on original castling squares
                                rook->setIsCastlingEligible(false);
                            }
                        } else if (piece->getColor() == BLACK && row == 0) {
                            // Black rooks on back rank
                            if (col == 0) {
                                // Queenside rook - eligible if 'q' in castling rights
                                rook->setIsCastlingEligible(castlingRights.find('q') != std::string::npos);
                            } else if (col == 7) {
                                // Kingside rook - eligible if 'k' in castling rights
                                rook->setIsCastlingEligible(castlingRights.find('k') != std::string::npos);
                            } else {
                                // Rook not on original castling squares
                                rook->setIsCastlingEligible(false);
                            }
                        } else {
                            // Rook not on back rank - cannot castle
                            rook->setIsCastlingEligible(false);
                        }
                    }
                }
            }
        }
    }

    // Parse en passant target square from FEN (field 4): algebraic notation or "-" for none
    if (fenParts.size() >= 4) {
        std::string enPassantSquare = fenParts[3];
        
        // Reset all pawn en passant flags before setting new ones
        for (auto& row : pieceGrid) {
            for (Piece* piece : row) {
                if (piece && piece->getType() == PAWN) {
                    static_cast<Pawn*>(piece)->setEnPassantCaptureEligible(false);
                }
            }
        }
        
        // Convert en passant target square to capturable pawn position
        if (enPassantSquare != "-" && enPassantSquare.length() == 2) {
            int targetCol = enPassantSquare[0] - 'a';  // Convert file 'a'-'h' to column 0-7
            int targetRow = 8 - (enPassantSquare[1] - '0');  // Convert rank '1'-'8' to row 7-0
            
            // En passant target is the square behind the pawn that just moved two squares
            // Target on rank 3 (row 5) means Black pawn on rank 4 (row 4) can be captured
            // Target on rank 6 (row 2) means White pawn on rank 5 (row 3) can be captured
            int pawnRow = (targetRow == 2) ? 3 : 4;
            
            if (targetCol >= 0 && targetCol < 8 && pawnRow >= 0 && pawnRow < 8) {
                Piece* pawn = getPieceAt(pawnRow, targetCol);
                if (pawn && pawn->getType() == PAWN) {
                    static_cast<Pawn*>(pawn)->setEnPassantCaptureEligible(true);
                }
            }
        }
    }

    // Parse active color from FEN (field 2): "w" = White to move, "b" = Black to move
    if (fenParts.size() >= 2) {
        std::string activeColor = fenParts[1];
        currentPlayer = (activeColor == "w") ? WHITE : BLACK;
    } else {
        currentPlayer = WHITE;  // Default to White if field missing
    }

    // Parse halfmove clock from FEN (field 5): moves since last pawn move or capture for 50-move rule
    if (fenParts.size() >= 5) {
        try {
            halfMoveClock = std::stoi(fenParts[4]);
        } catch (const std::exception&) {
            halfMoveClock = 0;  // Reset to 0 if invalid format
        }
    } else {
        halfMoveClock = 0;  // Default if field missing
    }

    // Parse fullmove number from FEN (field 6): increments after each Black move, starts at 1
    if (fenParts.size() >= 6) {
        try {
            fullMoveNumber = std::stoi(fenParts[5]);
        } catch (const std::exception&) {
            fullMoveNumber = 1;  // Reset to 1 if invalid format
        }
    } else {
        fullMoveNumber = 1;  // Default if field missing
    }

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
    
    // Initialize TextureCache with renderer BEFORE loading FEN
    if (gameRenderer) {
        TextureCache::setRenderer(gameRenderer);  // Add this line
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
    allLegalMoves.reserve(256);
    getAllLegalMoves(color, allLegalMoves, generateCastlingMoves);
    return allLegalMoves;
}

void Board::getAllLegalMoves(Color color, std::vector<Move>& out, bool generateCastlingMoves) const {
    out.clear();
    out.reserve(256);
    g_profiler.startTimer("getAllLegalMoves");

    // Get pieces of a specific color using the manager
    const std::vector<Piece*>& pieces = pieceManager->getPieces(color);

    std::vector<Move> pieceMoves;
    for (Piece* piece : pieces) {
        if (!piece) continue;
        // Use the out-parameter variant on Piece to append moves without reallocating repeatedly
        piece->getPseudoLegalMoves(*this, pieceMoves, generateCastlingMoves);

        // A move is legal if it doesn't leave the king in check
        for (const Move& move : pieceMoves) {
            if (const_cast<Board*>(this)->checkIfMoveRemovesCheck(move)) {
                out.push_back(move);
            }
        }
        pieceMoves.clear();
    }
    g_profiler.endTimer("getAllLegalMoves");
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
    // Evaluate king safety either in current position or after applying a hypothetical move
    std::pair<int,int> kingPos{-1,-1};

    if (hypotheticalMove) {
        // Temporarily apply move to evaluate resulting position without permanent state change
        int r1 = hypotheticalMove->startPos.first;
        int c1 = hypotheticalMove->startPos.second;
        int r2 = hypotheticalMove->endPos.first;
        int c2 = hypotheticalMove->endPos.second;

        Piece* movingPiece = getPieceAt(r1, c1);
        Piece* capturedPiece = getPieceAt(r2, c2);
        
        // Temporarily modify grid for attack calculation
        pieceGrid[r1][c1] = nullptr;
        
        // Handle promotion moves: create temporary promoted piece for accurate attack calculation
        std::unique_ptr<Piece> tempPromotedPiece;
        Piece* finalPiece = movingPiece;
        
        if (hypotheticalMove->isPromotion && movingPiece && movingPiece->getType() == PAWN) {
            // Promotion requires creating temporary piece of promoted type for accurate attack pattern evaluation
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
                    tempPromotedPiece = std::make_unique<Queen>(pieceColor, QUEEN, renderer); // Default fallback
                    break;
            }
            
            if (tempPromotedPiece) {
                tempPromotedPiece->setPosition(r2, c2);
                finalPiece = tempPromotedPiece.get();
            }
        }
        
        pieceGrid[r2][c2] = finalPiece;

        // Determine king position after hypothetical move
        if (movingPiece && movingPiece->getType() == KING) {
            kingPos = { r2, c2 }; // King moved to new position
        } else {
            Piece* king = pieceManager->findKing(color);
            if (king) kingPos = king->getPosition(); // King stays in current position
        }

        // Evaluate if king would be in check after the hypothetical move
        bool result = true; // Assume check if king not found (defensive)
        if (kingPos.first != -1) {
            result = isSquareAttacked(kingPos.first, kingPos.second, (color == WHITE ? BLACK : WHITE));
        }

        // Restore original board state (critical for move legality testing)
        pieceGrid[r1][c1] = movingPiece;
        pieceGrid[r2][c2] = capturedPiece;
        // tempPromotedPiece automatically destroyed when going out of scope

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
    // Check pawn attacks: pawns attack diagonally one square forward
    int dir = (byColor == BLACK ? +1 : -1); // Black pawns move down (+1), White pawns move up (-1)
    int pr = r - dir; // Attacking pawn position is one step behind target in pawn's movement direction
    for (int dc : {-1, +1}) { // Check both diagonal attack squares
        int pc = c + dc; // Pawn column offset from target
        if (pr >= 0 && pr < 8 && pc >= 0 && pc < 8) {
            Piece* p = getPieceAt(pr, pc);
            if (p && p->getColor() == byColor && p->getType() == PAWN) return true;
        }
    }

    // Check knight attacks: L-shaped moves (2+1 or 1+2 squares in perpendicular directions)
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

// Pin detection implementation (based on C# reference)
bool Board::isPinnedPiece(int pieceRow, int pieceCol, Color pieceColor) const {
    // Find the king of the same color
    Piece* king = pieceManager->findKing(pieceColor);
    if (!king) return false;
    
    auto [kingRow, kingCol] = king->getPosition();
    
    // Check if this piece is on the same rank, file, or diagonal as the king
    int rowDiff = pieceRow - kingRow;
    int colDiff = pieceCol - kingCol;
    
    // If not aligned with king, can't be pinned
    if (rowDiff != 0 && colDiff != 0 && abs(rowDiff) != abs(colDiff)) {
        return false;
    }
    
    // Determine the direction from king to piece
    int rowDir = (rowDiff == 0) ? 0 : (rowDiff > 0 ? 1 : -1);
    int colDir = (colDiff == 0) ? 0 : (colDiff > 0 ? 1 : -1);
    
    // First, verify there's a clear path from king to piece
    int checkRow = kingRow + rowDir;
    int checkCol = kingCol + colDir;
    
    while (checkRow != pieceRow || checkCol != pieceCol) {
        if (checkRow < 0 || checkRow >= 8 || checkCol < 0 || checkCol >= 8) {
            return false; // Off board
        }
        
        Piece* p = getPieceAt(checkRow, checkCol);
        if (p) {
            return false; // Path blocked between king and piece
        }
        
        checkRow += rowDir;
        checkCol += colDir;
    }
    
    // Now check if there's an enemy piece that could create a pin
    // Look beyond the piece in the same direction
    checkRow = pieceRow + rowDir;
    checkCol = pieceCol + colDir;
    
    while (checkRow >= 0 && checkRow < 8 && checkCol >= 0 && checkCol < 8) {
        Piece* p = getPieceAt(checkRow, checkCol);
        if (p) {
            if (p->getColor() != pieceColor) {
                // Found an enemy piece - check if it can attack along this line
                PieceType type = p->getType();
                
                // Check if this enemy piece can attack along the line
                bool canAttackLine = false;
                if (rowDir == 0 || colDir == 0) {
                    // Horizontal/vertical line - rook or queen can attack
                    canAttackLine = (type == ROOK || type == QUEEN);
                } else {
                    // Diagonal line - bishop or queen can attack
                    canAttackLine = (type == BISHOP || type == QUEEN);
                }
                
                if (canAttackLine) {
                    // This piece is pinned by the enemy piece we found
                    return true;
                }
            }
            // Any piece on the line stops the potential pin
            break;
        }
        checkRow += rowDir;
        checkCol += colDir;
    }
    
    return false;
}

// Check if a move would cause a discovered check by moving a pinned piece
bool Board::wouldMoveCauseDiscoveredCheck(const Move& move, Color movingColor) const {
    int fromRow = move.startPos.first;
    int fromCol = move.startPos.second;
    int toRow = move.endPos.first;
    int toCol = move.endPos.second;
    
    Piece* king = pieceManager->findKing(movingColor);
    if (!king) return true; // Defensive: if no king, treat as illegal
    
    auto [kingRow, kingCol] = king->getPosition();
    Piece* movingPiece = getPieceAt(fromRow, fromCol);
    if (!movingPiece) return false;
    
    // Note: Pin-based filtering should be sufficient for most discovery checks
    // The hypothetical move evaluation in isKingInCheck should catch remaining issues
    
    // If the piece is pinned, check if the move is along the pin line
    if (isPinnedPiece(fromRow, fromCol, movingColor)) {
        // Check if the move is along the line from king through the piece
        int kingToFromRowDiff = fromRow - kingRow;
        int kingToFromColDiff = fromCol - kingCol;
        int kingToToRowDiff = toRow - kingRow;
        int kingToToColDiff = toCol - kingCol;
        
        // Calculate if both positions are on the same ray from the king
        bool fromOnRay = false, toOnRay = false;
        
        // Check if from position is on a ray from king
        if (kingToFromRowDiff == 0 || kingToFromColDiff == 0 || abs(kingToFromRowDiff) == abs(kingToFromColDiff)) {
            fromOnRay = true;
        }
        
        // Check if to position is on the same ray from king
        if (kingToToRowDiff == 0 || kingToToColDiff == 0 || abs(kingToToRowDiff) == abs(kingToToColDiff)) {
            // Normalize the directions to check if they're the same ray
            auto normalize = [](int diff) { return (diff == 0) ? 0 : (diff > 0 ? 1 : -1); };
            
            int fromRowDir = normalize(kingToFromRowDiff);
            int fromColDir = normalize(kingToFromColDiff);
            int toRowDir = normalize(kingToToRowDiff);
            int toColDir = normalize(kingToToColDiff);
            
            if (fromRowDir == toRowDir && fromColDir == toColDir) {
                toOnRay = true;
            }
        }
        
        // If the piece moves off the pin ray, it's an illegal move
        if (fromOnRay && !toOnRay) {
            return true;
        }
    }
    
    return false;
}
