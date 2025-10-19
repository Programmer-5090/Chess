#include <chess/utils/fen_util.h>
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
    // FEN character to piece type/color mapping for piece placement parsing
    std::unordered_map<char, std::pair<Color, PieceType>> pieceFromSymbol = {
        {'P', {WHITE, PAWN}}, {'p', {BLACK, PAWN}},
        {'R', {WHITE, ROOK}}, {'r', {BLACK, ROOK}},
        {'N', {WHITE, KNIGHT}}, {'n', {BLACK, KNIGHT}},
        {'B', {WHITE, BISHOP}}, {'b', {BLACK, BISHOP}},
        {'Q', {WHITE, QUEEN}}, {'q', {BLACK, QUEEN}},
        {'K', {WHITE, KING}}, {'k', {BLACK, KING}}
    };

    board.clearPieceGridAndPieces();

    // Parse piece placement (field 1): iterate through FEN board representation
    std::string fenBoard = splitString(fen, ' ')[0];
    int row = 0, col = 0;
    for (char c : fenBoard) {
        if (c == '/') {
            row++;  // Move to next rank
            col = 0;
        } else if (isdigit(static_cast<unsigned char>(c))) {
            col += c - '0'; // Skip empty squares (digit represents count)
        } else {
            auto it = pieceFromSymbol.find(c);
            if (it != pieceFromSymbol.end()) {
                Color color = it->second.first;
                PieceType type = it->second.second;
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
                board.getPieceManager()->addPiece(std::move(newPiece));
                // set in the board's internal grid via friend access
                board.pieceGrid[row][col] = rawPtr;
                col++;
            }
        }
    }

    board.setStartFEN(fen);

    // Parse castling rights from FEN (field 3): K=White kingside, Q=White queenside, k=Black kingside, q=Black queenside
    std::vector<std::string> fenParts = splitString(fen, ' ');
    if (fenParts.size() >= 3) {
        std::string castlingRights = fenParts[2];

        // Set castling eligibility for kings and rooks based on FEN rights string
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

    // Parse en passant target square from FEN (field 4): algebraic notation or "-" for none
    if (fenParts.size() >= 4) {
        std::string enPassantSquare = fenParts[3];

        // Reset all pawn en passant flags before setting new ones
        for (auto& rowArr : board.pieceGrid) {
            for (Piece* piece : rowArr) {
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
            int pawnRow = (targetRow == 2) ? 3 : 4;

            if (targetCol >= 0 && targetCol < 8 && pawnRow >= 0 && pawnRow < 8) {
                Piece* pawn = board.getPieceAt(pawnRow, targetCol);
                if (pawn && pawn->getType() == PAWN) {
                    static_cast<Pawn*>(pawn)->setEnPassantCaptureEligible(true);
                }
            }
        }
    }

    // Parse active color from FEN (field 2): "w" = White to move, "b" = Black to move
    if (fenParts.size() >= 2) {
        std::string activeColor = fenParts[1];
        board.setCurrentPlayer((activeColor == "w") ? WHITE : BLACK);
    } else {
        board.setCurrentPlayer(WHITE);  // Default to White if field missing
    }

    // Parse halfmove clock from FEN (field 5): moves since last pawn move or capture for 50-move rule
    if (fenParts.size() >= 5) {
        try {
            int half = std::stoi(fenParts[4]);
            // direct member access because friend class
            board.halfMoveClock = half;
        } catch (const std::exception&) {
            board.halfMoveClock = 0;  // Reset to 0 if invalid format
        }
    } else {
        board.halfMoveClock = 0;  // Default if field missing
    }

    // Parse fullmove number from FEN (field 6): increments after each Black move, starts at 1
    if (fenParts.size() >= 6) {
        try {
            int full = std::stoi(fenParts[5]);
            board.fullMoveNumber = full;
        } catch (const std::exception&) {
            board.fullMoveNumber = 1;  // Reset to 1 if invalid format
        }
    } else {
        board.fullMoveNumber = 1;  // Default if field missing
    }

}