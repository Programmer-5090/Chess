#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>
#include <unordered_map>
#include <array>

/**
 * Input class to handle keyboard and mouse input
 */
class Input {
public:
    Input();
    ~Input();
    
    void update();
    
    // Keyboard state
    bool isKeyDown(SDL_Scancode key) const;
    bool isKeyPressed(SDL_Scancode key) const;
    bool isKeyReleased(SDL_Scancode key) const;
    
    // Mouse state
    bool isMouseButtonDown(Uint8 button) const;
    bool isMouseButtonPressed(Uint8 button) const;
    bool isMouseButtonReleased(Uint8 button) const;
    
    int getMouseX() const;
    int getMouseY() const;
    int getMouseDeltaX() const;
    int getMouseDeltaY() const;
    
    // Text input
    const char* getTextInput() const;
    void startTextInput();
    void stopTextInput();
    
private:
    std::array<Uint8, SDL_NUM_SCANCODES> currentKeyStates;
    std::array<Uint8, SDL_NUM_SCANCODES> previousKeyStates;
    
    std::array<Uint8, 5> currentMouseStates;  // Up to 5 mouse buttons
    std::array<Uint8, 5> previousMouseStates;
    
    int mouseX;
    int mouseY;
    int mouseDeltaX;
    int mouseDeltaY;
    
    char textInputBuffer[32];
    bool textInputActive;
};

#endif // INPUT_H
