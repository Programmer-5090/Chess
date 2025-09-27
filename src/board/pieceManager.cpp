#include "board/pieceManager.h"
#include "pieces/piece.h"

void PieceManager::rebuildCaches() const {
    if (!cachesDirty) return;
    
    cachedWhitePieces.clear();
    cachedBlackPieces.clear();
    cachedAllPieces.clear();
    
    for (const auto& [id, piece] : pieces) {
        if (!piece) continue; // Safety check
        
        cachedAllPieces.push_back(piece.get());
        if (piece->getColor() == Color::WHITE) {
            cachedWhitePieces.push_back(piece.get());
        } else {
            cachedBlackPieces.push_back(piece.get());
        }
    }
    
    cachesDirty = false;
}

void PieceManager::addPiece(std::unique_ptr<Piece> piece) {
    if (!piece) return;
    
    PieceId id = piece->id;
    // Defensive: if an entry already exists with this id (shouldn't normally
    // happen), pick a new unused id to avoid accidental overwrites / double-free
    if (pieces.find(id) != pieces.end()) {
        PieceId newId = id;
        while (pieces.find(newId) != pieces.end()) ++newId;
        piece->id = newId; // mutate id on the piece to keep consistency
        id = newId;
    }
    pieces[id] = std::move(piece);
    cachesDirty = true;
}

std::unique_ptr<Piece> PieceManager::removePiece(PieceId id) {
    auto it = pieces.find(id);
    if (it == pieces.end()) {
        return nullptr;
    }
    
    std::unique_ptr<Piece> removedPiece = std::move(it->second);
    pieces.erase(it);
    cachesDirty = true;
    
    return removedPiece;
}

void PieceManager::movePiece(PieceId id, const Position& newPos) {
    auto it = pieces.find(id);
    if (it == pieces.end() || !it->second) return;
    
    it->second->setPosition(newPos.first, newPos.second);
    // Note: Position change doesn't affect caches since they only store pointers
}

const std::vector<Piece*>& PieceManager::getPieces(Color color) const {
    rebuildCaches();
    return (color == Color::WHITE) ? cachedWhitePieces : cachedBlackPieces;
}

const std::vector<Piece*>& PieceManager::getAllPieces() const {
    rebuildCaches();
    return cachedAllPieces;
}

Piece* PieceManager::findKing(Color color) const {
    rebuildCaches(); // Ensure caches are current
    
    const auto& colorPieces = (color == Color::WHITE) ? cachedWhitePieces : cachedBlackPieces;
    
    for (Piece* piece : colorPieces) {
        if (piece && piece->getType() == PieceType::KING) {
            return piece;
        }
    }
    
    return nullptr;
}

// In PieceManager class:
bool PieceManager::validateKings() const {
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
    auto it = pieces.find(id);
    return (it != pieces.end()) ? it->second.get() : nullptr;
}

size_t PieceManager::getPieceCount(Color color) const {
    rebuildCaches();
    return (color == Color::WHITE) ? cachedWhitePieces.size() : cachedBlackPieces.size();
}

void PieceManager::clear() {
    pieces.clear();
    cachedWhitePieces.clear();
    cachedBlackPieces.clear();
    cachedAllPieces.clear();
    cachesDirty = false; // Caches are now empty and consistent
}