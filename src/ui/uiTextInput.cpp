#include "ui/uiTextInput.h"
#include "ui/uiConfig.h"
#include "input.h"
#include <algorithm>
#include <iostream>

UITextInput::~UITextInput() {
	if (font) TTF_CloseFont(font);
}

void UITextInput::ensureFont() {
	if (!font) {
		if (!TTF_WasInit()) TTF_Init();
		font = TTF_OpenFont(fontPath.c_str(), fontSize);
		if (!font) std::cout << "TextInput font load failed: " << TTF_GetError() << std::endl;
	}
}

void UITextInput::ensureTextInputStarted() {
	if (!SDL_IsTextInputActive()) {
		SDL_StartTextInput();
	}
}

void UITextInput::blur() {
	focused = false;
	if (SDL_IsTextInputActive()) SDL_StopTextInput();
}

void UITextInput::insertText(const std::string& s) {
	text.insert(cursor, s);
	cursor += (int)s.size();
	changedSinceLastRender = true;
	if (onChange && UIConfig::areCallbacksEnabled()) onChange(text);
}

void UITextInput::backspace() {
	if (cursor > 0 && !text.empty()) {
		text.erase(cursor - 1, 1);
		cursor--;
		changedSinceLastRender = true;
	if (onChange && UIConfig::areCallbacksEnabled()) onChange(text);
	}
}

void UITextInput::moveCursorLeft() { if (cursor > 0) cursor--; }
void UITextInput::moveCursorRight() { if (cursor < (int)text.size()) cursor++; }

int UITextInput::measureTextWidth(const std::string& s) const {
	if (!font) return 0;
	int w = 0, h = 0;
	std::string display = passwordMode ? std::string(s.size(), maskChar) : s;
	if (TTF_SizeText(font, display.c_str(), &w, &h) != 0) return 0;
	return w;
}

void UITextInput::update(Input& input) {
	if (!visible) return;

	int mx = input.getMouseX();
	int my = input.getMouseY();
	bool mouseDown = input.getMouseStates()["left"];
	SDL_Rect box{ rect.x, rect.y, rect.w, rect.h };
	bool inside = mx >= box.x && mx <= box.x + box.w && my >= box.y && my <= box.y + box.h;

	static bool mouseLatch = false;
	if (mouseDown && inside && !mouseLatch) mouseLatch = true;
	if (!mouseDown && mouseLatch) {
		if (inside) {
			focused = true;
			ensureTextInputStarted();
		} else {
			blur();
		}
		mouseLatch = false;
	}

	if (!focused) return;

	// Handle SDL text input events and key presses
	const SDL_Event& ev = input.getCurrentEvent();
	if (ev.type == SDL_TEXTINPUT) {
		insertText(ev.text.text);
	} else if (ev.type == SDL_KEYDOWN) {
		SDL_Keycode key = ev.key.keysym.sym;
		switch (key) {
			case SDLK_BACKSPACE: backspace(); break;
			case SDLK_LEFT: moveCursorLeft(); break;
			case SDLK_RIGHT: moveCursorRight(); break;
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				if (onSubmit && UIConfig::areCallbacksEnabled()) onSubmit(text);
				break;
		}
	}

	// caret blink
	Uint64 now = SDL_GetTicks64();
	if (now - lastBlink > 500) { caretVisible = !caretVisible; lastBlink = now; }
}

void UITextInput::render(SDL_Renderer* renderer) {
	if (!visible) return;
	ensureFont();

	// box
	SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
	SDL_Rect box{ rect.x, rect.y, rect.w, rect.h };
	SDL_RenderFillRect(renderer, &box);
	SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
	SDL_RenderDrawRect(renderer, &box);

	// text or placeholder
	std::string display = passwordMode ? std::string(text.size(), maskChar) : text;
	SDL_Color col = (!text.empty() || focused) ? textColor : placeholderColor;
	const std::string& toShow = (!text.empty() || focused) ? display : placeholder;
	if (!toShow.empty()) {
		SDL_Surface* surf = TTF_RenderText_Blended(font, toShow.c_str(), col);
		if (surf) {
			SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
			if (tex) {
				SDL_Rect dst{ rect.x + padding, rect.y + (rect.h - surf->h) / 2, surf->w, surf->h };
				SDL_RenderCopy(renderer, tex, nullptr, &dst);
				SDL_DestroyTexture(tex);
			}
			SDL_FreeSurface(surf);
		}
	}

	// caret
	if (focused && caretVisible) {
		int caretX = rect.x + padding + measureTextWidth(display.substr(0, cursor));
		SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b, textColor.a);
		SDL_RenderDrawLine(renderer, caretX, rect.y + 6, caretX, rect.y + rect.h - 6);
	}
}
