#include <chess/utils/logger.h>
#include "chess/rendering/screen.h"
#include <chess/menus/manager.h>
#include <chess/ui/input.h>
#include <chess/board/board.h>
#include <chess/board/game_logic.h>

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
// Undefine Windows macros that conflict with our enums
#ifdef ERROR
#undef ERROR
#endif
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Allocate a console for this GUI application
    AllocConsole();
    
    // Redirect stdout, stdin, stderr to console
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    
    // Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
    // point to console as well
    std::ios::sync_with_stdio(true);
    
    // Set console title (use ANSI version)
    SetConsoleTitleA("Chess Game - AI Performance Monitor");
#endif
    // Initialize logger without stream redirection so console output works
    Logger::init("output/logs", LogLevel::DEBUG, false, 50);
    
    std::cout << "Chess Game Console - AI Performance Monitor\n";
    std::cout << "==========================================\n";
    std::cout << "Chess game starting!\n";
    std::cout << "\nInstructions:\n";
    std::cout << "- Select 'Play vs Computer' from the main menu\n";
    std::cout << "- Choose your color (the AI will play the opposite color)\n";
    std::cout << "- Make your move, then watch the AI performance stats!\n";
    std::cout << "==========================================\n" << std::flush;

    Logger::setMinLevel(LogLevel::ERROR);
    
    try {
        // Create and run the game
        Screen gameScreen(600, 600);
        gameScreen.run();
        
        std::cout << "Chess game completed successfully!\n";
    } catch (const std::exception& e) {
        std::cout << "Chess game error: " << e.what() << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        Logger::shutdown();
        return 1;
    }
    
    Logger::shutdown();
    return 0;
}
