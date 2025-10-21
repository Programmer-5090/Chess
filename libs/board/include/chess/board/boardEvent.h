#ifndef BOARD_EVENT_H
#define BOARD_EVENT_H

#include <functional>

struct BoardEvent;
enum class BoardEventType;

class BoardEventSystem {
public:
    void subscribe(BoardEventType type, std::function<void(const BoardEvent&)> handler);
    void notify(const BoardEvent& event);
};

#endif // BOARD_EVENT_H