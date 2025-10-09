#pragma once
#include <SDL.h>
#include <functional>
#include "uiElement.h"

class Input;

class UISlider : public UIElement {
public:
    UISlider(int x, int y, int w, int h,
             double minVal = 0.0,
             double maxVal = 100.0,
             double value = 50.0,
             SDL_Color trackColor = {220,220,220,255},
             SDL_Color fillColor  = {100,150,240,255},
             SDL_Color borderColor= {60,60,60,255},
             SDL_Color thumbColor = {250,250,250,255},
             SDL_Color thumbBorder= {60,60,60,255})
        : UIElement(x,y,w,h), minVal(minVal), maxVal(maxVal), value(value),
          trackColor(trackColor), fillColor(fillColor), borderColor(borderColor),
          thumbColor(thumbColor), thumbBorder(thumbBorder) {}

    void update(Input& input) override;
    void render(SDL_Renderer* renderer) override;

    void setOnChange(std::function<void(double)> cb) { onChange = std::move(cb); }
    void setValue(double v);
    double getValue() const { return value; }
    void setRange(double minV, double maxV);

private:
    // model
    double minVal = 0.0;
    double maxVal = 100.0;
    double value  = 50.0;

    // visuals
    SDL_Color trackColor;
    SDL_Color fillColor;
    SDL_Color borderColor;
    SDL_Color thumbColor;
    SDL_Color thumbBorder;

    // state
    bool dragging = false;
    bool prevMouseDown = false;

    // helpers
    void updateFromMouseX(int mx);
    int thumbXFromValue() const; // center x of thumb
    static int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
    static double clamp(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }

    std::function<void(double)> onChange;
};
