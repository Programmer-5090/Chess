#include "headers/gameLogic.h"
#include "headers/board.h"
#include "headers/pieces/piece.h"
#include <iostream>

GameLogic::GameLogic() : currentPlayer(WHITE), pieceIsSelected(false) {
    selectedPieceSquare = {-1, -1};
}

void GameLogic::switchPlayer(){
    currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
    std::cout << "Player switched to: " << (currentPlayer == WHITE ? "WHITE" : "BLACK") << std::endl;
}

void GameLogic::clearSelection() {
    pieceIsSelected = false;
    selectedPieceSquare = {-1, -1};
    possibleMoves.clear();
    std::cout << "Selection cleared." << std::endl;
}

void GameLogic::handleMouseClick(int mouseX, int mouseY, Board& board, bool leftMouseClicked) {
    if (!leftMouseClicked) return;

    int r_clicked, c_clicked;
    if (!board.screenToBoardCoords(mouseX, mouseY, r_clicked, c_clicked)) {
        // Clicked outside the board
        if (pieceIsSelected) { 
            clearSelection(); 
        }
        return;
    }
    std::cout << "Clicked board square: (" << r_clicked << ", " << c_clicked << ")" << std::endl;


    if (pieceIsSelected) {
        // A piece is already selected, check if this click is a move
        bool validMoveClicked = false;
        // Create a temporary non-const pointer to the selected piece for generating moves
        Piece* currentSelectedPiece = board.getPieceAt(selectedPieceSquare.first, selectedPieceSquare.second);
        if (!currentSelectedPiece) {
            clearSelection();
            return;
        }

        if(currentSelectedPiece->getType() == PAWN){
            Pawn* pawn = static_cast<Pawn*>(currentSelectedPiece);
            if(pawn->getEnPassantCaptureEligible()){
                std::cout << "This pawn is enpassant capturable" << std::endl;
            }
            else{
                std::cout << "This pawn is NOT enpassant capturable" << std::endl;
            }
        }


        for (const auto& move : possibleMoves) {
            if(!board.isCheckMate(move.piece->getColor())){
                if (move.endPos.first == r_clicked && move.endPos.second == c_clicked) {
                    // User clicked on a potential destination.
                    // Now, check if this move is truly legal (doesn't leave or put king in check).
                    if (board.checkIfMoveRemovesCheck(move)) {
                        std::cout << "Attempting to make move to (" << r_clicked << ", " << c_clicked << ")" << std::endl;
                        makeMove(move, board);
                        validMoveClicked = true;
                        Color oppColor = (move.piece->getColor() == BLACK) ? WHITE : BLACK;
                        if (board.isCheckMate(oppColor)){
                            std::string colorName = (oppColor == BLACK) ? "Black" : "White";
                            std::cout<< colorName << " is CHECKMATED" << std::endl;
                        }
                    } else {
                        std::cout << "Illegal move: King would be in check." << std::endl;
                    }
                    break; // Processed the click for this specific target square
                }
            }
        }
        if (!validMoveClicked) {
            // Clicked on a square that is not a valid move.
            // Check if it's another piece of the current player to select it instead.
            Piece* pieceAtClickedSquare = board.getPieceAt(r_clicked, c_clicked);
            if (pieceAtClickedSquare && pieceAtClickedSquare->getColor() == currentPlayer) {
                // Select this new piece
                clearSelection(); // Clear previous selection
                selectedPieceSquare = {r_clicked, c_clicked};
                pieceIsSelected = true;
                possibleMoves = pieceAtClickedSquare->getPseudoLegalMoves(board);
                std::cout << "Selected new piece at (" << r_clicked << ", " << c_clicked << "). Possible moves: " << possibleMoves.size() << std::endl;
            } else {
                // Clicked on an empty square (not a move) or an opponent's piece. Deselect.
                clearSelection();
            }
        }
    } else {
        // No piece is selected, try to select one
        Piece* piece = board.getPieceAt(r_clicked, c_clicked);
        if (piece && piece->getColor() == currentPlayer) {
            selectedPieceSquare = {r_clicked, c_clicked};
            pieceIsSelected = true;
            possibleMoves = piece->getPseudoLegalMoves(board);
            std::cout << "Selected piece at (" << r_clicked << ", " << c_clicked << "). Possible moves: " << possibleMoves.size() << std::endl;
        } else {
            // Clicked on empty square or opponent's piece, do nothing or clear (already clear)
            std::cout << "Clicked on empty or opponent piece. No selection." << std::endl;
        }
    }
}


void GameLogic::makeMove(const Move& move, Board& board){
    // This ensures the opportunity from the opponent's last move expires.
    board.clearEnPassantFlags(currentPlayer);

    // Get piece info before the move is made
    const Piece* movingPiece = move.piece;
    if (!movingPiece) {
        std::cerr << "Error: Attempted to make a move with a null piece." << std::endl;
        return;
    }

    // Execute the move on the board
    std::cout << "Making move from (" << move.startPos.first << "," << move.startPos.second
              << ") to (" << move.endPos.first << "," << move.endPos.second << ")" << std::endl;
    board.movePiece(move); 

    // After the move, check if it was a two-square pawn push to set a new en passant flag.
    if (movingPiece->getType() == PAWN && abs(move.startPos.first - move.endPos.first) == 2) {
        // The pawn that just moved is now at the destination square.
        Piece* pieceAtDest = board.getPieceAt(move.endPos.first, move.endPos.second);
        if (pieceAtDest && pieceAtDest->getType() == PAWN) {
            static_cast<Pawn*>(pieceAtDest)->setEnPassantCaptureEligible(true);
            std::cout << "Pawn at (" << move.endPos.first << "," << move.endPos.second << ") is now en passant eligible." << std::endl;
        }
    }

    clearSelection();
    switchPlayer();
}


Piece* GameLogic::getPieceAt(int row, int col, const Board& board) const {
    return board.getPieceAt(row, col);
}

Color GameLogic::getCurrentPlayer() const {
    return currentPlayer;
}

const std::pair<int, int>* GameLogic::getSelectedPieceSquare() const {
    if (pieceIsSelected) {
        return &selectedPieceSquare;
    }
    return nullptr;
}

const std::vector<Move>& GameLogic::getPossibleMoves() const {
    return possibleMoves;
}



