#include "board.h"
#include "pieces/piece.h"
#include "pieces/queen.h"
#include "pieces/rook.h"
#include "pieces/bishop.h"
#include "pieces/knight.h"
#include "ui/uiPromotionDialog.h"
#include "utilities.h"
#include <unordered_map>

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
}

#include <chrono>
using namespace std::chrono;

MakeUnmakeProfile g_muProfile; // defined for fine-grained profiling

void Board::loadFEN(const std::string& fen, SDL_Renderer* gameRenderer){
    // Placeholder for future FEN loading implementation
    auto pieceFromSymbol = std::unordered_map<char, std::pair<Color, PieceType>>{
        {'P', {WHITE, PAWN}}, {'p', {BLACK, PAWN}},
        {'R', {WHITE, ROOK}}, {'r', {BLACK, ROOK}},
        {'N', {WHITE, KNIGHT}}, {'n', {BLACK, KNIGHT}},
        {'B', {WHITE, BISHOP}}, {'b', {BLACK, BISHOP}},
        {'Q', {WHITE, QUEEN}}, {'q', {BLACK, QUEEN}},
        {'K', {WHITE, KING}}, {'k', {BLACK, KING}}
    };

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
                switch (type) {
                    case PAWN:
                        boardState[row][col] = std::make_unique<Pawn>(color, type, gameRenderer);
                        break;
                    case ROOK:
                        boardState[row][col] = std::make_unique<Rook>(color, type, gameRenderer);
                        break;
                    case KNIGHT:
                        boardState[row][col] = std::make_unique<Knight>(color, type, gameRenderer);
                        break;
                    case BISHOP:
                        boardState[row][col] = std::make_unique<Bishop>(color, type, gameRenderer);
                        break;
                    case QUEEN:
                        boardState[row][col] = std::make_unique<Queen>(color, type, gameRenderer);
                        break;
                    case KING:
                        boardState[row][col] = std::make_unique<King>(color, type, gameRenderer);
                        break;
                }
                boardState[row][col]->setPosition(row, col);
                col++;
            }
        }
    }

    this->startFEN = fen;
}

Board::~Board() {
    // Unique pointers will automatically clean up
}

void Board::initializeBoard(SDL_Renderer* gameRenderer) {
    for (int i = 0; i < 8; i++) { // Rows
        for (int j = 0; j < 8; j++) { // Columns
            boardGrid[i][j].x = startXPos + (j * squareSide);
            boardGrid[i][j].y = startYPos + (i * squareSide);
            boardGrid[i][j].w = squareSide;
            boardGrid[i][j].h = squareSide;
            boardState[i][j] = nullptr; // Initialize with nullptrs
        }
    }
    
    loadFEN(this->startFEN, gameRenderer);
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

void Board::resetBoard() {
    // Clear all pieces
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            boardState[i][j] = nullptr;
        }
    }
    
    // Clear captured pieces
    whiteCapturedPieces.clear();
    blackCapturedPieces.clear();
    
    // Reset to starting position
    loadFEN(this->startFEN, nullptr);
}

void Board::updateBoardState() {
    // Placeholder for future board state updates
}

void Board::draw(SDL_Renderer* renderer, const std::pair<int, int>* selectedSquare, const std::vector<Move>* possibleMoves) {
    // Draw highlights first
    if (renderer) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // Highlight selected piece's square
        if (selectedSquare) {
            SDL_FRect rect = getSquareRect(selectedSquare->first, selectedSquare->second);
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 150); // Semi-transparent green
            SDL_RenderFillRectF(renderer, &rect);
        }

        // Highlight possible moves
        if (possibleMoves) {
            for (const auto& move : *possibleMoves) {
                SDL_FRect rect = getSquareRect(move.endPos.first, move.endPos.second);
                if (checkIfMoveRemovesCheck(move)) {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 150); // Semi-transparent green
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150); // Semi-transparent red
                }
                SDL_RenderFillRectF(renderer, &rect);
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); // Reset blend mode
    }

    // Draw pieces
    for (int i = 0; i < 8; i++) { // Rows
        for (int j = 0; j < 8; j++) { // Columns
            Piece* piece = boardState[i][j].get();
            SDL_FRect fRect = boardGrid[i][j];
            if (piece != nullptr) {
                piece->draw(fRect);
            }
        }
    }
}

Piece* Board::getPieceAt(int r, int c) const {
    if (r >= 0 && r < 8 && c >= 0 && c < 8) {
        return boardState[r][c].get();
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
          r2 >= 0 && r2 < 8 && c2 >= 0 && c2 < 8 && boardState[r1][c1])) {
        // Invalid move parameters or no piece at start
        return;
    }

    std::unique_ptr<Piece> pieceTaken = nullptr;

    // Determine if a piece is captured and take ownership
    if (move.capturedPiece) {
        int capturedPieceRow = move.capturedPiece->getPosition().first;
        int capturedPieceCol = move.capturedPiece->getPosition().second;

        // Verify that the board actually has this piece at the expected location
        if (capturedPieceRow >= 0 && capturedPieceRow < 8 &&
            capturedPieceCol >= 0 && capturedPieceCol < 8 &&
            boardState[capturedPieceRow][capturedPieceCol] &&
            boardState[capturedPieceRow][capturedPieceCol].get() == move.capturedPiece) {
            
            pieceTaken = std::move(boardState[capturedPieceRow][capturedPieceCol]);
        }
    } else if (boardState[r2][c2]) {
        // If move.capturedPiece was null, but destination is occupied by an opponent
        if (boardState[r1][c1]->getColor() != boardState[r2][c2]->getColor()) {
            pieceTaken = std::move(boardState[r2][c2]);
        }
    }

    // Perform the move: move the piece from (r1,c1) to (r2,c2)
    boardState[r2][c2] = std::move(boardState[r1][c1]);

    // Update the moved piece's internal state
    if (boardState[r2][c2]) {
        boardState[r2][c2]->setPosition(r2, c2);
        boardState[r2][c2]->setHasMoved(true);
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
            std::cout << list << std::endl;
        }
    } else if (move.piece) {
        if (!blackCapturedPieces.empty()) {
            std::string list = "White has captured: ";
            for (size_t i = 0; i < blackCapturedPieces.size(); ++i) {
                if (blackCapturedPieces[i]) {
                    list += blackCapturedPieces[i]->stringPieceType();
                    if (i < blackCapturedPieces.size() - 1) {
                        list += ", ";
                    }
                }
            }
            std::cout << list << std::endl;
        }
    }

    // Handle castling
    if (move.castling) {
        if (move.isKingSide) {
            int rookDestCol = 5;
            if (boardState[r1][7] != nullptr) {
                Rook* actualRook = dynamic_cast<Rook*>(boardState[r1][7].get()); 
                if (actualRook) {
                    actualRook->setHasMoved(true);
                    actualRook->setIsCastlingEligible(false);
                    actualRook->setPosition(r1, rookDestCol);
                    boardState[r1][rookDestCol] = std::move(boardState[r1][7]);
                } else {
                    std::cerr << "Error: Piece at rook's starting position (" << r1 << ", 7) is not a Rook during castling." << std::endl;
                    return;
                }
            } else {
                std::cerr << "Error: Expected rook for castling not found at (" << r1 << ", 7)." << std::endl;
                return;
            }
        } else if (move.isQueenSide) {
            int rookDestCol = 3;
            if (boardState[r1][0] != nullptr) {
                Rook* actualRook = dynamic_cast<Rook*>(boardState[r1][0].get()); 
                if (actualRook) {
                    actualRook->setHasMoved(true);
                    actualRook->setIsCastlingEligible(false);
                    actualRook->setPosition(r1, rookDestCol);
                    boardState[r1][rookDestCol] = std::move(boardState[r1][0]);
                } else {
                    std::cerr << "Error: Piece at rook's starting position (" << r1 << ", 0) is not a Rook during castling." << std::endl;
                    return;
                }
            } else {
                std::cerr << "Error: Expected rook for castling not found at (" << r1 << ", 0)." << std::endl;
                return;
            }
        }
        
        if (boardState[r2][c2] && boardState[r2][c2]->getType() == KING) {
            King* actualKing = static_cast<King*>(boardState[r2][c2].get());
            actualKing->setIsCastlingEligible(false);
        } else {
            std::cerr << "Error: King not found at destination after castling or piece is not a King." << std::endl;
        }
    }
    // Handle Pawn Promotion
    if (move.piece && move.piece->getType() == PAWN) {
        if ((move.piece->getColor() == WHITE && r2 == 0) || (move.piece->getColor() == BLACK && r2 == 7)) {
            // Get renderer from the existing piece
            SDL_Renderer* pieceRenderer = boardState[r2][c2]->getRenderer();
            
            // Show promotion dialog instead of auto-promoting
            Color pawnColor = move.piece->getColor();
            showPromotionDialog(r2, c2, pawnColor, pieceRenderer);
        }
    }
}

void Board::applyMoveWithUndo(const Move& move, UndoMove& undo) {
    // Coordinates are pre-validated by move generation - skip bounds checking
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;

    // Clear en passant flags for the moving color (your own EP opportunities expire when you move)
    Color movingColor = boardState[r1][c1]->getColor();
    {
        auto t0 = high_resolution_clock::now();
        clearEnPassantFlags(movingColor);
        auto t1 = high_resolution_clock::now();
        g_muProfile.clearEPTime += duration_cast<microseconds>(t1 - t0).count();
    }


    undo.wasCastling = move.castling;
    undo.wasKingSide = move.isKingSide;
    undo.wasQueenSide = move.isQueenSide;

    // Capture handling: take ownership for undo restoration
    undo.wasCapture = false;
    
    // Handle captures (normal and en passant)
    {
        auto t0 = high_resolution_clock::now();
        if (move.capturedPiece) {
            undo.wasCapture = true;
            // Store the actual position of the captured piece
            undo.capturedPiecePos = move.capturedPiece->getPosition();
            
            // Remove the captured piece from its actual position
            int capturedRow = undo.capturedPiecePos.first;
            int capturedCol = undo.capturedPiecePos.second;
            
            if (capturedRow >= 0 && capturedRow < 8 && capturedCol >= 0 && capturedCol < 8 &&
                boardState[capturedRow][capturedCol] &&
                boardState[capturedRow][capturedCol].get() == move.capturedPiece) {
                undo.capturedPiece = std::move(boardState[capturedRow][capturedCol]);
            }
        }
        auto t1 = high_resolution_clock::now();
        g_muProfile.captureHandleTime += duration_cast<microseconds>(t1 - t0).count();
    }

    // Track moved piece previous hasMoved
    undo.movedPiecePrevHasMoved = boardState[r1][c1]->getHasMoved();

    // Move piece
    {
        auto t0 = high_resolution_clock::now();
        boardState[r2][c2] = std::move(boardState[r1][c1]);
        if (boardState[r2][c2]) {
            boardState[r2][c2]->setPosition(r2, c2);
            boardState[r2][c2]->setHasMoved(true);
            
            // Set en passant flag for double pawn pushes
            if (boardState[r2][c2]->getType() == PAWN && abs(r2 - r1) == 2) {
                Pawn* movedPawn = static_cast<Pawn*>(boardState[r2][c2].get());
                movedPawn->setEnPassantCaptureEligible(true);
            }
        }
        auto t1 = high_resolution_clock::now();
        g_muProfile.movePieceTime += duration_cast<microseconds>(t1 - t0).count();
    }


    // Castling rook move bookkeeping
    {
        auto t0 = high_resolution_clock::now();
        if (move.castling) {
            undo.wasCastling = true;
            undo.rookRow = r1;
            if (move.isKingSide) {
                undo.rookFromCol = 7; undo.rookToCol = 5;
            } else if (move.isQueenSide) {
                undo.rookFromCol = 0; undo.rookToCol = 3;
            }
            if (undo.rookFromCol != -1 && boardState[undo.rookRow][undo.rookFromCol]) {
                undo.rookPrevHasMoved = boardState[undo.rookRow][undo.rookFromCol]->getHasMoved();
            }
        }
        auto t1 = high_resolution_clock::now();
        g_muProfile.castlingTime += duration_cast<microseconds>(t1 - t0).count();
    }

    g_muProfile.applyCalls++;
    if (move.isKingSide) {
        int rookDestCol = 5;
        if (boardState[r1][7] != nullptr) {
            Rook* actualRook = dynamic_cast<Rook*>(boardState[r1][7].get()); 
            if (actualRook) {
                actualRook->setHasMoved(true);
                actualRook->setIsCastlingEligible(false);
                actualRook->setPosition(r1, rookDestCol);
                boardState[r1][rookDestCol] = std::move(boardState[r1][7]);
            }
        }
    } else if (move.isQueenSide) {
        int rookDestCol = 3;
        if (boardState[r1][0] != nullptr) {
            Rook* actualRook = dynamic_cast<Rook*>(boardState[r1][0].get()); 
            if (actualRook) {
                actualRook->setHasMoved(true);
                actualRook->setIsCastlingEligible(false);
                actualRook->setPosition(r1, rookDestCol);
                boardState[r1][rookDestCol] = std::move(boardState[r1][0]);
            }
        }
    }
}

void Board::unmakeMove(const Move& move, const UndoMove& undo) {
    // Coordinates are pre-validated by move generation - skip bounds checking
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;

    if (undo.wasCastling) {
        auto t0 = high_resolution_clock::now();
        // Move king back
        boardState[r1][c1] = std::move(boardState[r2][c2]);
        if (boardState[r1][c1]) {
            boardState[r1][c1]->setPosition(r1, c1);
            boardState[r1][c1]->setHasMoved(undo.movedPiecePrevHasMoved);
        }
        if (undo.rookToCol != -1 && boardState[undo.rookRow][undo.rookToCol]) {
            Rook* actualRook = dynamic_cast<Rook*>(boardState[undo.rookRow][undo.rookToCol].get());
            int rookDestCol = (move.isKingSide ? 6 : 2);
            int rookFromCol = (move.isKingSide ? 5 : 3);
            // Move rook back
            boardState[undo.rookRow][rookFromCol] = std::move(boardState[undo.rookRow][undo.rookToCol]);
            if (actualRook) {
                actualRook->setPosition(undo.rookRow, rookFromCol);
                actualRook->setHasMoved(undo.rookPrevHasMoved);
                actualRook->setIsCastlingEligible(!undo.rookPrevHasMoved);
            }
        }
        auto t1 = high_resolution_clock::now();
        g_muProfile.unmakeCastlingTime += duration_cast<microseconds>(t1 - t0).count();
    }

    // Move piece back (if not already moved during castling branch)
    if (!undo.wasCastling) {
        auto t0 = high_resolution_clock::now();
        boardState[r1][c1] = std::move(boardState[r2][c2]);
        if (boardState[r1][c1]) {
            boardState[r1][c1]->setPosition(r1, c1);
            boardState[r1][c1]->setHasMoved(undo.movedPiecePrevHasMoved);
        }
        auto t1 = high_resolution_clock::now();
        g_muProfile.unmakeMoveBackTime += duration_cast<microseconds>(t1 - t0).count();
    }

    // Restore captured piece if any
    {
        auto t0 = high_resolution_clock::now();
        if (undo.wasCapture && undo.capturedPiece) {
            // Restore captured piece to its actual position (handles en passant correctly)
            int capturedRow = undo.capturedPiecePos.first;
            int capturedCol = undo.capturedPiecePos.second;
            
            if (capturedRow >= 0 && capturedRow < 8 && capturedCol >= 0 && capturedCol < 8) {
                boardState[capturedRow][capturedCol] = std::move(const_cast<UndoMove&>(undo).capturedPiece);
                if (boardState[capturedRow][capturedCol]) {
                    boardState[capturedRow][capturedCol]->setPosition(capturedRow, capturedCol);
                }
            }
        } else {
            if (!undo.wasCastling) {
                boardState[r2][c2] = nullptr;
            }
        }
        auto t1 = high_resolution_clock::now();
        g_muProfile.unmakeRestoreCaptureTime += duration_cast<microseconds>(t1 - t0).count();
    }

    g_muProfile.unmakeCalls++;

}

std::vector<Move> Board::getAllLegalMoves(Color color, bool generateCastlingMoves) const {
    std::vector<Move> allLegalMoves;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            Piece* piece = getPieceAt(i, j);
            if (piece && piece->getColor() == color) {
                std::vector<Move> pieceMoves = piece->getPseudoLegalMoves(*this, generateCastlingMoves);
                for (const auto& move : pieceMoves) { 
                    allLegalMoves.push_back(move);
                }
            }
        }
    }
    return allLegalMoves;
}

bool Board::isKingInCheck(Color color) const {
    // Locate king position
    int kr = -1, kc = -1;
    for (int i = 0; i < 8 && kr == -1; ++i) {
        for (int j = 0; j < 8; ++j) {
            Piece* p = getPieceAt(i, j);
            if (p && p->getColor() == color && p->getType() == KING) {
                kr = i; kc = j; break;
            }
        }
    }
    if (kr == -1) {
        std::string colorName = (color == WHITE) ? "White" : "Black";
        std::cerr << "Error: No king of color " << colorName << " found on the board." << std::endl;
        return false;
    }
    Color opp = (color == WHITE ? BLACK : WHITE);
    return isSquareAttacked(kr, kc, opp);
}

bool Board::isSquareAttacked(int r, int c, Color byColor) const {
    // Pawns
    int dir = (byColor == BLACK ? +1 : -1);
    int pr = r - dir; // pawn would be one step behind the target in its moving direction
    for (int dc : {-1, +1}) {
        int pc = c - dc;
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
    int r1 = move.startPos.first;
    int c1 = move.startPos.second;
    int r2 = move.endPos.first;
    int c2 = move.endPos.second;

    if (!(r1 >= 0 && r1 < 8 && c1 >= 0 && c1 < 8 &&
          r2 >= 0 && r2 < 8 && c2 >= 0 && c2 < 8 && boardState[r1][c1])) {
        return false; // An invalid move doesn't remove check
    }

    Piece* movingPieceInfo = boardState[r1][c1].get(); // Get info before moving

    // Store the piece at the destination, if any
    std::unique_ptr<Piece> pieceOriginallyAtDestination = nullptr;
    if (boardState[r2][c2] != nullptr) {
        // Ensure it's an opponent piece if capturing
        if (boardState[r1][c1]->getColor() == boardState[r2][c2]->getColor()) {
            return false; // Cannot move onto a friendly piece; illegal move
        }
        pieceOriginallyAtDestination = std::move(boardState[r2][c2]);
    }

    // Perform the move temporarily
    boardState[r2][c2] = std::move(boardState[r1][c1]);
    if (boardState[r2][c2]) {
        boardState[r2][c2]->setPosition(r2, c2);
    }

    // Check if the king of the relevant color is in check
    bool kingIsInCheckAfterMove = isKingInCheck(movingPieceInfo->getColor());

    // Revert the move
    boardState[r1][c1] = std::move(boardState[r2][c2]);
    if (boardState[r1][c1]) {
        boardState[r1][c1]->setPosition(r1, c1);
    }
    boardState[r2][c2] = std::move(pieceOriginallyAtDestination);
    if (boardState[r2][c2]) {
        boardState[r2][c2]->setPosition(r2, c2);
    }

    // Return true if the king is NOT in check after the move
    return !kingIsInCheckAfterMove;
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


void Board::clearEnPassantFlags(Color colorToClear) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            Piece* piece = getPieceAt(i, j);
            if (piece && piece->getType() == PAWN && piece->getColor() == colorToClear) {
                static_cast<Pawn*>(piece)->setEnPassantCaptureEligible(false);
            }
        }
    }
}

void Board::promotePawnTo(int row, int col, Color color, PieceType pieceType, SDL_Renderer* renderer) {
    std::unique_ptr<Piece> newPiece;
    
    switch (pieceType) {
        case QUEEN:
            newPiece = std::make_unique<Queen>(color, QUEEN, renderer);
            break;
        case ROOK:
            newPiece = std::make_unique<Rook>(color, ROOK, renderer);
            break;
        case BISHOP:
            newPiece = std::make_unique<Bishop>(color, BISHOP, renderer);
            break;
        case KNIGHT:
            newPiece = std::make_unique<Knight>(color, KNIGHT, renderer);
            break;
        default:
            // Default to Queen if invalid piece type
            newPiece = std::make_unique<Queen>(color, QUEEN, renderer);
            break;
    }
    
    newPiece->setPosition(row, col);
    newPiece->setHasMoved(true); // The promoted piece has "moved"
    
    // Replace the pawn with the new piece
    boardState[row][col] = std::move(newPiece);
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
