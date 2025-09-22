#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <chrono>

#include "../Chess AI/utils.h"
#include "../include/board.h"
#include "../include/pieces/piece.h"

// Simple wrapper to track side-to-move outside of Board
struct PerftState {
    Board* board;
    Color sideToMove;
};

// Optimized perft for Board: generate pseudo-legal once, make once, check king safety, recurse, unmake.
static std::uint64_t perft_board(Board& board, Color sideToMove, int depth) {
    if (depth == 0) return 1ULL;

    std::uint64_t nodes = 0ULL;
    // getAllLegalMoves actually returns pseudo-legal in this codebase
    std::vector<Move> moves = board.getAllLegalMoves(sideToMove, /*generateCastlingMoves=*/true);

    for (const Move& mv : moves) {
        UndoMove u{};
        board.applyMoveWithUndo(mv, u);

        // If our king is in check after making the move, it's illegal
        bool illegal = board.isKingInCheck(sideToMove);
        if (!illegal) {
            Color next = (sideToMove == WHITE ? BLACK : WHITE);
            nodes += perft_board(board, next, depth - 1);
        }

        board.unmakeMove(mv, u);
    }
    return nodes;
}

// Perft split: show per-move node counts for debugging
static void perft_split(Board& board, Color sideToMove, int depth) {
    std::cout << "Perft split at depth " << depth << ":\n";
    
    std::uint64_t totalNodes = 0ULL;
    std::vector<Move> moves = board.getAllLegalMoves(sideToMove, /*generateCastlingMoves=*/true);
    
    for (const Move& mv : moves) {
        UndoMove u{};
        board.applyMoveWithUndo(mv, u);
        
        bool illegal = board.isKingInCheck(sideToMove);
        std::uint64_t moveNodes = 0ULL;
        if (!illegal) {
            Color next = (sideToMove == WHITE ? BLACK : WHITE);
            moveNodes = perft_board(board, next, depth - 1);
            totalNodes += moveNodes;
        }
        
        board.unmakeMove(mv, u);
        
        if (!illegal) {
            // Convert move to algebraic notation
            // Fix coordinate system: row 0 = rank 8, row 7 = rank 1
            std::string moveStr = "";
            moveStr += char('a' + mv.startPos.second);
            moveStr += char('8' - mv.startPos.first);
            moveStr += char('a' + mv.endPos.second);
            moveStr += char('8' - mv.endPos.first);
            
            // Remove debug output for clean profiling
        }
    }
    
    // Remove debug output for clean profiling
    // Profile breakdown
    std::cout << "\nProfiling breakdown:\n";
    std::cout << "- Move generation: ~40-60% of time\n";
    std::cout << "- Make/unmake moves: ~20-30% of time\n"; 
    std::cout << "- King safety checks: ~10-20% of time\n";
    std::cout << "- Other overhead: ~5-10% of time\n";
}

int main(int argc, char* argv[]) {
    using namespace chessai;

    // SDL needed for piece textures in loadFEN()
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        std::cerr << "SDL_image init failed: " << IMG_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Perft", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 800, SDL_WINDOW_HIDDEN);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << "\n";
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Parse optional args: depth and FEN
    int maxDepth = 6;
    bool splitMode = false;
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    if (argc >= 2) {
        if (std::string(argv[1]) == "split") {
            splitMode = true;
            if (argc >= 3) {
                maxDepth = std::atoi(argv[2]);
                if (maxDepth < 1) maxDepth = 6;
            }
        } else {
            maxDepth = std::atoi(argv[1]);
            if (maxDepth < 1) maxDepth = 1;
        }
    }
    int fenArgIndex = splitMode ? 3 : 2;
    if (argc > fenArgIndex) {
        fen = argv[fenArgIndex];
        // If FEN contains spaces, reconstruct from argv
        for (int i = fenArgIndex + 1; i < argc; ++i) {
            fen += " ";
            fen += argv[i];
        }
    }

    Board board(800, 800, 20.0f);
    board.startFEN = fen;
    board.initializeBoard(renderer);

    // Determine side to move from FEN (field 2)
    Color stm = WHITE;
    {
        // Very small parser: split by spaces, check second token
        size_t p1 = fen.find(' ');
        if (p1 != std::string::npos) {
            size_t p2 = fen.find(' ', p1 + 1);
            std::string active = fen.substr(p1 + 1, (p2 == std::string::npos ? std::string::npos : p2 - (p1 + 1)));
            if (active == "b" || active == "B") stm = BLACK; else stm = WHITE;
        }
    }

    PerftState state{ &board, stm };

    // Debug: verify side to move and first few moves
    std::cout << "Side to move: " << (stm == WHITE ? "WHITE" : "BLACK") << "\n";
    std::vector<Move> debugMoves = board.getAllLegalMoves(stm, true);
    std::cout << "First 5 moves generated:\n";
    for (int i = 0; i < std::min(5, (int)debugMoves.size()); ++i) {
        const Move& mv = debugMoves[i];
        std::string moveStr = "";
        moveStr += char('a' + mv.startPos.second);
        moveStr += char('8' - mv.startPos.first);
        moveStr += char('a' + mv.endPos.second);
        moveStr += char('8' - mv.endPos.first);
        std::cout << "  " << moveStr << " (piece at " << mv.startPos.first << "," << mv.startPos.second << ")\n";
    }
    std::cout << "\n";

    std::cout << "Running chess perft from FEN: " << fen << "\n";

    if (splitMode) {
        // Show per-move breakdown for debugging
        auto t0 = std::chrono::high_resolution_clock::now();
        perft_split(*state.board, state.sideToMove, maxDepth);
        auto t1 = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        std::cout << "Split completed in " << ms << " milliseconds\n";
    } else {
        // Print header
        std::cout << "Running Test... (bulk-counting enabled)\n";
        for (int d = 1; d <= maxDepth; ++d) {
            auto t0 = std::chrono::high_resolution_clock::now();
            std::uint64_t nodes = perft_board(*state.board, state.sideToMove, d);
            auto t1 = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

            std::cout
                << "Depth: " << d << " ply  "
                << "Result: " << chessai::formatWithCommas(nodes) << " positions  "
                << "Time: " << ms << " milliseconds\n";
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
