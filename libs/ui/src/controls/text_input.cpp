#include "chess/ui/controls/text_input.h"
#include "chess/ui/controls/ui/uiConfig.h"
#include "chess/ui/input.h"
#include <chess/utils/logger.h>
#include <algorithm>
#include <iostream>

UITextInput::~UITextInput() {
	if (font) TTF_CloseFont(font);
}

void UITextInput::ensureFont() {
	if (!font) {
		if (!TTF_WasInit()) TTF_Init();
		font = TTF_OpenFont(fontPath.c_str(), fontSize);
		if (!font) LOG_ERROR(std::string("TextInput font load failed: ") + TTF_GetError());
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
	ensureCaretVisible();
	if (onChange && UIConfig::areCallbacksEnabled()) onChange(text);
}

void UITextInput::backspace() {
	if (cursor > 0 && !text.empty()) {
		text.erase(cursor - 1, 1);
		cursor--;
		changedSinceLastRender = true;
	ensureCaretVisible();
	if (onChange && UIConfig::areCallbacksEnabled()) onChange(text);
	}
}

void UITextInput::moveCursorLeft() { if (cursor > 0) { cursor--; ensureCaretVisible(); } }
void UITextInput::moveCursorRight() { if (cursor < (int)text.size()) { cursor++; ensureCaretVisible(); } }

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
				if (multiline) { insertText("\n"); }
				else { if (onSubmit && UIConfig::areCallbacksEnabled()) onSubmit(text); }
				break;
			case SDLK_UP:
				if (multiline && firstVisibleLine > 0) firstVisibleLine--;
				break;
			case SDLK_DOWN:
				if (multiline) firstVisibleLine++;
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

	// text or placeholder with horizontal scroll/clipping (single-line) or multiline box
	std::string raw = text;
	std::string display = passwordMode ? std::string(raw.size(), maskChar) : raw;
	SDL_Color col = (!raw.empty() || focused) ? textColor : placeholderColor;
	const std::string& toShow = (!raw.empty() || focused) ? display : placeholder;

	// compute available inner rect
	SDL_Rect inner{ rect.x + padding, rect.y + padding, rect.w - 2 * padding, rect.h - 2 * padding };

	if (multiline) {
		renderMultiline(renderer, inner);
	} else if (!toShow.empty()) {
		SDL_Surface* surf = TTF_RenderText_Blended(font, toShow.c_str(), col);
		if (surf) {
			SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
			if (tex) {
				// Apply horizontal scroll via source rect x offset
				SDL_Rect src{ scrollOffsetPx, 0, inner.w, surf->h };
				if (src.x < 0) src.x = 0;
				if (src.x + src.w > surf->w) src.w = std::max(0, surf->w - src.x);
				SDL_Rect dst{ inner.x, rect.y + (rect.h - surf->h) / 2, src.w, surf->h };
				if (src.w > 0) SDL_RenderCopy(renderer, tex, &src, &dst);
				SDL_DestroyTexture(tex);
			}
			SDL_FreeSurface(surf);
		}
	}

	// caret
	if (focused && caretVisible && !multiline) {
		int caretXAbs = rect.x + padding + measureTextWidth(display.substr(0, cursor));
		int caretX = caretXAbs - scrollOffsetPx; // apply scroll
		// Clamp caret drawing within inner box
		caretX = std::max(rect.x + padding, std::min(rect.x + rect.w - padding, caretX));
		SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b, textColor.a);
		SDL_RenderDrawLine(renderer, caretX, rect.y + 6, caretX, rect.y + rect.h - 6);
	}
}

void UITextInput::ensureCaretVisible() {
	// Ensure font to measure widths
	if (!font) ensureFont();
	if (multiline) {
		updateLineMetrics();
		// Keep firstVisibleLine non-negative; more nuanced caret tracking can be added later
		if (firstVisibleLine < 0) firstVisibleLine = 0;
		return;
	}
	std::string raw = text;
	std::string display = passwordMode ? std::string(raw.size(), maskChar) : raw;
	int caretPxFromStart = measureTextWidth(display.substr(0, cursor));
	int innerLeft = rect.x + padding;
	int innerRight = rect.x + rect.w - padding;
	int visibleLeftPx = scrollOffsetPx;
	int visibleRightPx = scrollOffsetPx + (innerRight - innerLeft);

	if (caretPxFromStart < visibleLeftPx) {
		scrollOffsetPx = std::max(0, caretPxFromStart);
	} else if (caretPxFromStart > visibleRightPx) {
		scrollOffsetPx = caretPxFromStart - (innerRight - innerLeft) + 1;
	}

	int totalW = measureTextWidth(display);
	int viewport = (innerRight - innerLeft);
	int maxScroll = std::max(0, totalW - viewport);
	if (scrollOffsetPx > maxScroll) scrollOffsetPx = maxScroll;
	if (scrollOffsetPx < 0) scrollOffsetPx = 0;
}

void UITextInput::updateLineMetrics() {
	if (!font) return;
	int ascent = TTF_FontAscent(font);
	int descent = TTF_FontDescent(font);
	lineHeightPx = ascent - descent + 2; // rough line height
}

void UITextInput::renderMultiline(SDL_Renderer* renderer, const SDL_Rect& inner) {
	// Simple word-wrapped multi-line render with vertical scrolling (firstVisibleLine)
	if (!font) return;
	updateLineMetrics();
	std::vector<std::string> lines;
	lines.reserve(64);
	// Split by existing newlines then wrap each paragraph to inner.w
	size_t start = 0;
	while (start <= text.size()) {
		size_t nl = text.find('\n', start);
		std::string para = (nl == std::string::npos) ? text.substr(start) : text.substr(start, nl - start);
		// wrap para
		size_t i = 0; size_t lastBreak = 0; int widthAccum = 0;
		auto flush = [&](size_t from, size_t to){ lines.emplace_back(para.substr(from, to - from)); };
		while (i < para.size()) {
			size_t j = i;
			// advance one word
			while (j < para.size() && !isspace((unsigned char)para[j])) j++;
			std::string word = para.substr(i, j - i);
			int w = 0, h = 0; TTF_SizeText(font, word.c_str(), &w, &h);
			if (widthAccum == 0 && w > inner.w) {
				// word longer than line, hard break
				// cut roughly to fit
				size_t cut = std::max<size_t>(1, inner.w / std::max(1, fontSize/2));
				flush(i, std::min(para.size(), i + cut));
				i += cut; widthAccum = 0; lastBreak = i; continue;
			}
			if (widthAccum + (widthAccum ? fontSize/2 : 0) + w > inner.w) {
				// wrap before this word
				flush(lastBreak, i);
				widthAccum = 0;
			}
			if (widthAccum) widthAccum += fontSize/2; // approximate space width
			widthAccum += w;
			i = j;
			// include trailing spaces up to next word
			while (i < para.size() && isspace((unsigned char)para[i]) && para[i] != '\n') i++;
			lastBreak = i;
		}
		// flush remainder
		if (lastBreak < para.size()) flush(lastBreak, para.size()); else if (para.empty()) lines.emplace_back("");
		if (nl == std::string::npos) break; else start = nl + 1, lines.emplace_back("");
	}

	// Render visible slice
	int y = inner.y;
	int maxLines = std::max(1, inner.h / std::max(1, lineHeightPx));
	if (firstVisibleLine < 0) firstVisibleLine = 0;
	if (firstVisibleLine > (int)lines.size()) firstVisibleLine = (int)lines.size();
	SDL_Rect clipPrev; SDL_RenderGetClipRect(renderer, &clipPrev);
	SDL_RenderSetClipRect(renderer, &inner);
	for (int li = 0; li < maxLines && (firstVisibleLine + li) < (int)lines.size(); ++li) {
		const std::string& ln = lines[firstVisibleLine + li];
		SDL_Surface* surf = TTF_RenderText_Blended(font, ln.c_str(), textColor);
		if (!surf) continue;
		SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
		SDL_Rect dst{ inner.x, y, surf->w, surf->h };
		SDL_RenderCopy(renderer, tex, nullptr, &dst);
		SDL_DestroyTexture(tex);
		SDL_FreeSurface(surf);
		y += lineHeightPx;
		if (y > inner.y + inner.h) break;
	}
	SDL_RenderSetClipRect(renderer, &clipPrev);
}
