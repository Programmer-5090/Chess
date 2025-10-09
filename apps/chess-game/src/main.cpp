#include "screen.h"
#include "logger.h"

int main(int argc, char* argv[]) {
    // Initialize logger: directory, min level, redirect std streams, max file size MB
    Logger::init("output/logs", LogLevel::DEBUG, true, 50);  // Changed to DEBUG to see texture loading
    Screen screen(600, 600);
    screen.run();
    Logger::shutdown();
    return 0;
}
