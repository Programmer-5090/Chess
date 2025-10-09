#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <chess/board/board.h>
#include <chess/utils/logger.h>

using namespace std::chrono;

// Simple number formatting function
std::string formatWithCommas(uint64_t number) {
    std::string str = std::to_string(number);
    int insertPosition = (int)str.length() - 3;
    while (insertPosition > 0) {
        str.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return str;
}

struct ProfileData {
    long long moveGenTime = 0;
    long long makeUnmakeTime = 0;
    long long legalityCheckTime = 0;
    long long totalCalls = 0;
};

ProfileData globalProfile;

uint64_t profiledPerft(Board& board, int depth, Color sideToMove) {
    if (depth == 0) return 1;
    
    auto startMoveGen = high_resolution_clock::now();
    std::vector<Move> moves = board.getAllLegalMoves(sideToMove, true);
    auto endMoveGen = high_resolution_clock::now();
    globalProfile.moveGenTime += duration_cast<microseconds>(endMoveGen - startMoveGen).count();
    
    uint64_t nodes = 0;
    Color nextSide = (sideToMove == WHITE ? BLACK : WHITE);
    
    for (const Move& mv : moves) {
        // Legality check without mutating permanent board state
        auto startLegality = high_resolution_clock::now();
        bool illegal = board.isKingInCheck(sideToMove, &mv);
        auto endLegality = high_resolution_clock::now();
        globalProfile.legalityCheckTime += duration_cast<microseconds>(endLegality - startLegality).count();

        if (!illegal) {
            UndoMove u = board.executeMove(mv);
            nodes += profiledPerft(board, depth - 1, nextSide);
            board.undoMove(mv, u);
            globalProfile.totalCalls++;
        }
    }
    
    return nodes;
}

int SDL_main(int argc, char* argv[]) {
    // Initialize SDL (required for piece loading)
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Logger::log(LogLevel::ERROR, "SDL init failed", __FILE__, __LINE__);
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Profile", 0, 0, 100, 100, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    
    Board board(800, 800, 20.0f);
    board.setStartFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.initializeBoard(renderer);
    
    Logger::log(LogLevel::INFO, "Profiling perft depth 5...", __FILE__, __LINE__);
    
    auto start = high_resolution_clock::now();
    uint64_t nodes = profiledPerft(board, 5, WHITE);
    auto end = high_resolution_clock::now();
    
    long long totalTime = duration_cast<microseconds>(end - start).count();
    
    Logger::log(LogLevel::INFO, "\n=== PERFORMANCE PROFILE ===", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Total nodes: ") + formatWithCommas(nodes), __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Total time: ") + std::to_string(totalTime / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Total function calls: ") + formatWithCommas(globalProfile.totalCalls), __FILE__, __LINE__);

    Logger::log(LogLevel::INFO, "Time breakdown:", __FILE__, __LINE__);
    double makeUnmakeMs = (g_muProfile.applyTime + g_muProfile.unmakeTime) / 1000.0;
    Logger::log(LogLevel::INFO, std::string("Move generation: ") + std::to_string(globalProfile.moveGenTime / 1000.0) + " ms (" + std::to_string(100.0 * globalProfile.moveGenTime / totalTime) + "%)", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Make/unmake: ") + std::to_string(makeUnmakeMs) + " ms (" + std::to_string(100.0 * (g_muProfile.applyTime + g_muProfile.unmakeTime) / totalTime) + "%)", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Legality checks: ") + std::to_string(globalProfile.legalityCheckTime / 1000.0) + " ms (" + std::to_string(100.0 * globalProfile.legalityCheckTime / totalTime) + "%)", __FILE__, __LINE__);

    long long accountedTime = globalProfile.moveGenTime + g_muProfile.applyTime + g_muProfile.unmakeTime + globalProfile.legalityCheckTime;
    long long otherTime = totalTime - accountedTime;
    Logger::log(LogLevel::INFO, std::string("Other overhead: ") + std::to_string(otherTime / 1000.0) + " ms (" + std::to_string(100.0 * otherTime / totalTime) + "%)", __FILE__, __LINE__);

    Logger::log(LogLevel::INFO, "Performance metrics:", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Nodes per second: ") + formatWithCommas((nodes * 1000000) / totalTime), __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Avg move gen time: ") + std::to_string(globalProfile.moveGenTime / (double)globalProfile.totalCalls) + " μs", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Avg make/unmake time: ") + std::to_string((g_muProfile.applyTime + g_muProfile.unmakeTime) / (double)globalProfile.totalCalls) + " μs", __FILE__, __LINE__);

    // Fine-grained make/unmake profiling (updated field names)
    Logger::log(LogLevel::INFO, "\nMake/Unmake micro breakdown:", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("clearEnPassantFlags: ") + std::to_string(g_muProfile.clearEnPassantFlags / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Capture handling:    ") + std::to_string(g_muProfile.captureHandling / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Move piece:          ") + std::to_string(g_muProfile.movePiece / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Castling bookkeeping:") + std::to_string(g_muProfile.castlingBookkeeping / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Promotion handling:  ") + std::to_string(g_muProfile.promotionHandling / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Unmake move back:    ") + std::to_string(g_muProfile.unmakeMoveBack / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Unmake restore cap:  ") + std::to_string(g_muProfile.unmakeRestoreCap / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Unmake castling:     ") + std::to_string(g_muProfile.unmakeCastling / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Apply time:          ") + std::to_string(g_muProfile.applyTime / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Unmake time:         ") + std::to_string(g_muProfile.unmakeTime / 1000.0) + " ms", __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Apply calls:         ") + formatWithCommas(g_muProfile.applyCalls), __FILE__, __LINE__);
    Logger::log(LogLevel::INFO, std::string("Unmake calls:        ") + formatWithCommas(g_muProfile.unmakeCalls), __FILE__, __LINE__);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
