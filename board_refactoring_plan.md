# Board Class Refactoring Plan

## Current Issues Identified

1. **Single Responsibility Violation**: Board handles game state, rendering, UI dialogs, move validation, and cache management
2. **Complex State Management**: Multiple caches that can become inconsistent
3. **Performance Bottlenecks**: Frequent cache rebuilds and consistency checks
4. **Tight Coupling**: Board directly manages UI elements and rendering
5. **Large Interface**: Too many public methods with unclear responsibilities

## Phase 1: Extract Separate Concerns

### 1.1 Create BoardRenderer Class
```cpp
class BoardRenderer {
private:
    SDL_Renderer* renderer;
    BoardVisuals visuals; // square colors, highlights, etc.
    
public:
    void drawBoard(const BoardState& state);
    void drawPieces(const std::vector<Piece*>& pieces);
    void drawHighlights(const std::vector<Move>& moves);
    void setFlipped(bool flipped);
};
```

### 1.2 Create PieceManager Class
```cpp
class PieceManager {
private:
    std::vector<std::unique_ptr<Piece>> whitePieces;
    std::vector<std::unique_ptr<Piece>> blackPieces;
    std::unordered_map<PieceId, Piece*> pieceMap;
    
public:
    void addPiece(std::unique_ptr<Piece> piece);
    void removePiece(PieceId id);
    void movePiece(PieceId id, Position newPos);
    const std::vector<Piece*>& getPieces(Color color) const;
    Piece* findKing(Color color) const;
};
```

### 1.3 Create MoveExecutor Class
```cpp
class MoveExecutor {
private:
    BoardState& boardState;
    PieceManager& pieceManager;
    
public:
    UndoInfo executeMove(const Move& move);
    void undoMove(const Move& move, const UndoInfo& undoInfo);
    bool isLegalMove(const Move& move) const;
};
```

### 1.4 Create PromotionHandler Class
```cpp
class PromotionHandler {
private:
    std::unique_ptr<UIPromotionDialog> dialog;
    std::function<void(Position, PieceType)> onPromotionCallback;
    
public:
    bool needsPromotion(const Move& move) const;
    void showPromotionDialog(Position pos, Color color);
    void handleSelection(PieceType type);
    bool isDialogActive() const;
};
```

## Phase 2: Simplify Board State Management

### 2.1 Create Immutable BoardState
```cpp
class BoardState {
private:
    std::array<std::array<PieceId, 8>, 8> squares;
    GameStateFlags flags; // castling rights, en passant, etc.
    
public:
    PieceId getPieceAt(Position pos) const;
    BoardState withMove(const Move& move) const; // immutable updates
    bool isSquareEmpty(Position pos) const;
    std::vector<Position> getOccupiedSquares() const;
};
```

### 2.2 Remove Redundant Caches
- Eliminate `pieceCache`, `whitePieces`, `blackPieces` from Board
- Move piece lookup responsibility to PieceManager
- Use lazy evaluation instead of eager caching

## Phase 3: Redesign Board Class Interface

### 3.1 Refactored Board Class
```cpp
class Board {
private:
    BoardState state;
    PieceManager pieceManager;
    MoveExecutor moveExecutor;
    PromotionHandler promotionHandler;
    BoardRenderer renderer;
    
    // Configuration
    BoardDimensions dimensions;
    bool isFlipped = false;
    
public:
    // Core game operations
    void loadFEN(const std::string& fen);
    bool makeMove(const Move& move);
    void undoMove();
    
    // Query operations
    std::vector<Move> getLegalMoves(Color color) const;
    bool isInCheck(Color color) const;
    bool isCheckmate(Color color) const;
    GameResult getGameResult() const;
    
    // UI operations
    void draw(const RenderContext& context);
    bool handleClick(int x, int y, Position& boardPos);
    
    // Piece queries
    Piece* getPieceAt(Position pos) const;
    Position getKingPosition(Color color) const;
};
```

## Phase 4: Improve Performance Architecture

### 4.1 Event-Driven Updates
```cpp
class BoardEventSystem {
public:
    void subscribe(BoardEventType type, std::function<void(const BoardEvent&)> handler);
    void notify(const BoardEvent& event);
};

// Events: PieceMoved, PieceCaptured, GameStateChanged, etc.
```

### 4.2 Incremental Move Generation
```cpp
class MoveCache {
private:
    std::unordered_map<Position, std::vector<Move>> cachedMoves;
    bool isDirty = true;
    
public:
    const std::vector<Move>& getMovesFrom(Position pos);
    void invalidate(Position pos);
    void invalidateAll();
};
```

## Phase 5: Eliminate Global State and Improve Testability

### 5.1 Remove Global Profile Object
```cpp
class PerformanceProfiler {
private:
    MakeUnmakeProfile profile;
    
public:
    void startTimer(const std::string& operation);
    void endTimer(const std::string& operation);
    const MakeUnmakeProfile& getProfile() const;
};
```

### 5.2 Dependency Injection
```cpp
class Board {
public:
    Board(std::unique_ptr<BoardRenderer> renderer,
          std::unique_ptr<PieceFactory> pieceFactory,
          std::unique_ptr<PerformanceProfiler> profiler = nullptr);
};
```

## Implementation Strategy

### Step 1: Extract Renderer (Low Risk)
- Move all SDL rendering code to BoardRenderer
- Keep Board interface unchanged initially
- Test rendering functionality

### Step 2: Extract PieceManager (Medium Risk)
- Move piece collection management
- Gradually migrate cache-dependent methods
- Maintain backward compatibility

### Step 3: Extract MoveExecutor (High Risk)
- Move make/unmake move logic
- Requires careful testing of game logic
- Consider feature flags for gradual rollout

### Step 4: Simplify Board Interface
- Remove redundant methods
- Consolidate related functionality
- Update client code

### Step 5: Performance Optimizations
- Implement event system
- Add move caching
- Profile and optimize hot paths

## Benefits of This Refactoring

1. **Single Responsibility**: Each class has one clear purpose
2. **Testability**: Easier to unit test individual components
3. **Performance**: Eliminates redundant cache rebuilds
4. **Maintainability**: Smaller, focused classes are easier to modify
5. **Extensibility**: New features can be added without modifying core Board
6. **Thread Safety**: Immutable BoardState enables concurrent access

## Migration Timeline

- **Week 1-2**: Extract BoardRenderer and PromotionHandler
- **Week 3-4**: Extract PieceManager
- **Week 5-6**: Extract MoveExecutor and refactor move logic
- **Week 7-8**: Simplify Board class interface
- **Week 9-10**: Performance optimizations and cleanup

## Risk Mitigation

1. **Comprehensive Testing**: Unit tests for each extracted class
2. **Feature Flags**: Allow switching between old/new implementations
3. **Incremental Rollout**: Extract one component at a time
4. **Performance Monitoring**: Ensure no regressions
5. **Code Reviews**: Multiple eyes on critical game logic changes