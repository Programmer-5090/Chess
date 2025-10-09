#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>

// Use relative include to the utils header
#include "../Chess AI/utils.h"

// A minimal dummy board representing a k-ary tree of moves.
// This is purely to validate perft/runPerft utilities without SDL or chess engine.
struct TreeBoard {
    int branchFactor;
};

using TreeMove = int; // Moves are just 0..branchFactor-1 labels

static std::vector<TreeMove> generateTreeMoves(const TreeBoard& b) {
    std::vector<TreeMove> moves;
    moves.reserve(b.branchFactor);
    for (int i = 0; i < b.branchFactor; ++i) moves.push_back(i);
    return moves;
}

static void makeTreeMove(TreeBoard& /*b*/, const TreeMove& /*m*/) {
    // No state to mutate for this dummy board
}

static void unmakeTreeMove(TreeBoard& /*b*/, const TreeMove& /*m*/) {
    // No state to revert for this dummy board
}

static std::uint64_t ipow(std::uint64_t base, int exp) {
    std::uint64_t result = 1ULL;
    while (exp-- > 0) result *= base;
    return result;
}

int main() {
    using namespace chessai;

    // 1) formatWithCommas sanity checks
    std::cout << "Testing formatWithCommas...\n";
    std::cout << "0 => " << formatWithCommas(0) << "\n";
    std::cout << "1234 => " << formatWithCommas(1234) << "\n";
    std::cout << "9876543210 => " << formatWithCommas(9876543210ULL) << "\n";

    // 2) perft on a dummy k-ary tree: nodes(d) = k^d
    TreeBoard board{3}; // branching factor k = 3

    for (int d = 0; d <= 6; ++d) {
        std::uint64_t nodes = perft<TreeBoard, TreeMove>(
            board,
            d,
            generateTreeMoves,
            makeTreeMove,
            unmakeTreeMove
        );
        std::uint64_t expected = ipow(3ULL, d);
        std::cout << "Depth " << d << ": nodes = " << nodes
                  << ", expected = " << expected << "\n";
        assert(nodes == expected && "perft should equal k^d for a k-ary tree");
    }

    // 3) runPerft pretty printer (starts at depth 1)
    std::cout << "\nrunPerft on a 3-ary tree (depth 1..6)\n";
    runPerft<TreeBoard, TreeMove>(
        board,
        6,
        generateTreeMoves,
        makeTreeMove,
        unmakeTreeMove,
        /*showHeader=*/true
    );

    std::cout << "\nAll utils.h tests completed successfully.\n" << std::endl;
    return 0;
}
