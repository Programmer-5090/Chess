#include "gameLogic.h"
#include "board.h"
#include "pieces/pieces.h"
#include "logger.h"
#include <iostream>

GameLogic::GameLogic() : currentPlayer(WHITE), pieceIsSelected(false) {
    selectedPieceSquare = {-1, -1};
}

void GameLogic::switchPlayer() {
    currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
    LOG_INFO(std::string("Player switched to: ") + (currentPlayer == WHITE ? "WHITE" : "BLACK"));
}

void GameLogic::clearSelection() {
    pieceIsSelected = false;
    selectedPieceSquare = {-1, -1};
    possibleMoves.clear();
    LOG_INFO("Selection cleared.");
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
    LOG_INFO(std::string("Clicked board square: (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + ")");

    if (pieceIsSelected) {
        // A piece is already selected, check if this click is a move
        bool validMoveClicked = false;
        Piece* currentSelectedPiece = board.getPieceAt(selectedPieceSquare.first, selectedPieceSquare.second);
        if (!currentSelectedPiece) {
            clearSelection();
            return;
        }

        if (currentSelectedPiece->getType() == PAWN) {
            Pawn* pawn = static_cast<Pawn*>(currentSelectedPiece);
            if (pawn->getEnPassantCaptureEligible()) {
                LOG_INFO("This pawn is enpassant capturable");
            } else {
                LOG_INFO("This pawn is NOT enpassant capturable");
            }
        }

        for (const auto& move : possibleMoves) {
            if (!board.isCheckMate(move.piece->getColor())) {
                if (move.endPos.first == r_clicked && move.endPos.second == c_clicked) {
                    // User clicked on a potential destination.
                    if (board.checkIfMoveRemovesCheck(move)) {
                        LOG_INFO(std::string("Attempting to make move to (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + ")");
                        makeMove(move, board);
                        validMoveClicked = true;
                        Color oppColor = (move.piece->getColor() == BLACK) ? WHITE : BLACK;
                        if (board.isCheckMate(oppColor)) {
                            std::string colorName = (oppColor == BLACK) ? "Black" : "White";
                            LOG_WARN(colorName + std::string(" is CHECKMATED"));
                        }
                    } else {
                        LOG_WARN("Illegal move: King would be in check.");
                    }
                    break;
                }
            }
        }
        
        if (!validMoveClicked) {
            // Clicked on a square that is not a valid move.
            Piece* pieceAtClickedSquare = board.getPieceAt(r_clicked, c_clicked);
            if (pieceAtClickedSquare && pieceAtClickedSquare->getColor() == currentPlayer) {
                // Select this new piece
                clearSelection();
                selectedPieceSquare = {r_clicked, c_clicked};
                pieceIsSelected = true;
                possibleMoves = pieceAtClickedSquare->getPseudoLegalMoves(board);
                LOG_INFO(std::string("Selected new piece at (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + "). Possible moves: " + std::to_string(possibleMoves.size()));
            } else {
                // Clicked on an empty square or opponent's piece. Deselect.
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
            LOG_INFO(std::string("Selected piece at (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + "). Possible moves: " + std::to_string(possibleMoves.size()));
        } else {
            LOG_INFO("Clicked on empty or opponent piece. No selection.");
        }
    }
}

void GameLogic::makeMove(const Move& move, Board& board) {
    // Clear en passant flags from the current player's pawns
    board.clearEnPassantFlags(currentPlayer);

    // Get piece info before the move is made
    const Piece* movingPiece = move.piece;
    if (!movingPiece) {
        LOG_ERROR("Error: Attempted to make a move with a null piece.");
        return;
    }

    // Execute the move on the board
    LOG_INFO(std::string("Making move from (") + std::to_string(move.startPos.first) + "," + std::to_string(move.startPos.second)
              + ") to (" + std::to_string(move.endPos.first) + "," + std::to_string(move.endPos.second) + ")");
    board.movePiece(move); 

    // After the move, check if it was a two-square pawn push to set a new en passant flag
    if (movingPiece->getType() == PAWN && abs(move.startPos.first - move.endPos.first) == 2) {
        Piece* pieceAtDest = board.getPieceAt(move.endPos.first, move.endPos.second);
        if (pieceAtDest && pieceAtDest->getType() == PAWN) {
            static_cast<Pawn*>(pieceAtDest)->setEnPassantCaptureEligible(true);
            LOG_INFO(std::string("Pawn at (") + std::to_string(move.endPos.first) + "," + std::to_string(move.endPos.second) + ") is now en passant eligible.");
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
