#ifndef BOARD_INTERNAL_H
#define BOARD_INTERNAL_H

#include <memory>
#include <string>
#include <vector>

// Forward declarations to keep this header lightweight.
class BoardState;
class PieceManager;
class MoveExecutor;
class PromotionHandler;
class BoardRenderer;
class PieceFactory;
class PerformanceProfiler;
struct Move;
struct UndoInfo;
enum class Color;
class Piece;
struct RenderContext;
struct Position;
enum class GameResult;

// Small POD for board layout; defined here to avoid depending on UI-only headers.
struct BoardDimensions {
    int width = 800;
    int height = 800;
    float offset = 0.0f;
};

class Board {
private:
    // Use pimpl-like unique_ptr members to keep this header lightweight and avoid
    // requiring full definitions for internal types. Implementations can include
    // the concrete headers and manage lifetime in the .cpp.
    std::unique_ptr<BoardState> state;
    std::unique_ptr<PieceManager> pieceManager;
    std::unique_ptr<MoveExecutor> moveExecutor;
    std::unique_ptr<PromotionHandler> promotionHandler;
    std::unique_ptr<BoardRenderer> renderer;
    
    // Configuration
    // Use the global BoardDimensions defined above for layout data.
    BoardDimensions dimensions;
    bool isFlipped = false;
    
public:
    Board(std::unique_ptr<BoardRenderer> renderer,
          std::unique_ptr<PieceFactory> pieceFactory,
          std::unique_ptr<PerformanceProfiler> profiler = nullptr);
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
#endif // BOARD_INTERNAL_H