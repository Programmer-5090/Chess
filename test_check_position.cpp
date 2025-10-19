#include <chess/board/board.h>
#include <chess/board/bitboard/board_state.h>
#include <chess/board/bitboard/move_generator_bb.h>
#include <iostream>

int main() {
    // Position where white is in check
    std::string fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    
    Board board(800, 800, 50.0f);
    board.loadFEN(fen, nullptr);
    
    // Check using legacy
    bool legacyInCheck = board.isKingInCheck(WHITE);
    std::cout << "Legacy: King in check? " << (legacyInCheck ? "YES" : "NO") << "\n";
    
    // Generate moves using legacy
    auto legacyMoves = board.getAllPseudoLegalMoves(WHITE, true);
    std::cout << "Legacy pseudo-legal moves: " << legacyMoves.size() << "\n";
    
    // Now test bitboard
    chess::BitboardState bbState;
    bbState.loadFromFEN(fen);
    
    chess::MoveGeneratorBB gen;
    auto bbMoves = gen.generateMoves(bbState, false);
    
    std::cout << "Bitboard moves: " << bbMoves.size() << "\n";
    std::cout << "Bitboard in check? " << (gen.isInCheck(bbState) ? "YES" : "NO") << "\n";
    
    // Print first few bitboard moves
    std::cout << "\nFirst bitboard moves:\n";
    for (size_t i = 0; i < std::min(size_t(10), bbMoves.size()); ++i) {
        int from = bbMoves[i].startSquare();
        int to = bbMoves[i].targetSquare();
        char fromFile = 'a' + (from % 8);
        char fromRank = '1' + (from / 8);
        char toFile = 'a' + (to % 8);
        char toRank = '1' + (to / 8);
        std::cout << "  " << fromFile << fromRank << toFile << toRank << "\n";
    }
    
    return 0;
}
