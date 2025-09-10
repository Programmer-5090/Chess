#include "../include/input.h"

Input::Input() {
    mouseButtons["left"] = mouseButtons["right"] = mouseButtons["middle"] = false;
    mousePos = std::make_pair(0, 0);
    quit = false;
}

void Input::resetStates() {
    keysDown.clear(); 
    keysUp.clear();
}

void Input::update() {
    resetStates();
    events.clear();
    while (SDL_PollEvent(&event)) {
        events.push_back(event);
        if (event.type == SDL_QUIT) {
            quit = true;
        } else if (event.type == SDL_KEYDOWN) {
            const char* keyNameCStr = SDL_GetKeyName(event.key.keysym.sym);
            if (keyNameCStr && keyNameCStr[0] != '\0') { // Check for non-empty string
                std::string name(keyNameCStr);
                if (std::find(keysHeld.begin(), keysHeld.end(), name) == keysHeld.end()) {
                    keysDown.push_back(name);
                    keysHeld.push_back(name);
                }
            }
        } else if (event.type == SDL_KEYUP) {
            const char* keyNameCStr = SDL_GetKeyName(event.key.keysym.sym);
            if (keyNameCStr && keyNameCStr[0] != '\0') {
                std::string name(keyNameCStr);
                auto it = std::find(keysHeld.begin(), keysHeld.end(), name);
                if (it != keysHeld.end()) {
                    keysHeld.erase(it);
                    keysUp.push_back(name);
                }
            }
        } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            bool down = (event.type == SDL_MOUSEBUTTONDOWN);
            updateMouse(event.button.button, down);
        }
    }
    // Update mouse position every frame
    int x, y;
    SDL_GetMouseState(&x, &y);
    mousePos.first = x;
    mousePos.second = y;
}

bool Input::shouldQuit() const { 
    return quit; 
}

bool Input::keyDown(const std::string& k) const { 
    return std::find(keysDown.begin(), keysDown.end(), k) != keysDown.end(); 
}

bool Input::keyUp(const std::string& k) const { 
    return std::find(keysUp.begin(), keysUp.end(), k) != keysUp.end(); 
}

bool Input::keyHeld(const std::string& k) const { 
    return std::find(keysHeld.begin(), keysHeld.end(), k) != keysHeld.end(); 
}

std::map<std::string, bool> Input::getMouseStates() const { 
    return mouseButtons; 
}
    
const std::vector<std::string>& Input::getKeysDown() const { 
    return keysDown; 
}

std::pair<int, int> Input::getMousePos() const {
    return mousePos;
}
    
void Input::updateMouse(int b, bool d) {
    if (b == SDL_BUTTON_LEFT)   mouseButtons["left"]   = d;
    if (b == SDL_BUTTON_MIDDLE) mouseButtons["middle"] = d;
    if (b == SDL_BUTTON_RIGHT)  mouseButtons["right"]  = d;
}

// Added for UI system integration
bool Input::isMouseButtonDown(int button) const {
    switch (button) {
        case SDL_BUTTON_LEFT:   return mouseButtons.at("left");
        case SDL_BUTTON_MIDDLE: return mouseButtons.at("middle");
        case SDL_BUTTON_RIGHT:  return mouseButtons.at("right");
        default:                return false;
    }
}

bool Input::isMouseButtonReleased(int button) const {
    if (event.type != SDL_MOUSEBUTTONUP) return false;
    return event.button.button == button;
}

bool Input::isMouseButtonPressed(int button) const {
    if (event.type != SDL_MOUSEBUTTONDOWN) return false;
    return event.button.button == button;
}
