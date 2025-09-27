#ifndef CHESS_AI_UTILS_H
#define CHESS_AI_UTILS_H

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "../include/logger.h"

namespace chessai {

// Formats an integer with thousands separators (e.g., 11,906,0324)
inline std::string formatWithCommas(std::uint64_t value) {
    // Try the user's global locale first; some Windows environments may not have
    // a default C++ locale installed. Fall back gracefully.
    try {
        std::ostringstream oss;
        oss.imbue(std::locale("")); // user-preferred locale
        oss << std::fixed << value;
        return oss.str();
    } catch (...) {
        try {
            std::ostringstream oss;
            oss.imbue(std::locale::classic());
            oss << std::fixed << value;
            return oss.str();
        } catch (...) {
            // Final fallback: manual grouping with commas
            std::string s = std::to_string(value);
            std::string out;
            out.reserve(s.size() + s.size() / 3);
            int count = 0;
            for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
                out.push_back(s[i]);
                if (++count == 3 && i != 0) {
                    out.push_back(',');
                    count = 0;
                }
            }
            std::reverse(out.begin(), out.end());
            return out;
        }
    }
}

// Generic perft (move-generation test)
// - BoardT: your board type (passed by reference)
// - MoveT:  your move type
// - generate(board) -> std::vector<MoveT>
// - make(board, move)
// - unmake(board, move)
template <typename BoardT, typename MoveT, typename GenerateFn, typename MakeFn, typename UnmakeFn>
std::uint64_t perft(BoardT& board,
					int depth,
					GenerateFn generate,
					MakeFn make,
					UnmakeFn unmake) {
	if (depth == 0) {
		return 1ULL;
	}

	std::uint64_t nodes = 0ULL;
	std::vector<MoveT> moves = generate(board);
	for (const MoveT& mv : moves) {
		make(board, mv);
		nodes += perft<BoardT, MoveT>(board, depth - 1, generate, make, unmake);
		unmake(board, mv);
	}
	return nodes;
}

// Runs perft for depths [1..maxDepth] and prints results like the screenshot
// Example usage:
//   runPerft(board, 6,
//     [](Board& b){ return generateMoves(b); },
//     [](Board& b, const Move& m){ makeMove(b,m); },
//     [](Board& b, const Move& m){ unmakeMove(b,m); }
//   );
template <typename BoardT, typename MoveT, typename GenerateFn, typename MakeFn, typename UnmakeFn>
void runPerft(BoardT& board,
			  int maxDepth,
			  GenerateFn generate,
			  MakeFn make,
			  UnmakeFn unmake,
			  bool showHeader = true) {
	if (showHeader) {
		Logger::log(LogLevel::INFO, "Running Test... (bulk-counting enabled)", __FILE__, __LINE__);
	}

	for (int d = 1; d <= maxDepth; ++d) {
		auto t0 = std::chrono::high_resolution_clock::now();
		std::uint64_t nodes = perft<BoardT, MoveT>(board, d, generate, make, unmake);
		auto t1 = std::chrono::high_resolution_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

		Logger::log(LogLevel::INFO, std::string("Depth: ") + std::to_string(d) + " ply  Result: " + formatWithCommas(nodes) + " positions  Time: " + std::to_string(ms) + " milliseconds", __FILE__, __LINE__);
	}
}

} // namespace chessai

#endif // CHESS_AI_UTILS_H


