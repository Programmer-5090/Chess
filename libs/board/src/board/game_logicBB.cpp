#include <chess/board/game_logicBB.h>
#include <chess/board/boardBB.h>
#include <chess/board/pieces/pieces.h>
#include <chess/board/bitboard/move.h>
#include <chess/board/pieces/piece_const.h>
#include <chess/board/bitboard/move_exec.h>
#include <chess/AI/ai_bb.h>
#include <chess/utils/logger.h>
#include <iostream>
#include <fstream>
#include <future>
#include <thread>
#include <atomic>

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
        if (pieceIsSelected) { 
            clearSelection(); 
        }
        return;
    }
    LOG_INFO(std::string("Clicked board square: (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + ")");

    if (pieceIsSelected) {
    bool validMoveClicked = false;
    // Convert UI row/col to bitboard square index (bitboard uses rank 0 = a1 at bottom)
    int selectedSquareIdx = (7 - selectedPieceSquare.first) * 8 + selectedPieceSquare.second;
    int clickedSquareIdx = (7 - r_clicked) * 8 + c_clicked;
        
        for (const auto& move : possibleMoves) {
            if (move.targetSquare() == clickedSquareIdx) {
                if (aiColor != NO_COLOR && currentPlayer == aiColor) {
                    LOG_INFO("Cannot make move - it's the AI's turn");
                    return;
                }
                
                LOG_INFO(std::string("Attempting to make move to (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + ")");
                
                makeMove(move, board);
                validMoveClicked = true;
                
                Color oppColor = (currentPlayer == WHITE) ? BLACK : WHITE;
                if (board.isCheckMate(oppColor)) {
                    std::string colorName = (oppColor == BLACK) ? "Black" : "White";
                    LOG_WARN(colorName + std::string(" is CHECKMATED"));
                }
                break;
            }
        }
        
        if (!validMoveClicked) {
            int pieceAtClickedSquare = board.getPieceAt(r_clicked, c_clicked);
            
            if (pieceAtClickedSquare != chess::PIECE_NONE) {
                int clickedPieceColor = chess::colorOf(pieceAtClickedSquare);
                int currentPlayerColor = (currentPlayer == WHITE) ? 8 : 16;
                
                if (clickedPieceColor == currentPlayerColor) {
                    clearSelection();
                    selectedPieceSquare = {r_clicked, c_clicked};
                    pieceIsSelected = true;
                    
                    std::vector<chess::BBMove> allLegalMoves = board.getAllLegalMoves(currentPlayer);
                    
                    for (const auto& move : allLegalMoves) {
                        if (move.startSquare() == clickedSquareIdx) {
                            possibleMoves.push_back(move);
                        }
                    }
                    
                    LOG_INFO(std::string("Selected new piece at (") + std::to_string(r_clicked) + ", " + std::to_string(c_clicked) + "). Possible moves: " + std::to_string(possibleMoves.size()));
                } else {
                    clearSelection();
                }
            } else {
                clearSelection();
            }
        }
    } else {
        int pieceAtSquare = board.getPieceAt(r_clicked, c_clicked);
        
    if (pieceAtSquare != chess::PIECE_NONE) {
            int pieceColor = chess::colorOf(pieceAtSquare);
            int currentPlayerColor = (currentPlayer == WHITE) ? 8 : 16;
            
            if (pieceColor == currentPlayerColor) {
                selectedPieceSquare = {r_clicked, c_clicked};
                pieceIsSelected = true;
                
                std::vector<chess::BBMove> allLegalMoves = board.getAllLegalMoves(currentPlayer);
                
                int selectedSquareIdx = (7 - r_clicked) * 8 + c_clicked;
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
    int startIdx = move.startSquare();
    int targetIdx = move.targetSquare();
    // Convert bitboard square index to UI row/col
    int startRank = startIdx / 8;
    int startFile = startIdx % 8;
    int startRow = 7 - startRank;
    int startCol = startFile;
    int movingPieceType = board.getPieceAt(startRow, startCol);
    
    if (movingPieceType == chess::PIECE_NONE) {
        LOG_ERROR("Error: Attempted to make a move with no piece at start square.");
        return;
    }

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

void GameLogicBB::update(BoardBB& board) {
    if (ai && aiColor != NO_COLOR) {
        Color current = getCurrentPlayer();
        
        if (current == aiColor) {
            if (board.isPromotionDialogActive()) return;

            if (!aiSearchRunning.load()) {
                aiSearchRunning.store(true);
                LOG_INFO("GameLogicBB: Starting AI search at depth " + std::to_string(aiSearchDepth));
                std::string currentFEN;
                try {
                    currentFEN = board.getCurrentFEN();
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] Failed to get FEN: " << e.what() << std::endl;
                    aiSearchRunning.store(false);
                    return;
                } catch (...) {
                    std::cerr << "[ERROR] Failed to get FEN: unknown exception" << std::endl;
                    aiSearchRunning.store(false);
                    return;
                }
                
                std::shared_ptr<AI_BB> aiPtr = ai;
                int depth = aiSearchDepth;
                
                aiFuture = std::async(std::launch::async, [aiPtr, currentFEN, depth]() -> std::pair<std::pair<chess::BBMove, int>, std::string> {
                    try {
                        BoardBB localBoard(100, 100, 30.0f);
                        localBoard.loadFEN(currentFEN, nullptr);
                        
                        // sequential search (parallel has issues with mate scores)
                        auto res = aiPtr->getSearchResult(localBoard, depth);
                        
                        LOG_INFO("GameLogicBB: AI search complete. Move value: " + std::to_string(res.first.value) + ", Eval: " + std::to_string(res.second));
                        return {res, currentFEN};
                    } catch (const std::exception& e) {
                        LOG_ERROR("GameLogicBB: AI search exception: " + std::string(e.what()));
                        return {{chess::BBMove(), 0}, currentFEN};
                    } catch (...) {
                        LOG_ERROR("GameLogicBB: AI search unknown exception");
                        return {{chess::BBMove(), 0}, currentFEN};
                    }
                });
            } else {
                // If future ready, get result and apply
                if (aiFuture.valid()) {
                    auto status = aiFuture.wait_for(std::chrono::milliseconds(0));
                    if (status == std::future_status::ready) {
                        auto resultWithFEN = aiFuture.get();
                        aiSearchRunning.store(false);
                        auto result = resultWithFEN.first;
                        auto fen = resultWithFEN.second;
                        chess::BBMove aiMove = result.first;
                        if (aiMove.value != 0 && aiMove.isValid()) {
                            if (board.getCurrentFEN() == fen) {
                                LOG_INFO("GameLogicBB: Applying AI move");
                                makeMove(aiMove, board);
                                return;
                            } else {
                                LOG_INFO("GameLogicBB: AI result ignored - board changed during search");
                            }
                        } else {
                            LOG_WARN("GameLogicBB: Invalid AI move received");
                        }
                    }
                }
            }
        }
    }
}

void GameLogicBB::setAISettings(int searchDepth, unsigned threadCount) {
    aiSearchDepth = searchDepth;
    aiThreadCount = threadCount;
}

void GameLogicBB::setAI(std::shared_ptr<AI_BB> aiInstance, Color aiColorIn) {
    ai = aiInstance;
    aiColor = aiColorIn;
}