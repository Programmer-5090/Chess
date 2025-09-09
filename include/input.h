#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>

class Input {
public:
    Input();
    
    void resetStates();
    void update();
    bool shouldQuit() const;
    bool keyDown(const std::string& k) const;
    bool keyUp(const std::string& k) const;
    bool keyHeld(const std::string& k) const;
    std::map<std::string, bool> getMouseStates() const;
    const std::vector<std::string>& getKeysDown() const;
    std::pair<int, int> getMousePos() const;
    
    // Added for UI system integration
    int getMouseX() const { return mousePos.first; }
    int getMouseY() const { return mousePos.second; }
    bool isMouseButtonDown(int button) const;
    bool isMouseButtonReleased(int button) const;
    bool isMouseButtonPressed(int button) const;
    
    // Store current event for UI system
    const SDL_Event& getCurrentEvent() const { return event; }

private:
    SDL_Event event;
    bool quit = false;
    std::vector<std::string> keysDown, keysHeld, keysUp;
    std::map<std::string, bool> mouseButtons;
    std::pair<int, int> mousePos;

    void updateMouse(int b, bool d);
};

#endif // INPUT_H
