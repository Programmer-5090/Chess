# Chess Game in C++

A modern C++ chess game implementation using SDL2 for graphics and rendering.

## Project Structure

```
Chess-C--/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── .vscode/                # VS Code configuration
│   ├── tasks.json          # Build and run tasks
│   ├── launch.json         # Debug configuration
│   └── settings.json       # Editor settings
├── src/                    # Source files
│   ├── main.cpp            # Entry point
│   ├── screen.cpp          # Main game screen and SDL management
│   ├── board.cpp           # Chess board logic
│   ├── gameLogic.cpp       # Game rules and turn management
│   ├── input.cpp           # Input handling
│   └── pieces/             # Chess piece implementations
│       ├── piece.cpp       # Base piece class
│       ├── pawn.cpp        # Pawn implementation
│       ├── rook.cpp        # Rook implementation
│       ├── knight.cpp      # Knight implementation
│       ├── bishop.cpp      # Bishop implementation
│       ├── queen.cpp       # Queen implementation
│       └── king.cpp        # King implementation
├── include/                # Header files
│   ├── screen.h            # Screen class declaration
│   ├── board.h             # Board class declaration
│   ├── gameLogic.h         # Game logic declarations
│   ├── input.h             # Input handling declarations
│   ├── enums.h             # Color and piece type enumerations
│   ├── pieceClasses.h      # Includes all piece headers
│   └── pieces/             # Piece header files
│       ├── piece.h         # Base piece class and Move struct
│       ├── pawn.h          # Pawn class declaration
│       ├── rook.h          # Rook class declaration
│       ├── knight.h        # Knight class declaration
│       ├── bishop.h        # Bishop class declaration
│       ├── queen.h         # Queen class declaration
│       └── king.h          # King class declaration
├── images/                 # Game assets
│   ├── chess.png           # Game icon
│   ├── board_plain_05.png  # Chess board texture
│   └── *.png               # Piece sprites (W_/B_ prefix for white/black)
└── build/                  # CMake build directory (generated)
    └── bin/                # Output executable location
```

## Requirements

- **C++17** compatible compiler
- **CMake 3.20** or higher
- **SDL2** development libraries (automatically downloaded by CMake)
- **OpenGL** support
- **Internet connection** (for automatic SDL2 download on first build)

## Quick Setup

### Automatic Setup (Recommended)

The build system will automatically download and configure SDL2 dependencies - **no manual setup required!**

```bash
# Clone the repository
git clone <repository-url>
cd Chess-C--

# Build and run (SDL2 will be downloaded automatically on first build)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
./build/bin/Debug/chess_game.exe
```

**Features:**
- ✅ **Zero-dependency setup**: CMake automatically downloads SDL2, SDL2_image, and SDL2_ttf
- ✅ **Cross-platform**: Works with MSVC, MinGW, and system SDL2 installations
- ✅ **Portable**: Perfect for sharing - no manual library installation needed
- ✅ **Smart detection**: Prefers system SDL2 if available, downloads only when needed

### VS Code Quick Start

1. **Open in VS Code**: `File` → `Open Folder` → Select the Chess-C-- directory
2. **Build and Run**: `Ctrl+Shift+P` → "Tasks: Run Task" → "Build and Run"

The first build will automatically download SDL2, SDL2_image, and SDL2_ttf. No manual setup required!

## Manual SDL2 Installation (Optional)

If you prefer to install SDL2 system-wide or the automatic download fails:

### Windows with MSYS2/MinGW
```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2
pacman -S mingw-w64-ucrt-x86_64-SDL2_image
pacman -S mingw-w64-ucrt-x86_64-SDL2_ttf
```

### Windows with vcpkg
```bash
vcpkg install sdl2 sdl2-image sdl2-ttf
```

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
```

### macOS with Homebrew
```bash
brew install sdl2 sdl2_image sdl2_ttf
```

If you have SDL2 installed system-wide, CMake will use that instead of downloading.

## Building

### Using CMake (Recommended)

1. **Configure the project**:
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Debug
   ```

2. **Build the project**:
   ```bash
   cmake --build build --config Debug
   ```

3. **Run the game**:
   ```bash
   ./build/bin/chess_game.exe
   ```

### Using VS Code Tasks

The project includes pre-configured VS Code tasks:

- **Ctrl+Shift+P** → "Tasks: Run Task" → "Setup SDL2" (First-time setup)
- **Ctrl+Shift+P** → "Tasks: Run Task" → "CMake: Build Debug" (Default build task)
- **Ctrl+Shift+P** → "Tasks: Run Task" → "Build and Run" (Build and run the game)
- **Ctrl+Shift+P** → "Tasks: Run Task" → "Run Chess Game" (Run without building)

### Alternative Build Methods

**G++ Direct Build** (Legacy):
```bash
g++ -std=c++17 -Wall -Wextra -g src/*.cpp src/pieces/*.cpp -Iinclude -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lopengl32 -lglu32 -o build/bin/chess_game.exe
```

## Features

- **Full Chess Implementation**: All standard chess rules including castling, en passant, and checkmate detection
- **Graphical Interface**: SDL2-based rendering with piece sprites and board highlighting
- **Move Validation**: Real-time legal move checking and visualization
- **Turn-based Gameplay**: Alternating white and black player turns
- **Check Detection**: Visual and logical check/checkmate detection
- **Captured Pieces Tracking**: Console output of captured pieces

## How to Play

1. Launch the game
2. Click on a piece to select it (highlighted squares show possible moves)
3. Click on a highlighted square to move the piece
4. Game alternates between white and black players
5. The game detects check, checkmate, and enforces all standard chess rules

## Architecture

### Core Classes

- **Screen**: Manages SDL2 initialization, rendering loop, and input coordination
- **Board**: Represents the chess board state and handles piece movement logic
- **GameLogic**: Manages turn sequence, move validation, and game state
- **Piece**: Abstract base class for all chess pieces with move generation
- **Input**: Handles keyboard and mouse input events

### Design Patterns

- **Polymorphism**: All chess pieces inherit from the base `Piece` class
- **RAII**: Smart pointers (`std::unique_ptr`) manage piece lifetime
- **Forward Declarations**: Minimize header dependencies and compilation time
- **Move Semantics**: Efficient piece transfers using `std::move`

## Code Quality

- **C++17 Standards**: Modern C++ features and best practices
- **Memory Safety**: RAII and smart pointers prevent memory leaks
- **Modular Design**: Clear separation of concerns between components
- **Cross-platform**: CMake build system supports multiple platforms
- **Comprehensive Error Handling**: Robust error checking for invalid moves and states

## Contributing

1. Follow the existing code style and structure
2. Add appropriate error handling and validation
3. Update documentation for new features
4. Test changes thoroughly before submitting

## License

This project is open source. Please check the repository for license details.
