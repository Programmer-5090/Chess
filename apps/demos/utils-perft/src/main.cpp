#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>

#include <chess/AI/utils.h>

struct TreeBoard {
    int branchFactor;
};

using TreeMove = int;

static std::vector<TreeMove> generateTreeMoves(const TreeBoard& b) {
    std::vector<TreeMove> moves;
    moves.reserve(b.branchFactor);
    for (int i = 0; i < b.branchFactor; ++i) moves.push_back(i);
    return moves;
}

static void makeTreeMove(TreeBoard& /*b*/, const TreeMove& /*m*/) {
}

static void unmakeTreeMove(TreeBoard& /*b*/, const TreeMove& /*m*/) {
}

static std::uint64_t ipow(std::uint64_t base, int exp) {
    std::uint64_t result = 1ULL;
    while (exp-- > 0) result *= base;
    return result;
}

int main() {
    using namespace chess;
    std::cout << "Testing formatWithCommas...\n";
    std::cout << "0 => " << formatWithCommas(0) << "\n";
    std::cout << "1234 => " << formatWithCommas(1234) << "\n";
    std::cout << "9876543210 => " << formatWithCommas(9876543210ULL) << "\n";
    TreeBoard board{3};
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
