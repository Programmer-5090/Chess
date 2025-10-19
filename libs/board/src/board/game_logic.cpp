#include <chess/board/game_logic.h>
#include <chess/board/board.h>
#include <chess/board/pieces/pieces.h>
#include <chess/utils/logger.h>
#include <chess/AI/ai.h>
#include <iostream>
#include <chrono>
#include <thread>

GameLogic::GameLogic() : aiPlayer(NO_COLOR), 
                         pieceIsSelected(false), ai(nullptr) {
    selectedPieceSquare = {-1, -1};
}

GameLogic::~GameLogic() {
    // AI will be automatically cleaned up by unique_ptr
}

void GameLogic::switchPlayer(Board& board) {
    Color newPlayer = (board.getCurrentPlayer() == WHITE) ? BLACK : WHITE;
    board.setCurrentPlayer(newPlayer);
    LOG_INFO(std::string("Player switched to: ") + (newPlayer == WHITE ? "WHITE" : "BLACK"));
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
                            std::cout << colorName << " is CHECKMATED" << std::endl;
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
            if (pieceAtClickedSquare && pieceAtClickedSquare->getColor() == board.getCurrentPlayer()) {
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
        if (piece && piece->getColor() == board.getCurrentPlayer()) {
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
    board.clearEnPassantFlags(board.getCurrentPlayer());

    // Get piece info before the move is made
    const Piece* movingPiece = move.piece;
    if (!movingPiece) {
        LOG_ERROR("Error: Attempted to make a move with a null piece.");
        return;
    }

    // Execute the move on the board
    LOG_INFO(std::string("Making move from (") + std::to_string(move.startPos.first) + "," + std::to_string(move.startPos.second)
              + ") to (" + std::to_string(move.endPos.first) + "," + std::to_string(move.endPos.second) + ")");
    UndoMove undoInfo = board.executeMove(move);

    // After the move, check if it was a two-square pawn push to set a new en passant flag
    if (movingPiece->getType() == PAWN && abs(move.startPos.first - move.endPos.first) == 2) {
        Piece* pieceAtDest = board.getPieceAt(move.endPos.first, move.endPos.second);
        if (pieceAtDest && pieceAtDest->getType() == PAWN) {
            static_cast<Pawn*>(pieceAtDest)->setEnPassantCaptureEligible(true);
            LOG_INFO(std::string("Pawn at (") + std::to_string(move.endPos.first) + "," + std::to_string(move.endPos.second) + ") is now en passant eligible.");
        }
    }

    clearSelection();
    switchPlayer(board);
}

Piece* GameLogic::getPieceAt(int row, int col, const Board& board) const {
    return board.getPieceAt(row, col);
}

Color GameLogic::getCurrentPlayer(const Board& board) const {
    return board.getCurrentPlayer();
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

void GameLogic::setAI(std::shared_ptr<AI> aiInstance, Color aiColor) {
    ai = aiInstance;
    aiPlayer = aiColor;
    
    // Debug logging removed for performance
    // std::cout << "DEBUG GameLogic::setAI: aiColor=" << aiColor 
    //           << " (" << (aiColor == WHITE ? "WHITE" : (aiColor == BLACK ? "BLACK" : "NO_COLOR")) << ")" << std::endl;
    
    if (ai) {
        LOG_INFO(std::string("AI will play as ") + (aiPlayer == WHITE ? "WHITE" : "BLACK"));
    } else {
        LOG_INFO("AI disabled");
        aiPlayer = NO_COLOR;
    }
}

bool GameLogic::isAITurn(const Board& board) const {
    Color currentPlayer = board.getCurrentPlayer();
    bool result = ai && (currentPlayer == aiPlayer);
    // Debug logging removed for performance
    // std::cout << "DEBUG isAITurn: ai=" << (ai ? "exists" : "null") 
    //           << ", currentPlayer=" << currentPlayer 
    //           << ", aiPlayer=" << aiPlayer 
    //           << ", result=" << result << std::endl;
    return result;
}

void GameLogic::update(Board& board) {
    if (isAITurn(board)) {
        makeAIMove(board);
    }
}

void GameLogic::makeAIMove(Board& board) {
    if (!ai) {
        std::cout << "ERROR: No AI instance available\n" << std::flush;
        return;
    }

    std::cout << "\nAI is thinking...\n" << std::flush;
    
    // Add a small delay so the move isn't instant
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Get the AI's best move with performance tracking
    Move bestMove = ai->getBestMove(4); // Search depth of 3 - good balance
    
    // Check if we got a valid move
    if (bestMove.piece == nullptr) {
        std::cout << "No legal moves available for AI\n" << std::flush;
        return;
    }

    // Make the AI's move
    std::cout << "AI moves from (" << bestMove.startPos.first << "," << bestMove.startPos.second 
              << ") to (" << bestMove.endPos.first << "," << bestMove.endPos.second << ")\n" << std::flush;
    
    makeMove(bestMove, board);
    
    // Print performance statistics to terminal
    ai->printPerformanceStats();
}
