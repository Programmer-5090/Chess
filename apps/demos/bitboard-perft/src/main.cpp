#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cstdint>
#include <algorithm>
#include <future>
#include <mutex>

#include <chess/board/bitboard/board_state.h>
#include <chess/board/bitboard/move_generator_bb.h>
#include <chess/board/bitboard/move_exec.h>
#include <chess/utils/thread_pool.h>

using namespace chess;
using Clock = std::chrono::high_resolution_clock;

static bool g_enableBulkCount = true;
static std::string onlyMoveGlobal = "";

static std::string moveToString(const BBMove& mv) {
    int s = mv.startSquare();
    int t = mv.targetSquare();
    int sf = toCol(s), sr = toRow(s);
    int tf = toCol(t), tr = toRow(t);
    std::string out;
    out.reserve(5);
    out.push_back(char('a' + sf));
    out.push_back(char('1' + sr));
    out.push_back(char('a' + tf));
    out.push_back(char('1' + tr));
    if (mv.isPromotion()) {
        switch (mv.flag()) {
            case BBMove::PromoteToQueen:  out.push_back('q'); break;
            case BBMove::PromoteToRook:   out.push_back('r'); break;
            case BBMove::PromoteToBishop: out.push_back('b'); break;
            case BBMove::PromoteToKnight: out.push_back('n'); break;
            default: break;
        }
    }
    return out;
}

static std::uint64_t perftRecursive(BitboardState& state, MoveGeneratorBB& gen, BBMoveExecutor& exec, int depth) {
    if (depth == 0) return 1ULL;
    std::vector<BBMove> moves = gen.generateMoves(state, false);
    if (depth == 1) return static_cast<std::uint64_t>(moves.size());
    std::uint64_t nodes = 0ULL;
    for (const auto& mv : moves) {
        UndoState u = exec.makeMove(mv);
        nodes += perftRecursive(state, gen, exec, depth - 1);
        exec.unmakeMove(mv, u);
    }
    return nodes;
}

static std::uint64_t perft(BitboardState& state, int depth) {
    MoveGeneratorBB gen;
    BBMoveExecutor exec(state);
    return perftRecursive(state, gen, exec, depth);
}

static std::uint64_t perftSplit(BitboardState& state, int depth) {
    MoveGeneratorBB gen;
    BBMoveExecutor exec(state);
    std::vector<BBMove> moves = gen.generateMoves(state, false);
    std::uint64_t total = 0ULL;
    
    for (const auto& mv : moves) {
        UndoState u = exec.makeMove(mv);
        std::uint64_t n = perft(state, depth - 1);
        exec.unmakeMove(mv, u);
        total += n;
        std::cout << moveToString(mv) << ": " << n << std::endl;
    }
    return total;
}

// Multi-threaded split perft: each root move runs on separate thread
static std::uint64_t perftSplitMT(const BitboardState& rootState, int depth, int maxThreads) {
    MoveGeneratorBB gen;
    BitboardState tempState = rootState;
    std::vector<BBMove> moves = gen.generateMoves(tempState, false);
    
    if (moves.empty()) return 0ULL;
    std::vector<BBMove> filteredMoves;
    for (const auto& mv : moves) {
        if (onlyMoveGlobal.empty() || moveToString(mv) == onlyMoveGlobal) {
            filteredMoves.push_back(mv);
        }
    }
    if (filteredMoves.empty()) return 0ULL;
    
    int threads = (maxThreads > 0) ? std::min(maxThreads, (int)filteredMoves.size()) : (int)filteredMoves.size();
    ThreadPool pool(threads);
    std::mutex coutMutex;
    std::vector<std::future<std::uint64_t>> futures;
    futures.reserve(filteredMoves.size());
    
    for (const auto& mv : filteredMoves) {
        futures.emplace_back(pool.enqueue([&rootState, mv, depth, &coutMutex]() -> std::uint64_t {
            BitboardState freshState = rootState;
            MoveGeneratorBB gen;
            BBMoveExecutor exec(freshState);
            
            // Find and execute the move
            std::vector<BBMove> freshMoves = gen.generateMoves(freshState, false);
            for (const auto& fm : freshMoves) {
                if (fm.startSquare() == mv.startSquare() && 
                    fm.targetSquare() == mv.targetSquare() &&
                    fm.flag() == mv.flag()) {
                    UndoState u = exec.makeMove(fm);
                    std::uint64_t moveNodes = perft(freshState, depth - 1);
                    exec.unmakeMove(fm, u);
                    
                    {
                        std::lock_guard<std::mutex> lk(coutMutex);
                        std::cout << moveToString(fm) << ": " << moveNodes << std::endl;
                    }
                    return moveNodes;
                }
            }
            return 0ULL;
        }));
    }
    
    std::uint64_t totalNodes = 0ULL;
    for (auto& fut : futures) {
        totalNodes += fut.get();
    }
    return totalNodes;
}

// Multi-threaded perft: parallelizes root moves without per-move output
static std::uint64_t perftMT(const BitboardState& rootState, int depth, int maxThreads) {
    if (depth <= 1) {
        BitboardState tempState = rootState;
        return perft(tempState, depth);
    }
    
    MoveGeneratorBB gen;
    BitboardState tempState = rootState;
    std::vector<BBMove> moves = gen.generateMoves(tempState, false);
    
    if (moves.empty()) return 0ULL;
    
    // Filter moves if onlyMoveGlobal is specified
    std::vector<BBMove> filteredMoves;
    for (const auto& mv : moves) {
        if (onlyMoveGlobal.empty() || moveToString(mv) == onlyMoveGlobal) {
            filteredMoves.push_back(mv);
        }
    }
    if (filteredMoves.empty()) return 0ULL;
    
    int threads = (maxThreads > 0) ? std::min(maxThreads, (int)filteredMoves.size()) : (int)filteredMoves.size();
    ThreadPool pool(threads);
    std::vector<std::future<std::uint64_t>> futures;
    futures.reserve(filteredMoves.size());
    
    for (const auto& mv : filteredMoves) {
        futures.emplace_back(pool.enqueue([&rootState, mv, depth]() -> std::uint64_t {
            BitboardState freshState = rootState;
            MoveGeneratorBB gen;
            BBMoveExecutor exec(freshState);
            
            // Find and execute the move
            std::vector<BBMove> freshMoves = gen.generateMoves(freshState, false);
            for (const auto& fm : freshMoves) {
                if (fm.startSquare() == mv.startSquare() && 
                    fm.targetSquare() == mv.targetSquare() &&
                    fm.flag() == mv.flag()) {
                    UndoState u = exec.makeMove(fm);
                    std::uint64_t moveNodes = perft(freshState, depth - 1);
                    exec.unmakeMove(fm, u);
                    return moveNodes;
                }
            }
            return 0ULL;
        }));
    }
    
    std::uint64_t totalNodes = 0ULL;
    for (auto& fut : futures) {
        totalNodes += fut.get();
    }
    return totalNodes;
}

static bool isNumber(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

int main(int argc, char* argv[]) {
    int maxDepth = 4;
    bool splitMode = false;
    int maxThreads = 0; // 0 = use all available
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "split") {
            splitMode = true;
            if (i + 1 < argc && isNumber(argv[i + 1])) {
                maxDepth = std::max(1, std::atoi(argv[i + 1]));
                ++i;
            }
        } else if (arg == "--threads" || arg == "-t") {
            if (i + 1 < argc && isNumber(argv[i + 1])) {
                maxThreads = std::max(1, std::atoi(argv[i + 1]));
                ++i;
            }
        } else if (arg == "--only") {
            if (i + 1 < argc) {
                onlyMoveGlobal = argv[i + 1];
                ++i;
            }
        } else if (isNumber(arg)) {
            maxDepth = std::max(1, std::atoi(arg.c_str()));
        } else if (arg.rfind("--", 0) != 0) {
            fen = arg;
            while (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) {
                fen += " ";
                fen += argv[++i];
            }
        }
    }

    BitboardState state;
    state.clear();
    state.loadFromFEN(fen);
    
    std::cout << "FEN: " << fen << std::endl;
    if (maxThreads > 0) {
        std::cout << "Using " << maxThreads << " threads" << std::endl;
    }
    if (!onlyMoveGlobal.empty()) {
        std::cout << "Filtering for move: " << onlyMoveGlobal << std::endl;
    }
    std::cout << std::endl;

    if (splitMode) {
        auto t0 = Clock::now();
        std::uint64_t nodes;
        if (maxThreads > 0) {
            nodes = perftSplitMT(state, maxDepth, maxThreads);
        } else {
            nodes = perftSplit(state, maxDepth);
        }
        auto t1 = Clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        std::cout << "\nSplit completed in " << ms << " milliseconds\nNodes searched: " << nodes << std::endl;
    } else {
        for (int d = 1; d <= maxDepth; ++d) {
            auto t0 = Clock::now();
            std::uint64_t nodes;
            if (maxThreads > 0 && d >= 4) {
                    nodes = perftMT(state, d, maxThreads);
            } else {
                BitboardState s = state;
                nodes = perft(s, d);
            }
            auto t1 = Clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
            std::cout << "Depth: " << d << " ply  Result: " << nodes << " positions  Time: " << ms << " milliseconds" << std::endl;
        }
    }

    return 0;
}
