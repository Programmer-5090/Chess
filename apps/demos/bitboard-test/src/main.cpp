#include <chess/board/board.h>
#include <chess/board/boardBB.h>
#include <chess/utils/logger.h>
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace std::chrono;

void compareMoveCounts(const std::string& position, const std::string& description) {
    Board board(800, 800, 50.0f);
    board.loadFEN(position, nullptr);
    
    Color color = board.getCurrentPlayer();
    
    std::cout << "Current player: " << (color == WHITE ? "WHITE" : "BLACK") << "\n";
    
    auto startLegacy = high_resolution_clock::now();
    std::vector<Move> legacyMoves = board.getAllPseudoLegalMoves(color, true);
    auto endLegacy = high_resolution_clock::now();
    auto durationLegacy = duration_cast<microseconds>(endLegacy - startLegacy);
    
    BoardBB bbBoard(100, 100, 30.0f);
    bbBoard.loadFEN(position, nullptr);
    auto startBB = high_resolution_clock::now();
    std::vector<chess::BBMove> bbMoves = bbBoard.getAllLegalMoves(color);
    auto endBB = high_resolution_clock::now();
    auto durationBB = duration_cast<microseconds>(endBB - startBB);
    
    std::cout << "\nResults:\n";
    std::cout << "  Legacy moves:   " << std::setw(4) << legacyMoves.size() 
              << " (" << std::setw(6) << durationLegacy.count() << " μs)\n";
    std::cout << "  Bitboard moves: " << std::setw(4) << bbMoves.size() 
              << " (" << std::setw(6) << durationBB.count() << " μs)\n";
    
    bool countMatch = (legacyMoves.size() == bbMoves.size());
    std::cout << "\n  Move count match: " << (countMatch ? "✓ PASS" : "✗ FAIL") << "\n";
    
    if (durationLegacy.count() > 0) {
        double speedup = static_cast<double>(durationLegacy.count()) / durationBB.count();
        std::cout << "  Speedup: " << std::fixed << std::setprecision(2) << speedup << "x\n";
    }
    
    std::cout << "\n  First 10 legacy moves:\n";
    for (size_t i = 0; i < std::min(size_t(10), legacyMoves.size()); ++i) {
        const auto& m = legacyMoves[i];
        std::cout << "    " << char('a' + m.startPos.second) << (m.startPos.first + 1)
                  << char('a' + m.endPos.second) << (m.endPos.first + 1);
        if (m.isPromotion) std::cout << "=Q";
        std::cout << "\n";
    }
        
    std::cout << "\n  First 10 bitboard moves:\n";
    for (size_t i = 0; i < std::min(size_t(10), bbMoves.size()); ++i) {
        const auto& m = bbMoves[i];
        int s = m.startSquare();
        int t = m.targetSquare();
        int sr = s / 8, sc = s % 8;
        int tr = t / 8, tc = t % 8;
        std::cout << "    " << char('a' + sc) << (8 - sr)
                  << char('a' + tc) << (8 - tr);
        if (m.isPromotion()) std::cout << "=Q";
        std::cout << "\n";
    }
    return;
}

int main(int argc, char* argv[]) {
    
    std::cout << "==============================================\n";
    std::cout << " Bitboard vs Legacy Move Generation Test\n";
    std::cout << "==============================================\n";
    
    // Test positions - each gets a fresh Board object
    compareMoveCounts( 
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Initial Position");
    
    compareMoveCounts(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "Kiwipete Position");
    
    compareMoveCounts(
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "Tricky Position (pins)");
    
    compareMoveCounts(
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "Complex Position");
    
    compareMoveCounts(
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "Position 4");
    
    compareMoveCounts(
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "Middle Game Position");
    
    std::cout << "\n==============================================\n";
    std::cout << " Test Complete!\n";
    std::cout << "==============================================\n";
    
    return 0;
}
