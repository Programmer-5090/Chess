#include <chess/board/bitboard/move.h>

namespace chess {

std::string BBMove::toString() const {
    if (value == 0) return "NULL";
    
    int from = startSquare();
    int to = targetSquare();
    
    std::string result;
    result += char('a' + (from % 8));
    result += char('1' + (from / 8));
    result += char('a' + (to % 8));
    result += char('1' + (to / 8));
    
    // Add promotion piece
    if (isPromotion()) {
        switch (flag()) {
            case PromoteToQueen:  result += 'q'; break;
            case PromoteToRook:   result += 'r'; break;
            case PromoteToBishop: result += 'b'; break;
            case PromoteToKnight: result += 'n'; break;
            default: break;
        }
    }
    
    return result;
}

} // namespace chess
