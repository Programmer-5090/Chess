#include <chess/board/piece_manager.h>
#include <chess/board/pieces/piece.h>
#include <chess/enums.h>
#include <chess/utils/logger.h>
#include <chess/utils/profiler.h>
#include <sstream>

void PieceManager::ensureCachesInitialized() const {
    g_profiler.startTimer("pm_ensureCachesInitialized");
    if (!cachesDirty) {
        g_profiler.endTimer("pm_ensureCachesInitialized");
        return;
    }
    
    // Only rebuild if caches are completely uninitialized
    cachedWhitePieces.clear();
    cachedBlackPieces.clear();
    cachedAllPieces.clear();
    
    size_t total = pieces.size();
    if (total > 0) {
        cachedAllPieces.reserve(total);
        cachedWhitePieces.reserve((total / 2) + 1);
        cachedBlackPieces.reserve((total / 2) + 1);
    }
    
    for (const auto& [id, piece] : pieces) {
        if (!piece) continue;
        
        cachedAllPieces.push_back(piece.get());
        if (piece->getColor() == Color::WHITE) {
            cachedWhitePieces.push_back(piece.get());
        } else {
            cachedBlackPieces.push_back(piece.get());
        }
    }
    
    cachesDirty = false;
    g_profiler.endTimer("pm_ensureCachesInitialized");
}

void PieceManager::addPiece(std::unique_ptr<Piece> piece) {
    g_profiler.startTimer("pm_addPiece");
    if (!piece) {
        g_profiler.endTimer("pm_addPiece");
        return;
    }
    
    PieceId id = piece->id;
    if (pieces.find(id) != pieces.end()) {
        PieceId newId = id;
        while (pieces.find(newId) != pieces.end()) ++newId;
        piece->id = newId;
        id = newId;
    }
    
    Piece* piecePtr = piece.get(); // Get pointer before moving
    pieces[id] = std::move(piece);
    
    if (!cachesDirty) {
        cachedAllPieces.push_back(piecePtr);
        if (piecePtr->getColor() == Color::WHITE) {
            cachedWhitePieces.push_back(piecePtr);
        } else {
            cachedBlackPieces.push_back(piecePtr);
        }
    }
    #ifdef DEBUG
    {
        std::ostringstream oss;
        oss << "pm_addPiece: id=" << piecePtr->id << " type=" << piecePtr->stringPieceType()
            << " color=" << (piecePtr->getColor()==WHITE?"W":"B")
            << " pos=" << piecePtr->getPosition().first << "," << piecePtr->getPosition().second;
        Logger::log(LogLevel::DEBUG, oss.str(), __FILE__, __LINE__);
    }
    #endif
    g_profiler.endTimer("pm_addPiece");
}

std::unique_ptr<Piece> PieceManager::removePiece(PieceId id) {
    g_profiler.startTimer("pm_removePiece");
    auto it = pieces.find(id);
    if (it == pieces.end()) {
        g_profiler.endTimer("pm_removePiece");
        return nullptr;
    }
    
    Piece* piecePtr = it->second.get();
    std::unique_ptr<Piece> removedPiece = std::move(it->second);
    pieces.erase(it);
    
    if (!cachesDirty && piecePtr) {
        auto allIt = std::find(cachedAllPieces.begin(), cachedAllPieces.end(), piecePtr);
        if (allIt != cachedAllPieces.end()) {
            cachedAllPieces.erase(allIt);
        }
        
        // Remove from color-specific cache
        if (piecePtr->getColor() == Color::WHITE) {
            auto whiteIt = std::find(cachedWhitePieces.begin(), cachedWhitePieces.end(), piecePtr);
            if (whiteIt != cachedWhitePieces.end()) {
                cachedWhitePieces.erase(whiteIt);
            }
        } else {
            auto blackIt = std::find(cachedBlackPieces.begin(), cachedBlackPieces.end(), piecePtr);
            if (blackIt != cachedBlackPieces.end()) {
                cachedBlackPieces.erase(blackIt);
            }
        }
    }
    g_profiler.endTimer("pm_removePiece");
    return removedPiece;
}

void PieceManager::movePiece(PieceId id, const Position& newPos) {
    auto it = pieces.find(id);
    if (it == pieces.end() || !it->second) return;
    
    it->second->setPosition(newPos.first, newPos.second);
}

const std::vector<Piece*>& PieceManager::getPieces(Color color) const {
    g_profiler.startTimer("pm_getPieces");
    ensureCachesInitialized();
    const std::vector<Piece*>& result = (color == Color::WHITE) ? cachedWhitePieces : cachedBlackPieces;
    g_profiler.endTimer("pm_getPieces");
    return result;
}

const std::vector<Piece*>& PieceManager::getAllPieces() const {
    g_profiler.startTimer("pm_getAllPieces");
    ensureCachesInitialized();
    g_profiler.endTimer("pm_getAllPieces");
    return cachedAllPieces;
}

Piece* PieceManager::findKing(Color color) const {
    ensureCachesInitialized();
    
    const auto& colorPieces = (color == Color::WHITE) ? cachedWhitePieces : cachedBlackPieces;
    
    for (Piece* piece : colorPieces) {
        if (piece && piece->getType() == PieceType::KING) {
            return piece;
        }
    }
    
    return nullptr;
}

bool PieceManager::validateKings() const {
    ensureCachesInitialized();
    bool whiteKingFound = false;
    bool blackKingFound = false;

    for (const auto& piece : cachedAllPieces) {
        if (piece && piece->getType() == KING) {
            if (piece->getColor() == WHITE) whiteKingFound = true;
            if (piece->getColor() == BLACK) blackKingFound = true;
        }
    }
    
    return whiteKingFound && blackKingFound;
}

Piece* PieceManager::getPieceById(PieceId id) const {
    g_profiler.startTimer("pm_getPieceById");
    auto it = pieces.find(id);
    g_profiler.endTimer("pm_getPieceById");
    return (it != pieces.end()) ? it->second.get() : nullptr;
}

size_t PieceManager::getPieceCount(Color color) const {
    ensureCachesInitialized();
    return (color == Color::WHITE) ? cachedWhitePieces.size() : cachedBlackPieces.size();
}

void PieceManager::clear() {
    pieces.clear();
    cachedWhitePieces.clear();
    cachedBlackPieces.clear();
    cachedAllPieces.clear();
    cachesDirty = false;
}

void PieceManager::invalidateCache() {
    g_profiler.startTimer("pm_invalidateCache");
    cachesDirty = true;
    g_profiler.endTimer("pm_invalidateCache");
}