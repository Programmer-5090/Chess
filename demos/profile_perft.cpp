#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include "board.h"

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
        auto startMakeUnmake = high_resolution_clock::now();
        UndoMove u;
        board.applyMoveWithUndo(mv, u);
        auto midMakeUnmake = high_resolution_clock::now();
        
        auto startLegality = high_resolution_clock::now();
        bool illegal = board.isKingInCheck(sideToMove);
        auto endLegality = high_resolution_clock::now();
        globalProfile.legalityCheckTime += duration_cast<microseconds>(endLegality - startLegality).count();
        
        if (!illegal) {
            nodes += profiledPerft(board, depth - 1, nextSide);
        }
        
        board.unmakeMove(mv, u);
        auto endMakeUnmake = high_resolution_clock::now();
        globalProfile.makeUnmakeTime += duration_cast<microseconds>(endMakeUnmake - midMakeUnmake).count();
        globalProfile.totalCalls++;
    }
    
    return nodes;
}

int SDL_main(int argc, char* argv[]) {
    // Initialize SDL (required for piece loading)
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed\n";
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("Profile", 0, 0, 100, 100, SDL_WINDOW_HIDDEN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    
    Board board(800, 800, 20.0f);
    board.startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    board.initializeBoard(renderer);
    
    std::cout << "Profiling perft depth 5...\n";
    
    auto start = high_resolution_clock::now();
    uint64_t nodes = profiledPerft(board, 5, WHITE);
    auto end = high_resolution_clock::now();
    
    long long totalTime = duration_cast<microseconds>(end - start).count();
    
    std::cout << "\n=== PERFORMANCE PROFILE ===\n";
    std::cout << "Total nodes: " << formatWithCommas(nodes) << "\n";
    std::cout << "Total time: " << totalTime / 1000.0 << " ms\n";
    std::cout << "Total function calls: " << formatWithCommas(globalProfile.totalCalls) << "\n\n";
    
    std::cout << "Time breakdown:\n";
    std::cout << "Move generation: " << globalProfile.moveGenTime / 1000.0 << " ms (" 
              << (100.0 * globalProfile.moveGenTime / totalTime) << "%)\n";
    std::cout << "Make/unmake: " << globalProfile.makeUnmakeTime / 1000.0 << " ms (" 
              << (100.0 * globalProfile.makeUnmakeTime / totalTime) << "%)\n";
    std::cout << "Legality checks: " << globalProfile.legalityCheckTime / 1000.0 << " ms (" 
              << (100.0 * globalProfile.legalityCheckTime / totalTime) << "%)\n";
    
    long long accountedTime = globalProfile.moveGenTime + globalProfile.makeUnmakeTime + globalProfile.legalityCheckTime;
    long long otherTime = totalTime - accountedTime;
    std::cout << "Other overhead: " << otherTime / 1000.0 << " ms (" 
              << (100.0 * otherTime / totalTime) << "%)\n\n";
    
    std::cout << "Performance metrics:\n";
    std::cout << "Nodes per second: " << formatWithCommas((nodes * 1000000) / totalTime) << "\n";
    std::cout << "Avg move gen time: " << (globalProfile.moveGenTime / (double)globalProfile.totalCalls) << " μs\n";
    std::cout << "Avg make/unmake time: " << (globalProfile.makeUnmakeTime / (double)globalProfile.totalCalls) << " μs\n";
    
    // Fine-grained make/unmake profiling
    std::cout << "\nMake/Unmake micro breakdown:\n";
    std::cout << "clearEnPassantFlags: " << (g_muProfile.clearEPTime / 1000.0) << " ms\n";
    std::cout << "Capture handling:    " << (g_muProfile.captureHandleTime / 1000.0) << " ms\n";
    std::cout << "Move piece:          " << (g_muProfile.movePieceTime / 1000.0) << " ms\n";
    std::cout << "Castling bookkeeping:" << (g_muProfile.castlingTime / 1000.0) << " ms\n";
    std::cout << "Unmake move back:    " << (g_muProfile.unmakeMoveBackTime / 1000.0) << " ms\n";
    std::cout << "Unmake restore cap:  " << (g_muProfile.unmakeRestoreCaptureTime / 1000.0) << " ms\n";
    std::cout << "Unmake castling:     " << (g_muProfile.unmakeCastlingTime / 1000.0) << " ms\n";
    std::cout << "Apply calls:         " << formatWithCommas(g_muProfile.applyCalls) << "\n";
    std::cout << "Unmake calls:        " << formatWithCommas(g_muProfile.unmakeCalls) << "\n";
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
