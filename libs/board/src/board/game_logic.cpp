#include <chess/board/game_logic.h>
#include <chess/board/boardBB.h>
#include <chess/board/board.h>
#include <chess/board/pieces/pieces.h>
#include <chess/board/pieces/piece_const.h>
#include <chess/utils/logger.h>
#include <chess/board/move_executor.h>
#include <chess/types.h>
#include <chess/AI/ai.h>
#include <iostream>
#include <chrono>


GameLogic::GameLogic() : aiPlayer(NO_COLOR), 
                         pieceIsSelected(false), ai(nullptr) {
    selectedPieceSquare = {-1, -1};
}

GameLogic::~GameLogic() {
}

void GameLogic::switchPlayer(::Board& board) {
    ::Color newPlayer = (board.getCurrentPlayer() == WHITE) ? BLACK : WHITE;
    board.setCurrentPlayer(newPlayer);
    LOG_INFO(std::string("Player switched to: ") + (newPlayer == WHITE ? "WHITE" : "BLACK"));
}

void GameLogic::clearSelection() {
    pieceIsSelected = false;
    selectedPieceSquare = {-1, -1};
    possibleMoves.clear();
    LOG_INFO("Selection cleared.");
}

void GameLogic::handleMouseClick(int mouseX, int mouseY, ::Board& board, bool leftMouseClicked) {
    if (!leftMouseClicked) return;

    int r_clicked, c_clicked;
    if (!board.screenToBoardCoords(mouseX, mouseY, r_clicked, c_clicked)) {
        if (pieceIsSelected) { 
            clearSelection(); 
        }
        return;
    }
    LOG_INFO(std::string("Clicked board square: (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + ")");
    if (pieceIsSelected) {
        bool validMoveClicked = false;

        for (const auto& mv : possibleMoves) {
            int tr = mv.endPos.first;
            int tf = mv.endPos.second;
            if (tr == r_clicked && tf == c_clicked) {
                if (aiPlayer != NO_COLOR && board.getCurrentPlayer() == aiPlayer) {
                    LOG_INFO("Cannot make move - it's the AI's turn");
                    return;
                }
                
                LOG_INFO(std::string("Attempting to make move to (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + ")");
                makeMove(mv, board);
                validMoveClicked = true;
                break;
            }
        }

        if (!validMoveClicked) {
            Piece* piecePtr = board.getPieceAt(r_clicked, c_clicked);
            if (piecePtr != nullptr && piecePtr->getColor() == board.getCurrentPlayer()) {
                clearSelection();
                selectedPieceSquare = {r_clicked, c_clicked};
                pieceIsSelected = true;
                possibleMoves = board.getAllLegalMoves(board.getCurrentPlayer());
                int startRank = r_clicked;
                int startFile = c_clicked;
                std::vector<Move> filtered;
                for (const auto &mv : possibleMoves) {
                    int sr = mv.startPos.first;
                    int sf = mv.startPos.second;
                    if (sr == startRank && sf == startFile) filtered.push_back(mv);
                }
                possibleMoves.swap(filtered);
                LOG_INFO(std::string("Selected new piece at (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + "). Possible moves: " + std::to_string(possibleMoves.size()));
            } else {
                clearSelection();
            }
        }
    } else {
            Piece* piecePtr = board.getPieceAt(r_clicked, c_clicked);
        if (piecePtr != nullptr && piecePtr->getColor() == board.getCurrentPlayer()) {
            selectedPieceSquare = {r_clicked, c_clicked};
            pieceIsSelected = true;
            possibleMoves = board.getAllLegalMoves(board.getCurrentPlayer());
            int startRank = r_clicked;
            int startFile = c_clicked;
            std::vector<Move> filtered;
            for (const auto &mv : possibleMoves) {
                int sr = mv.startPos.first;
                int sf = mv.startPos.second;
                if (sr == startRank && sf == startFile) filtered.push_back(mv);
            }
            possibleMoves.swap(filtered);
            LOG_INFO(std::string("Selected piece at (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + "). Possible moves: " + std::to_string(possibleMoves.size()));
        } else {
            LOG_INFO("Clicked on empty or opponent piece. No selection.");
        }
    }
}

void GameLogic::makeMove(const Move& move, ::Board& board) {
    UndoMove u = board.executeMove(move, true);
    (void)u;
    clearSelection();
    switchPlayer(board);
    aiMovePending = false;
}

Piece* GameLogic::getPieceAt(int row, int col, const ::Board& board) const {
    return board.getPieceAt(row, col);
}

::Color GameLogic::getCurrentPlayer(const ::Board& board) const {
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
    if (ai) {
        LOG_INFO(std::string("AI will play as ") + (aiPlayer == WHITE ? "WHITE" : "BLACK"));
    } else {
        LOG_INFO("AI disabled");
        aiPlayer = NO_COLOR;
    }
}

bool GameLogic::isAITurn(const ::Board& board) const {
    ::Color currentPlayer = board.getCurrentPlayer();
    bool result = ai && (currentPlayer == aiPlayer);
    return result;
}

void GameLogic::update(::Board& board) {
    if (isAITurn(board) && !aiMovePending) {
        aiMovePending = true;
        makeAIMove(board);
    }
}

void GameLogic::makeAIMove(::Board& board) {
    if (!ai) {
        std::cout << "ERROR: No AI instance available\n" << std::flush;
        aiMovePending = false;
        return;
    }

    std::cout << "\nAI is thinking...\n" << std::flush;

    Move uiMove = ai->getBestMove(4);
    if (uiMove.startPos.first < 0 || uiMove.endPos.first < 0) {
        std::cout << "No legal moves available for AI\n" << std::flush;
        aiMovePending = false;
        return;
    }
    Move mv(uiMove.startPos, uiMove.endPos, nullptr, nullptr, CastlingType::NONE, uiMove.isPromotion, uiMove.promotionType);
    std::cout << "AI moves from (" << uiMove.startPos.first << "," << uiMove.startPos.second 
              << ") to (" << uiMove.endPos.first << "," << uiMove.endPos.second << ")\n" << std::flush;
    makeMove(mv, board);
    ai->printPerformanceStats();
}
