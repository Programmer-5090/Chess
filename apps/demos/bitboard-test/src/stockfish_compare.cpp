#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <chess/board/board.h>

class StockfishInterface {
private:
    HANDLE hStdinRead = NULL;
    HANDLE hStdinWrite = NULL;
    HANDLE hStdoutRead = NULL;
    HANDLE hStdoutWrite = NULL;
    PROCESS_INFORMATION piProcInfo;
    bool initialized = false;

public:
    StockfishInterface(const std::string& stockfishPath) {
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        // Create pipes for STDIN
        if (!CreatePipe(&hStdinRead, &hStdinWrite, &saAttr, 0)) {
            std::cerr << "Failed to create stdin pipe\n";
            return;
        }
        SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);

        // Create pipes for STDOUT
        if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &saAttr, 0)) {
            std::cerr << "Failed to create stdout pipe\n";
            return;
        }
        SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);

        // Start Stockfish process
        STARTUPINFOA siStartInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = hStdoutWrite;
        siStartInfo.hStdOutput = hStdoutWrite;
        siStartInfo.hStdInput = hStdinRead;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        if (!CreateProcessA(NULL, const_cast<char*>(stockfishPath.c_str()),
            NULL, NULL, TRUE, 0, NULL, NULL, &siStartInfo, &piProcInfo)) {
            std::cerr << "Failed to start Stockfish at: " << stockfishPath << "\n";
            std::cerr << "Error code: " << GetLastError() << "\n";
            return;
        }

        initialized = true;
        sendCommand("uci");
        waitForResponse("uciok");
        sendCommand("setoption name UCI_ShowWDL value false");
        sendCommand("isready");
        waitForResponse("readyok");
    }

    ~StockfishInterface() {
        if (initialized) {
            sendCommand("quit");
            CloseHandle(piProcInfo.hProcess);
            CloseHandle(piProcInfo.hThread);
            CloseHandle(hStdinRead);
            CloseHandle(hStdinWrite);
            CloseHandle(hStdoutRead);
            CloseHandle(hStdoutWrite);
        }
    }

    void sendCommand(const std::string& cmd) {
        if (!initialized) return;
        std::string command = cmd + "\n";
        DWORD written;
        WriteFile(hStdinWrite, command.c_str(), command.length(), &written, NULL);
    }

    std::string readLine() {
        if (!initialized) return "";
        std::string line;
        char buffer[1];
        DWORD read;

        while (true) {
            if (ReadFile(hStdoutRead, buffer, 1, &read, NULL) && read > 0) {
                if (buffer[0] == '\n') {
                    break;
                }
                if (buffer[0] != '\r') {
                    line += buffer[0];
                }
            } else {
                break;
            }
        }
        return line;
    }

    void waitForResponse(const std::string& expected) {
        while (true) {
            std::string line = readLine();
            if (line.find(expected) != std::string::npos) {
                break;
            }
        }
    }

    std::vector<std::string> getLegalMoves(const std::string& fen) {
        if (!initialized) return {};

        // Set position
        sendCommand("position fen " + fen);
        sendCommand("go perft 1");

        std::vector<std::string> moves;
        while (true) {
            std::string line = readLine();
            
            // Check for end of perft output
            if (line.find("Nodes searched:") != std::string::npos) {
                break;
            }
            
            // Parse move lines (format: "e2e4: 1")
            // Skip info lines
            if (line.find("info") == 0) {
                continue;
            }
            if (!line.empty() && line.find(':') != std::string::npos) {
                size_t colonPos = line.find(':');
                std::string move = line.substr(0, colonPos);
                // Trim whitespace
                move.erase(0, move.find_first_not_of(" \t"));
                move.erase(move.find_last_not_of(" \t") + 1);
                if (move.length() >= 4) {
                    moves.push_back(move);
                }
            }
        }

        return moves;
    }

    bool isInitialized() const { return initialized; }
};

void testPosition(const std::string& fen, const std::string& description, 
                  StockfishInterface& stockfish) {
    std::cout << "\n========================================\n";
    std::cout << "Testing: " << description << "\n";
    std::cout << "FEN: " << fen << "\n";
    std::cout << "========================================\n";

    // Get Stockfish moves
    std::vector<std::string> stockfishMoves = stockfish.getLegalMoves(fen);
    std::sort(stockfishMoves.begin(), stockfishMoves.end());

    // Get Legacy moves
    Board board(800, 800, 50.0f);
    board.loadFEN(fen, nullptr);
    Color color = board.getCurrentPlayer();
    std::vector<Move> legacyMoves = board.getAllPseudoLegalMoves(color, true);

    // Get Bitboard moves
    std::vector<Move> bbMoves = board.getAllPseudoLegalMovesBB(color, true);

    // Convert our moves to UCI format for comparison
    // Our Move structure uses: row 0 = rank 8 (black's back rank)
    // UCI uses: rank 1 = row 0
    auto toUCI = [](const Move& m) -> std::string {
        std::string uci;
        uci += char('a' + m.startPos.second);  // file (column)
        uci += char('8' - m.startPos.first);    // rank (inverted row)
        uci += char('a' + m.endPos.second);     // file (column)
        uci += char('8' - m.endPos.first);      // rank (inverted row)
        if (m.isPromotion) {
            uci += 'q'; // Assume queen promotion for now
        }
        return uci;
    };

    std::vector<std::string> legacyUCI;
    for (const auto& m : legacyMoves) {
        legacyUCI.push_back(toUCI(m));
    }
    std::sort(legacyUCI.begin(), legacyUCI.end());

    std::vector<std::string> bbUCI;
    for (const auto& m : bbMoves) {
        bbUCI.push_back(toUCI(m));
    }
    std::sort(bbUCI.begin(), bbUCI.end());

    // Print results
    std::cout << "\nMove Counts:\n";
    std::cout << "  Stockfish:  " << stockfishMoves.size() << " moves\n";
    std::cout << "  Legacy:     " << legacyUCI.size() << " moves "
              << (legacyUCI.size() == stockfishMoves.size() ? "✓" : "✗") << "\n";
    std::cout << "  Bitboard:   " << bbUCI.size() << " moves "
              << (bbUCI.size() == stockfishMoves.size() ? "✓" : "✗") << "\n";

    // Find differences
    if (legacyUCI != stockfishMoves) {
        std::cout << "\nLegacy differences from Stockfish:\n";
        // Missing moves
        for (const auto& sf : stockfishMoves) {
            if (std::find(legacyUCI.begin(), legacyUCI.end(), sf) == legacyUCI.end()) {
                std::cout << "  Missing: " << sf << "\n";
            }
        }
        // Extra moves
        for (const auto& leg : legacyUCI) {
            if (std::find(stockfishMoves.begin(), stockfishMoves.end(), leg) == stockfishMoves.end()) {
                std::cout << "  Extra: " << leg << "\n";
            }
        }
    }

    if (bbUCI != stockfishMoves) {
        std::cout << "\nBitboard differences from Stockfish:\n";
        // Missing moves
        for (const auto& sf : stockfishMoves) {
            if (std::find(bbUCI.begin(), bbUCI.end(), sf) == bbUCI.end()) {
                std::cout << "  Missing: " << sf << "\n";
            }
        }
        // Extra moves
        for (const auto& bb : bbUCI) {
            if (std::find(stockfishMoves.begin(), stockfishMoves.end(), bb) == stockfishMoves.end()) {
                std::cout << "  Extra: " << bb << "\n";
            }
        }
    }

    // Summary
    std::cout << "\nSummary:\n";
    if (legacyUCI == stockfishMoves) {
        std::cout << "  Legacy: ✓ CORRECT\n";
    } else {
        std::cout << "  Legacy: ✗ INCORRECT\n";
    }
    if (bbUCI == stockfishMoves) {
        std::cout << "  Bitboard: ✓ CORRECT\n";
    } else {
        std::cout << "  Bitboard: ✗ INCORRECT\n";
    }
}

int main(int argc, char* argv[]) {
    std::string stockfishPath = "stockfish.exe";
    if (argc > 1) {
        stockfishPath = argv[1];
    }

    std::cout << "==============================================\n";
    std::cout << " Stockfish Move Generation Validation\n";
    std::cout << "==============================================\n";
    std::cout << "Stockfish path: " << stockfishPath << "\n";

    StockfishInterface stockfish(stockfishPath);
    if (!stockfish.isInitialized()) {
        std::cerr << "\nFailed to initialize Stockfish!\n";
        std::cerr << "Please ensure stockfish.exe is in your PATH or provide path as argument.\n";
        return 1;
    }

    std::cout << "Stockfish initialized successfully!\n";

    // Test positions
    testPosition(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Initial Position", stockfish);

    testPosition(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "Kiwipete Position", stockfish);

    testPosition(
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "Tricky Position (pins)", stockfish);

    testPosition(
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "Complex Position", stockfish);

    testPosition(
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "Position 4", stockfish);

    testPosition(
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "Middle Game Position", stockfish);

    std::cout << "\n==============================================\n";
    std::cout << " Test Complete!\n";
    std::cout << "==============================================\n";

    return 0;
}
