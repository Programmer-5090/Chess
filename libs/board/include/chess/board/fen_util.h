#ifndef FEN_UTIL_H
#define FEN_UTIL_H
#include <string>
#include <SDL.h>

// Forward declaration  
class Board;

class FENUtil {
public:
    void loadFEN(const std::string& fen, Board& board, SDL_Renderer* gameRenderer);
    std::string getCurrentFEN(const Board& board) const;
};
#endif // FEN_UTIL_H
