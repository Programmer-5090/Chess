#pragma once

#include <functional>

struct BoardEvent;
enum class BoardEventType;

class BoardEventSystem {
public:
    void subscribe(BoardEventType type, std::function<void(const BoardEvent&)> handler);
    void notify(const BoardEvent& event);
};