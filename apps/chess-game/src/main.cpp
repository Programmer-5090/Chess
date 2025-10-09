#include <chess/utils/logger.h>
#include "chess/rendering/screen.h"
#include <chess/menus/manager.h>
#include <chess/ui/input.h>
#include <chess/board/board.h>
#include <chess/board/game_logic.h>

#include <iostream>

int main(int argc, char* argv[]) {
    // Initialize logger
    Logger::init("output/logs", LogLevel::DEBUG, true, 50);
    Logger::log(LogLevel::INFO, "Chess game starting!", __FILE__, __LINE__);
    
    try {
        // Create and run the game
        Screen gameScreen(600, 600);
        gameScreen.run();
        
        Logger::log(LogLevel::INFO, "Chess game completed successfully!", __FILE__, __LINE__);
    } catch (const std::exception& e) {
        Logger::log(LogLevel::ERROR, std::string("Chess game error: ") + e.what(), __FILE__, __LINE__);
        std::cerr << "Error: " << e.what() << std::endl;
        Logger::shutdown();
        return 1;
    }
    
    Logger::shutdown();
    return 0;
}
