# Chess Game in C++

A modern C++ chess game implementation using SDL2 for graphics and rendering, featuring a comprehensive modular UI system.

## Project Structure

```
Chess-C--/
├── CMakeLists.txt          # CMake build configuration with UI demo target
├── README.md               # This file
├── demos/                  # Standalone demo applications
│   └── ui_demo.cpp         # UI system demonstration (panels, controls, layouts, edit mode)
├── assets/                 # Application assets
│   └── fonts/              # Font files for UI rendering
│       └── OpenSans-Regular.ttf
├── .vscode/                # VS Code configuration
│   ├── tasks.json          # Build and run tasks for both game and UI demo
│   ├── launch.json         # Debug configuration
│   └── settings.json       # Editor settings
├── src/                    # Source files
│   ├── main.cpp            # Chess game entry point
│   ├── screen.cpp          # Main game screen and SDL management
│   ├── board.cpp           # Chess board logic
│   ├── gameLogic.cpp       # Game rules and turn management
│   ├── input.cpp           # Input handling with event buffering
│   ├── pieces/             # Chess piece implementations
│   │   ├── piece.cpp       # Base piece class
│   │   ├── pawn.cpp        # Pawn implementation
│   │   ├── rook.cpp        # Rook implementation
│   │   ├── knight.cpp      # Knight implementation
│   │   ├── bishop.cpp      # Bishop implementation
│   │   ├── queen.cpp       # Queen implementation
│   │   └── king.cpp        # King implementation
│   └── ui/                 # Modular UI system implementation
│       ├── uiButton.cpp    # Interactive button component
│       ├── uiCheckbox.cpp  # Checkbox with bypass capability
│       ├── uiDialog.cpp    # Modal dialog system
│       ├── uiDropdown.cpp  # Dropdown with overlay rendering
│       ├── uiLabel.cpp     # Static text display
│       ├── uiManager.cpp   # UI manager with render surface
│       ├── uiSlider.cpp    # Value slider component
│       └── uiTextInput.cpp # Text input with cursor and selection
├── include/                # Header files
│   ├── screen.h            # Screen class declaration
│   ├── board.h             # Board class declaration
│   ├── gameLogic.h         # Game logic declarations
│   ├── input.h             # Input handling declarations
│   ├── enums.h             # Color and piece type enumerations
│   ├── pieceClasses.h      # Includes all piece headers
│   ├── pieces/             # Piece header files
│   │   ├── piece.h         # Base piece class and Move struct
│   │   ├── pawn.h          # Pawn class declaration
│   │   ├── rook.h          # Rook class declaration
│   │   ├── knight.h        # Knight class declaration
│   │   ├── bishop.h        # Bishop class declaration
│   │   ├── queen.h         # Queen class declaration
│   │   └── king.h          # King class declaration
│   └── ui/                 # UI system headers
│       ├── ui.h            # Includes all UI components
│       ├── uiButton.h      # Button component declaration
│       ├── uiCheckbox.h    # Checkbox with callback bypass
│       ├── uiCommon.h      # Common types and utilities
│       ├── uiConfig.h      # Global callback state management
│       ├── uiDialog.h      # Modal dialog declaration
│       ├── uiDropdown.h    # Dropdown menu declaration
│       ├── uiElement.h     # Base UI element class
│       ├── uiLabel.h       # Text label declaration
│       ├── uiManager.h     # UI manager declaration
│       ├── uiPanel.h       # Container with layout and edit mode
│       ├── uiSlider.h      # Slider control declaration
│       └── uiTextInput.h   # Text input declaration
├── images/                 # Game assets
│   ├── chess.png           # Game icon
│   ├── board_plain_05.png  # Chess board texture
│   └── *.png               # Piece sprites (W_/B_ prefix for white/black)
├── output/                 # Compiled executables
│   ├── chess_game.exe      # Main chess game
│   └── ui_demo.exe         # UI system demonstration
└── build/                  # CMake build directory (generated)
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
2. **Build Chess Game**: `Ctrl+Shift+P` → "Tasks: Run Task" → "CMake: Build Debug"
3. **Run Chess Game**: `Ctrl+Shift+P` → "Tasks: Run Task" → "Run Chess Game"
4. **Run UI Demo**: `Ctrl+Shift+P` → "Tasks: Run Task" → "Run: UI Demo (Debug)"

The first build will automatically download SDL2, SDL2_image, and SDL2_ttf. No manual setup required!

### Available Tasks

The project includes pre-configured VS Code tasks:

- **"CMake: Configure Debug"** - Configure CMake for Debug build
- **"CMake: Build Debug"** - Build chess game (default build task)
- **"CMake: Build Release"** - Build optimized release version
- **"Run Chess Game"** - Run the chess game executable
- **"Run: UI Demo (Debug)"** - Run the UI system demonstration
- **"CMake: Clean"** - Clean build files

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

3. **Run the chess game**:
   ```bash
   ./output/chess_game.exe
   ```

4. **Run the UI demo**:
   ```bash
   ./output/ui_demo.exe
   ```

### UI Demo Features

The UI demo showcases a complete modular UI system with the following features:

**Components:**
- **Buttons**: Interactive buttons with hover effects and callbacks
- **Labels**: Static text with customizable fonts and colors
- **Checkboxes**: Toggle controls with callback bypass capability
- **Panels**: Container widgets with multiple layout systems
- **Dropdowns**: Selection menus with overlay rendering
- **Text Input**: Fields with cursor, selection, and live editing
- **Sliders**: Value controls with continuous feedback
- **Modal Dialogs**: Popup dialogs with OK/Cancel actions

**Layout System:**
- **Vertical Layout**: Stack children top-to-bottom with padding and spacing
- **Grid Layout**: Arrange children in configurable rows and columns
- **Custom Layout**: User-defined positioning with lambda functions

**Edit Mode:**
- Toggle "Edit layout" to enable drag-and-drop repositioning of UI elements
- Visual feedback during dragging with continuous updates
- Edit mode automatically disables all callbacks except essential controls
- Prevents accidental actions while designing layouts

**Callback Management:**
- Global callback toggle for testing UI without triggering actions
- Intelligent callback gating with user preferences and edit mode overrides
- Bypass mechanism for critical controls (edit toggle, callback toggle)
- Status display showing effective callback state

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

### Chess Game
- **Full Chess Implementation**: All standard chess rules including castling, en passant, and checkmate detection
- **Graphical Interface**: SDL2-based rendering with piece sprites and board highlighting
- **Move Validation**: Real-time legal move checking and visualization
- **Turn-based Gameplay**: Alternating white and black player turns
- **Check Detection**: Visual and logical check/checkmate detection
- **Captured Pieces Tracking**: Console output of captured pieces

### Modular UI System
- **Component Architecture**: Base UIElement class with specialized controls (Button, Label, Checkbox, etc.)
- **Layout Management**: Flexible panel layouts (Vertical, Grid, Custom) with automatic positioning
- **Event Handling**: Buffered input system with mouse and keyboard support
- **Edit Mode**: Interactive drag-and-drop for runtime UI layout editing
- **Callback System**: Intelligent callback gating with global toggles and edit mode overrides
- **Overlay Rendering**: Modal dialogs and dropdowns render above other elements
- **Visual Feedback**: Hover effects, selection highlighting, and continuous drag updates
- **Font Rendering**: TTF font support with customizable sizes and colors
- **Memory Management**: RAII-based resource management with smart pointers

## How to Play

1. Launch the game
2. Click on a piece to select it (highlighted squares show possible moves)
3. Click on a highlighted square to move the piece
4. Game alternates between white and black players
5. The game detects check, checkmate, and enforces all standard chess rules

## Architecture

### Chess Game Classes

- **Screen**: Manages SDL2 initialization, rendering loop, and input coordination
- **Board**: Represents the chess board state and handles piece movement logic
- **GameLogic**: Manages turn sequence, move validation, and game state
- **Piece**: Abstract base class for all chess pieces with move generation
- **Input**: Handles keyboard and mouse input events with buffering

### UI System Classes

- **UIElement**: Abstract base class for all UI components with common properties (position, size, visibility)
- **UIManager**: Central coordinator managing UI elements, input distribution, and rendering
- **UIPanel**: Container widget with layout management (Vertical/Grid/Custom) and edit mode support
- **UIButton**: Interactive button with hover effects, callbacks, and customizable appearance
- **UILabel**: Static text display with font rendering and color customization
- **UICheckbox**: Toggle control with callback bypass capability for critical operations
- **UIDropdown**: Selection menu with overlay rendering and item management
- **UITextInput**: Text field with cursor positioning, selection, and live editing
- **UISlider**: Value control with drag interaction and continuous feedback
- **UIDialog**: Modal dialog system with OK/Cancel actions and overlay rendering
- **UIConfig**: Global state management for callback enabling and edit mode coordination

### Design Patterns

**Chess Game:**
- **Polymorphism**: All chess pieces inherit from the base `Piece` class
- **RAII**: Smart pointers (`std::unique_ptr`) manage piece lifetime
- **Forward Declarations**: Minimize header dependencies and compilation time
- **Move Semantics**: Efficient piece transfers using `std::move`

**UI System:**
- **Component Pattern**: Modular UI elements with common base interface
- **Composite Pattern**: UIPanel containers manage child elements hierarchically
- **Observer Pattern**: Callback system for UI event handling
- **Strategy Pattern**: Pluggable layout algorithms (Vertical, Grid, Custom)
- **Template Methods**: UIManager template functions for type-safe element creation
- **State Pattern**: Edit mode and callback states control UI behavior
- **Factory Pattern**: UIManager creates and manages UI element lifecycle

## Code Quality

- **C++17 Standards**: Modern C++ features including smart pointers, lambda expressions, and type deduction
- **Memory Safety**: RAII principles with automatic resource management and no manual memory allocation
- **Modular Design**: Clear separation of concerns with independent UI components and chess logic
- **Cross-platform**: CMake build system with automatic SDL2 dependency management
- **Comprehensive Error Handling**: Robust validation for moves, UI interactions, and resource loading
- **Event-Driven Architecture**: Buffered input system with efficient event distribution
- **Configurable Layouts**: Runtime layout switching with drag-and-drop editing capabilities
- **Type Safety**: Template-based UI element creation with compile-time type checking
- **Performance Optimized**: Render surface caching and efficient UI update cycles

## Contributing

1. Follow the existing code style and structure
2. Add appropriate error handling and validation
3. Update documentation for new features
4. Test changes thoroughly before submitting

## License

This project is open source. Please check the repository for license details.
