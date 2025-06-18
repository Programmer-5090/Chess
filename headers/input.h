#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>


class Input {
    SDL_Event event;
    bool quit = false;
    std::vector<std::string> keysDown, keysHeld, keysUp;
    std::map<std::string,bool> mouseButtons;
    std::pair<int, int> mousePos;
    //bool mouseIsDragging;
  public:
    Input();
    void resetStates();
    void update();
    bool shouldQuit()    const;
    bool keyDown(const std::string& k) const;
    bool keyUp(const std::string& k)   const;
    bool keyHeld(const std::string& k) const;
    std::map<std::string,bool> getMouseStates() const;
    const std::vector<std::string>& getKeysDown() const;
    std::pair<int, int> getMousePos() const;
    
  private:
    void updateMouse(int b, bool d);
};