#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <utility>
#include "enums.h" // use the shared enums (Color, PieceType)

 // Forward declarations
class Piece;
using Position = std::pair<int, int>;
using PieceId = unsigned int;

class PieceManager {
private:
    // Single source of truth - PieceManager owns all pieces
    std::unordered_map<PieceId, std::unique_ptr<Piece>> pieces;
    
    // Cached views (raw pointers, rebuilt when dirty)
    mutable std::vector<Piece*> cachedWhitePieces;
    mutable std::vector<Piece*> cachedBlackPieces;
    mutable std::vector<Piece*> cachedAllPieces;
    mutable bool cachesDirty = true;
    
    void rebuildCaches() const;

public:
    // Ownership transfer - PieceManager takes ownership
    void addPiece(std::unique_ptr<Piece> piece);
    
    // Remove and return ownership (for captures, promotions)
    std::unique_ptr<Piece> removePiece(PieceId id);
    
    // Update piece position
    void movePiece(PieceId id, const Position& newPos);
    
    // Access pieces (const views)
    const std::vector<Piece*>& getPieces(Color color) const;
    const std::vector<Piece*>& getAllPieces() const;
    
    // Find specific pieces
    Piece* findKing(Color color) const;
    // In PieceManager class:
    bool PieceManager::validateKings() const;
    Piece* getPieceById(PieceId id) const;
    
    // Utility
    size_t getPieceCount(Color color) const;
    std::unordered_map<PieceId, std::unique_ptr<Piece>>& getAllPieceMap() { return pieces; }
    void clear();
};