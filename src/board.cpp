#include "board.h"
#include "pieces/piece.h"

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
    
    // BLACK pieces
    boardState[0][0] = std::make_unique<Rook>(BLACK, ROOK, gameRenderer); 
    boardState[0][0]->setPosition(0, 0);
    boardState[0][1] = std::make_unique<Knight>(BLACK, KNIGHT, gameRenderer); 
    boardState[0][1]->setPosition(0, 1);
    boardState[0][2] = std::make_unique<Bishop>(BLACK, BISHOP, gameRenderer); 
    boardState[0][2]->setPosition(0, 2);
    boardState[0][3] = std::make_unique<Queen>(BLACK, QUEEN, gameRenderer); 
    boardState[0][3]->setPosition(0, 3);
    boardState[0][4] = std::make_unique<King>(BLACK, KING, gameRenderer); 
    boardState[0][4]->setPosition(0, 4);
    boardState[0][5] = std::make_unique<Bishop>(BLACK, BISHOP, gameRenderer); 
    boardState[0][5]->setPosition(0, 5);
    boardState[0][6] = std::make_unique<Knight>(BLACK, KNIGHT, gameRenderer); 
    boardState[0][6]->setPosition(0, 6);
    boardState[0][7] = std::make_unique<Rook>(BLACK, ROOK, gameRenderer); 
    boardState[0][7]->setPosition(0, 7);

    // WHITE pieces
    boardState[7][0] = std::make_unique<Rook>(WHITE, ROOK, gameRenderer); 
    boardState[7][0]->setPosition(7, 0);
    boardState[7][1] = std::make_unique<Knight>(WHITE, KNIGHT, gameRenderer); 
    boardState[7][1]->setPosition(7, 1);
    boardState[7][2] = std::make_unique<Bishop>(WHITE, BISHOP, gameRenderer); 
    boardState[7][2]->setPosition(7, 2);
    boardState[7][3] = std::make_unique<Queen>(WHITE, QUEEN, gameRenderer); 
    boardState[7][3]->setPosition(7, 3);
    boardState[7][4] = std::make_unique<King>(WHITE, KING, gameRenderer); 
    boardState[7][4]->setPosition(7, 4);
    boardState[7][5] = std::make_unique<Bishop>(WHITE, BISHOP, gameRenderer); 
    boardState[7][5]->setPosition(7, 5);
    boardState[7][6] = std::make_unique<Knight>(WHITE, KNIGHT, gameRenderer); 
    boardState[7][6]->setPosition(7, 6);
    boardState[7][7] = std::make_unique<Rook>(WHITE, ROOK, gameRenderer); 
    boardState[7][7]->setPosition(7, 7);

    // Pawns
    for (int i = 0; i < 8; i++) {
        boardState[1][i] = std::make_unique<Pawn>(BLACK, PAWN, gameRenderer); 
        boardState[1][i]->setPosition(1, i);
        boardState[6][i] = std::make_unique<Pawn>(WHITE, PAWN, gameRenderer); 
        boardState[6][i]->setPosition(6, i);
    }
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
    boardR = static_cast<int>((screenY - startYPos) / squareSide);
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
    Piece* currentPiece;
    // First find the players king
    for (int i = 0; i < 8; i++) { // Row
        for (int j = 0; j < 8; j++) { // Col
            currentPiece = getPieceAt(i, j);
            if (currentPiece && currentPiece->getColor() == color && currentPiece->getType() == KING) {
                King* king = static_cast<King*>(currentPiece);
                return king->getIsKingInCheck(*this);
            }
        }
    }
    // If loops complete, no king of 'color' was found.
    std::string colorName = (color == WHITE) ? "White" : "Black";
    std::cerr << "Error: No king of color " << colorName << " found on the board." << std::endl;
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
