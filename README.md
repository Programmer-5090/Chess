# Chess Engine & Game in C++

A chess engine with a full-featured graphical game interface, built in modern C++ with AI capabilities including alpha-beta pruning, transposition tables, and multi-threaded search. Features both traditional and bitboard-based board representations with testing and performance analysis tools.

# Images

<p float="left">
  <img src="Imgs/readMe/chess_game 1.png" alt="1" width="300" />
  <img src="Imgs/readMe/chess_game 2.png" alt="2" width="300" /> 
  <img src="Imgs/readMe/chess_game 3.png" alt="3" width="300" />
  <img src="Imgs/readMe/chess_game 4.png" alt="4" width="300" />
</p>

## Project Architecture

```
Chess-C--/
├── libs/                           # Modular library components
│   ├── core/                       # Core utilities and logging
│   ├── board/                      # Chess board logic
│   │   ├── traditional/            # Standard 2D array board
│   │   ├── bitboard/               # High-performance bitboard implementation
│   │   │   ├── BoardBB             # Bitboard state management
│   │   │   ├── move_generator_bb   # Legal move generation (bitboards)
│   │   │   ├── move_executor_bb    # Move execution and undo
│   │   │   └── transpositionTable  # TT for search memoization
│   │   ├── game_logic              # Turn management and rules
│   │   ├── game_logicBB            # Bitboard-based game logic
│   │   └── fen_util                # FEN parsing and generation
│   ├── rendering/                  # SDL2 graphics and rendering
│   ├── ui/                         # Modular UI component system
│   ├── menus/                      # Game menu system
│   └── utils/                      # Utilities and profiling
│
├── Chess AI/                       # Chess engine implementation
│   ├── include/
│   │   ├── ai.h                    # Traditional board AI
│   │   ├── ai_bb.h                 # Bitboard-based AI (primary)
│   │   └── utils.h                 # AI utilities
│   └── src/
│       ├── ai.cpp
│       └── ai_bb.cpp
│
├── apps/                           # Applications
│   ├── chess-game/                 # Main chess game application
│   └── demos/                      # Demonstration and testing tools
│       ├── bitboard-perft/         # Perft verification (bitboards)
│       ├── board-perft/            # Perft verification (traditional)
│       ├── bitboard-test/          # Bitboard functionality tests
│       ├── enhanced-ui/            # UI component showcase
│       ├── menu-system/            # Menu system demonstration
│       ├── profile-perft/          # Performance profiling
│       └── utils-perft/            # Utility function testing
│
├── assets/                         # Game assets
│   ├── fonts/                      # TTF fonts for UI rendering
│   └── images/                     # Chess piece sprites and board textures
│
├── cmake/                          # CMake build configuration
│   ├── CompilerSettings.cmake      # Compiler flags and optimization
│   ├── Dependencies.cmake          # SDL2 and dependency management
│   └── InstallConfig.cmake         # Installation configuration
│
├── output/                         # Build artifacts
│   ├── Debug/                      # Debug builds
│   └── Release/                    # Release builds
│
└── CMakeLists.txt                  # Root CMake configuration
```

## Requirements

- **Windows OS** (currently supported)
- **C++17** compatible compiler (MSVC 2017+ or MinGW)
- **CMake 3.20** or higher
- **OpenGL** support
- **Internet connection** (for automatic SDL2 download on first build)

**Note**: Currently Windows-only. Linux and macOS support would require extending CMake configuration for platform-specific SDL2 downloads.

## Quick Start

### Automatic Setup (Recommended)

The build system automatically downloads and configures all dependencies—**no manual setup required!**

```bash
# Clone the repository
git clone <repository-url>
cd Chess-C--

# Build and run (SDL2 downloaded automatically on first build)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/bin/Release/chess_game.exe
```

**Build Features:**
- ✅ **Zero-dependency setup**: CMake automatically downloads SDL2, SDL2_image, and SDL2_ttf
- ✅ **Windows support**: Works with MSVC 2017+ and MinGW
- ✅ **Smart detection**: Prefers system SDL2 if available, downloads only when needed
- ✅ **Portable**: Perfect for sharing—no manual library installation needed

### VS Code Quick Start

1. **Open in VS Code**: `File` → `Open Folder` → Select the Chess-C-- directory
2. **Build**: `Ctrl+Shift+P` → "Tasks: Run Task" → "CMake: Build Debug"
3. **Run Game**: `Ctrl+Shift+P` → "Tasks: Run Task" → "Run Chess Game"
4. **Run Tests**: `Ctrl+Shift+P` → "Tasks: Run Task" → Select any demo (perft, bitboard-test, etc.)

### Available VS Code Tasks

- **CMake: Configure Debug** - Configure CMake for debug build
- **CMake: Build Debug** - Build all targets in debug mode
- **CMake: Build Release** - Build optimized release version
- **Run Chess Game** - Launch the main chess game
- **Run: Bitboard Perft** - Run perft verification (bitboards)
- **Run: Board Perft** - Run perft verification (traditional board)
- **Run: Bitboard Test** - Test bitboard functionality
- **Run: Enhanced UI Demo** - Showcase UI component system
- **CMake: Clean** - Clean build artifacts

## Manual SDL2 Installation (Optional)

If automatic download fails or you prefer system-wide installation:

### Windows (MSYS2/MinGW)
```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_image mingw-w64-ucrt-x86_64-SDL2_ttf
```

### Windows (vcpkg)
```bash
vcpkg install sdl2 sdl2-image sdl2-ttf
```

## Building

### Using CMake (Recommended)

```bash
# Configure for Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build all targets
cmake --build build --config Debug

# Or build specific target
cmake --build build --target chess_game --config Debug

# Run the game
./build/bin/Debug/chess_game.exe
```

### Build Options

```bash
# Release build (optimized)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Build without demos
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_DEMOS=OFF

# Verbose build output
cmake --build build --verbose
```

### Build Targets

- **chess_game** - Main chess game application
- **bitboard-perft** - Perft verification tool (bitboard-based)
- **board-perft** - Perft verification tool (traditional board)
- **bitboard-test** - Bitboard functionality tests
- **enhanced-ui** - UI component showcase
- **menu-system** - Menu system demonstration
- **profile-perft** - Performance profiling tool
- **utils-perft** - Utility function tests

### Alternative Build Methods

**Direct G++ compilation** (legacy, not recommended):
```bash
g++ -std=c++17 -O3 -Wall -Wextra src/*.cpp src/pieces/*.cpp -Iinclude \
  -lSDL2 -lSDL2_image -lSDL2_ttf -lOpenGL32 -o chess_game.exe
```

## Features

### Chess Engine (AI_BB)

- **Minimax Search**: Alpha-beta pruning with configurable search depth
- **Quiescence Search**: Extends search in tactical positions to avoid horizon effect
- **Transposition Table**: Memoization of evaluated positions for faster re-evaluation
- **Move Ordering**: Intelligent move ordering for better alpha-beta pruning efficiency
- **Parallel Search**: Multi-threaded root move evaluation using thread pool
- **Material Evaluation**: Piece value-based position assessment
- **Piece-Square Tables**: Position-based piece evaluation for strategic play
- **Bitboard Representation**: High-performance 64-bit board encoding

### Board Representations

- **Traditional Board**: 2D array-based representation for clarity and debugging
- **Bitboard Board**: 64-bit integer representation for performance (primary)
- **FEN Support**: Full Forsyth-Edwards Notation parsing and generation
- **Move Execution**: Efficient make/unmake with history tracking
- **Zobrist Hashing**: Position hashing for transposition table lookups

### Chess Game Features

- **Complete Rule Set**: Castling, en passant, pawn promotion, check/checkmate detection
- **Graphical Interface**: SDL2-based rendering with piece sprites and board textures
- **Move Validation**: Real-time legal move checking with visual highlighting
- **Turn-based Gameplay**: Alternating white and black player turns
- **AI Opponent**: Play against configurable AI at various difficulty levels
- **Performance Monitoring**: Real-time AI search statistics and performance metrics
- **Captured Pieces Display**: Visual tracking of captured pieces

### Modular UI System

- **Component Architecture**: Base UIElement class with 8+ specialized controls
- **Layout Management**: Vertical, Grid, and Custom layout algorithms
- **Event Handling**: Buffered input system with mouse and keyboard support
- **Edit Mode**: Drag-and-drop runtime UI layout editing
- **Callback System**: Intelligent callback gating with edit mode overrides
- **Overlay Rendering**: Modal dialogs and dropdowns render above other elements
- **Visual Feedback**: Hover effects, selection highlighting, drag updates
- **Font Rendering**: TTF font support with customizable sizes and colors
- **Memory Management**: RAII-based resource management with smart pointers

### Testing & Verification Tools
- **Perft Verification**: Validate move generation against known positions (Depth 6: 119,060,324 nodes)
- **Multi-threaded Perft**: Parallel perft with configurable thread count (~6.4x speedup on 8 cores)
- **Performance Profiling**: Detailed timing analysis of search and move generation
- **Bitboard Testing**: Comprehensive bitboard functionality verification
- **Move Filtering**: Test specific moves or positions in isolation

## Gameplay

### Playing the Game

1. **Launch** the chess game from the main menu
2. **Select** "Play vs Computer" to face the AI
3. **Choose** your color (White or Black)
4. **Click** a piece to select it—legal moves highlight in green
5. **Click** a highlighted square to move
6. **Watch** the AI calculate and make its move
7. **Game ends** when checkmate, stalemate, or draw conditions are met

### Game Features

- **Real-time Move Validation**: Illegal moves are prevented
- **Check/Checkmate Detection**: Visual and logical detection of game-ending conditions
- **AI Performance Stats**: Console displays search depth, nodes evaluated, and time taken
- **Piece Capture Tracking**: Captured pieces displayed on screen
- **FEN Display**: Current position shown in FEN notation for analysis

## Architecture & Design

### Core Components

#### Chess Engine (AI_BB)

- `AI_BB::getSearchResult()` - Main search entry point
- `search()` - Recursive minimax with alpha-beta pruning
- `quiescenceSearch()` - Tactical search extension
- `evaluatePosition()` - Position evaluation using material and PST
- `orderMoves()` - Move ordering for pruning efficiency
- `TranspositionTable` - Memoization of evaluated positions

#### Bitboard Board (BoardBB)

- `BoardBB::makeMove()` - Execute move with full state tracking
- `BoardBB::unmakeMove()` - Undo move and restore state
- `BoardBB::generateLegalMoves()` - Generate all legal moves
- `BoardBB::isInCheck()` - Check detection
- `BoardBB::loadFEN()` - FEN parsing
- `BoardBB::toFEN()` - FEN generation

#### Game Logic (GameLogicBB)

- Turn management and game state
- AI move generation and execution
- Move validation and legality checking
- Game-over condition detection

#### Rendering (Screen)

- SDL2 window management
- Board and piece rendering
- Input event handling
- UI coordination

### Class Hierarchy

#### UI System

```text
UIElement (abstract base)
├── UIButton
├── UILabel
├── UICheckbox
├── UIPanel (container)
├── UIDropdown
├── UITextInput
├── UISlider
└── UIDialog
```

#### Chess Pieces

```text
Piece (abstract base)
├── Pawn
├── Rook
├── Knight
├── Bishop
├── Queen
└── King
```

### Design Patterns

#### Engine

- **Alpha-Beta Pruning**: Minimax optimization for faster search
- **Transposition Table**: Hash-based memoization of positions
- **Move Ordering**: Heuristic-based move ordering for better pruning
- **Quiescence Search**: Tactical search extension to avoid horizon effect

#### UI System

- **Component Pattern**: Modular, reusable UI elements
- **Composite Pattern**: Hierarchical container management
- **Observer Pattern**: Event-driven callback system
- **Strategy Pattern**: Pluggable layout algorithms
- **State Pattern**: Edit mode and callback state management
- **Factory Pattern**: UIManager element creation

#### Board

- **Bitboard Representation**: 64-bit integers for efficient move generation
- **Zobrist Hashing**: Position hashing for TT lookups
- **Make/Unmake Pattern**: Reversible move execution for search
- **FEN Notation**: Standard chess position representation

## Code Quality & Standards

- **C++17 Standards**: Modern C++ features including smart pointers, lambda expressions, and type deduction
- **Memory Safety**: RAII principles with automatic resource management and no manual memory allocation
- **Modular Architecture**: Clear separation of concerns with independent libraries (board, rendering, UI, AI)
- **Windows Build**: CMake with automatic SDL2 dependency management
- **Comprehensive Error Handling**: Robust validation for moves, UI interactions, and resource loading
- **Event-Driven Architecture**: Buffered input system with efficient event distribution
- **Type Safety**: Template-based UI element creation with compile-time type checking
- **Performance Optimized**: Bitboard representation, transposition tables, alpha-beta pruning
- **Testable Design**: Comprehensive demo applications for perft, profiling, and UI testing
- **Well-Documented**: Clear code structure with meaningful class and function names

## License

This project is open source. Please check the repository for license details.
