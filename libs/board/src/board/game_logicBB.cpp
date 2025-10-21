#include <chess/board/game_logicBB.h>
#include <chess/board/boardBB.h>
#include <chess/board/pieces/pieces.h>
#include <chess/board/bitboard/move.h>
#include <chess/board/pieces/piece_const.h>
#include <chess/utils/logger.h>
#include <iostream>

GameLogicBB::GameLogicBB() : currentPlayer(WHITE), pieceIsSelected(false) {
    selectedPieceSquare = {-1, -1};
}

void GameLogicBB::switchPlayer() {
    currentPlayer = (currentPlayer == WHITE) ? BLACK : WHITE;
    LOG_INFO(std::string("Player switched to: ") + (currentPlayer == WHITE ? "WHITE" : "BLACK"));
}

void GameLogicBB::clearSelection() {
    pieceIsSelected = false;
    selectedPieceSquare = {-1, -1};
    possibleMoves.clear();
    LOG_INFO("Selection cleared.");
}

void GameLogicBB::handleMouseClick(int mouseX, int mouseY, BoardBB& board, bool leftMouseClicked) {
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
        int selectedSquareIdx = selectedPieceSquare.first * 8 + selectedPieceSquare.second;
        int clickedSquareIdx = r_clicked * 8 + c_clicked;
        
        // Check if the clicked square is one of the possible moves
        for (const auto& move : possibleMoves) {
            if (move.targetSquare() == clickedSquareIdx) {
                // Found a matching move - attempt to execute it
                LOG_INFO(std::string("Attempting to make move to (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + ")");
                
                makeMove(move, board);
                validMoveClicked = true;
                
                // Check for checkmate after the move
                Color oppColor = (currentPlayer == WHITE) ? BLACK : WHITE; // currentPlayer already switched
                if (board.isCheckMate(oppColor)) {
                    std::string colorName = (oppColor == BLACK) ? "Black" : "White";
                    LOG_WARN(colorName + std::string(" is CHECKMATED"));
                }
                break;
            }
        }
        
        if (!validMoveClicked) {
            // Clicked on a square that is not a valid move
            // Check if user is selecting a different piece of their color
            int pieceAtClickedSquare = board.getPieceAt(r_clicked, c_clicked);
            
            if (pieceAtClickedSquare != PIECE_NONE) {
                int clickedPieceColor = chess::colorOf(pieceAtClickedSquare);
                int currentPlayerColor = (currentPlayer == WHITE) ? 8 : 16;
                
                if (clickedPieceColor == currentPlayerColor) {
                    // Select this new piece
                    clearSelection();
                    selectedPieceSquare = {r_clicked, c_clicked};
                    pieceIsSelected = true;
                    
                    // Generate legal moves for the current player
                    std::vector<chess::BBMove> allLegalMoves = board.getAllLegalMoves(currentPlayer);
                    
                    // Filter moves that start from the selected square
                    for (const auto& move : allLegalMoves) {
                        if (move.startSquare() == clickedSquareIdx) {
                            possibleMoves.push_back(move);
                        }
                    }
                    
                    LOG_INFO(std::string("Selected new piece at (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + "). Possible moves: " + std::to_string(possibleMoves.size()));
                } else {
                    // Clicked on opponent's piece - deselect
                    clearSelection();
                }
            } else {
                // Clicked on empty square - deselect
                clearSelection();
            }
        }
    } else {
        // No piece is selected, try to select one
        int pieceAtSquare = board.getPieceAt(r_clicked, c_clicked);
        
        if (pieceAtSquare != PIECE_NONE) {
            int pieceColor = chess::colorOf(pieceAtSquare);
            int currentPlayerColor = (currentPlayer == WHITE) ? 8 : 16;
            
            if (pieceColor == currentPlayerColor) {
                selectedPieceSquare = {r_clicked, c_clicked};
                pieceIsSelected = true;
                
                // Generate legal moves for the current player
                std::vector<chess::BBMove> allLegalMoves = board.getAllLegalMoves(currentPlayer);
                
                // Filter moves that start from the selected square
                int selectedSquareIdx = r_clicked * 8 + c_clicked;
                for (const auto& move : allLegalMoves) {
                    if (move.startSquare() == selectedSquareIdx) {
                        possibleMoves.push_back(move);
                    }
                }
                
                LOG_INFO(std::string("Selected piece at (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + "). Possible moves: " + std::to_string(possibleMoves.size()));
            } else {
                LOG_INFO("Clicked on opponent piece. No selection.");
            }
        } else {
            LOG_INFO("Clicked on empty square. No selection.");
        }
    }
}

void GameLogicBB::makeMove(const chess::BBMove& move, BoardBB& board) {
    // Get piece info before the move is made
    int startIdx = move.startSquare();
    int targetIdx = move.targetSquare();
    int movingPieceType = board.getPieceAt(startIdx / 8, startIdx % 8);
    
    if (movingPieceType == PIECE_NONE) {
        LOG_ERROR("Error: Attempted to make a move with no piece at start square.");
        return;
    }

    // Execute the move on the board
    LOG_INFO(std::string("Making move from square ") + std::to_string(startIdx) + " to " + std::to_string(targetIdx));
    board.executeMove(move, true);

    clearSelection();
    switchPlayer();
}

int GameLogicBB::getPieceAt(int row, int col, const BoardBB& board) const {
    return board.getPieceAt(row, col);
}

Color GameLogicBB::getCurrentPlayer() const {
    return currentPlayer;
}

const std::pair<int, int>* GameLogicBB::getSelectedPieceSquare() const {
    if (pieceIsSelected) {
        return &selectedPieceSquare;
    }
    return nullptr;
}

const std::vector<chess::BBMove>& GameLogicBB::getPossibleMoves() const {
    return possibleMoves;
}