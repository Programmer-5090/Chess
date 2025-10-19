#include <chess/board/fen_util.h>
#include <chess/board/board.h>
#include <chess/board/pieces/pieces.h>
#include <chess/board/piece_manager.h>
#include <chess/utilities.h>
#include <unordered_map>
#include <cctype>
#include <sstream>
#include <vector>
#include <memory>

void FENUtil::loadFEN(const std::string& fen, Board& board, SDL_Renderer* gameRenderer) {
    std::unordered_map<char, std::pair<Color, PieceType>> pieceFromSymbol = {
        {'P', {WHITE, PAWN}}, {'p', {BLACK, PAWN}},
        {'R', {WHITE, ROOK}}, {'r', {BLACK, ROOK}},
        {'N', {WHITE, KNIGHT}}, {'n', {BLACK, KNIGHT}},
        {'B', {WHITE, BISHOP}}, {'b', {BLACK, BISHOP}},
        {'Q', {WHITE, QUEEN}}, {'q', {BLACK, QUEEN}},
        {'K', {WHITE, KING}}, {'k', {BLACK, KING}}
    };

    board.clearPieceGridAndPieces();

    std::string fenBoard = splitString(fen, ' ')[0];
    int row = 0, col = 0;
    for (char c : fenBoard) {
        if (c == '/') {
            row++;
            col = 0;
        } else if (isdigit(static_cast<unsigned char>(c))) {
            col += c - '0';
        } else {
            auto it = pieceFromSymbol.find(c);
            if (it != pieceFromSymbol.end()) {
                Color color = it->second.first;
                PieceType type = it->second.second;
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
                Piece* rawPtr = newPiece.get();
                board.getPieceManager()->addPiece(std::move(newPiece));
                board.pieceGrid[row][col] = rawPtr;
                col++;
            }
        }
    }

    board.setStartFEN(fen);

    std::vector<std::string> fenParts = splitString(fen, ' ');
    if (fenParts.size() >= 3) {
        std::string castlingRights = fenParts[2];

        for (auto& rowArr : board.pieceGrid) {
            for (Piece* piece : rowArr) {
                if (piece) {
                    if (piece->getType() == KING) {
                        King* king = static_cast<King*>(piece);
                        if (piece->getColor() == WHITE) {
                            king->setIsCastlingEligible(castlingRights.find('K') != std::string::npos ||
                                                       castlingRights.find('Q') != std::string::npos);
                        } else {
                            king->setIsCastlingEligible(castlingRights.find('k') != std::string::npos ||
                                                       castlingRights.find('q') != std::string::npos);
                        }
                    } else if (piece->getType() == ROOK) {
                        Rook* rook = static_cast<Rook*>(piece);
                        int prow = piece->getPosition().first;
                        int pcol = piece->getPosition().second;

                        if (piece->getColor() == WHITE && prow == 7) {
                            if (pcol == 0) {
                                rook->setIsCastlingEligible(castlingRights.find('Q') != std::string::npos);
                            } else if (pcol == 7) {
                                rook->setIsCastlingEligible(castlingRights.find('K') != std::string::npos);
                            } else {
                                rook->setIsCastlingEligible(false);
                            }
                        } else if (piece->getColor() == BLACK && prow == 0) {
                            if (pcol == 0) {
                                rook->setIsCastlingEligible(castlingRights.find('q') != std::string::npos);
                            } else if (pcol == 7) {
                                rook->setIsCastlingEligible(castlingRights.find('k') != std::string::npos);
                            } else {
                                rook->setIsCastlingEligible(false);
                            }
                        } else {
                            rook->setIsCastlingEligible(false);
                        }
                    }
                }
            }
        }
    }

    if (fenParts.size() >= 4) {
        std::string enPassantSquare = fenParts[3];

        for (auto& rowArr : board.pieceGrid) {
            for (Piece* piece : rowArr) {
                if (piece && piece->getType() == PAWN) {
                    static_cast<Pawn*>(piece)->setEnPassantCaptureEligible(false);
                }
            }
        }

        if (enPassantSquare != "-" && enPassantSquare.length() == 2) {
            int targetCol = enPassantSquare[0] - 'a';
            int targetRow = 8 - (enPassantSquare[1] - '0');

            int pawnRow = (targetRow == 2) ? 3 : 4;

            if (targetCol >= 0 && targetCol < 8 && pawnRow >= 0 && pawnRow < 8) {
                Piece* pawn = board.getPieceAt(pawnRow, targetCol);
                if (pawn && pawn->getType() == PAWN) {
                    static_cast<Pawn*>(pawn)->setEnPassantCaptureEligible(true);
                }
            }
        }
    }

    if (fenParts.size() >= 2) {
        std::string activeColor = fenParts[1];
        board.setCurrentPlayer((activeColor == "w") ? WHITE : BLACK);
    } else {
        board.setCurrentPlayer(WHITE);
    }

    if (fenParts.size() >= 5) {
        try {
            int half = std::stoi(fenParts[4]);
            board.halfMoveClock = half;
        } catch (const std::exception&) {
            board.halfMoveClock = 0;
        }
    } else {
        board.halfMoveClock = 0;
    }

    if (fenParts.size() >= 6) {
        try {
            int full = std::stoi(fenParts[5]);
            board.fullMoveNumber = full;
        } catch (const std::exception&) {
            board.fullMoveNumber = 1;
        }
    } else {
        board.fullMoveNumber = 1;
    }

}