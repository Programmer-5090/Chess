#include "chess/ui/layouts/builder.h"
#include "chess/ui/controls/ui/uiCommon.h"
#include <iostream>

UIBuilder::UIBuilder(UIManager* mgr, const std::string& fontPath)
    : manager(mgr), defaultFontPath(fontPath) {
}

void UIBuilder::beginVerticalLayout(SDL_Rect bounds, int padding) {
    UILayout::beginVertical(layoutManager, bounds, padding);
}

void UIBuilder::beginHorizontalLayout(SDL_Rect bounds, int padding) {
    UILayout::beginHorizontal(layoutManager, bounds, padding);
}

void UIBuilder::endLayout() {
    UILayout::end(layoutManager);
}

Button* UIBuilder::button(const std::string& text, std::function<void()> callback, 
                         int width, int height) {
    SDL_Rect rect = UILayout::buttonRect(layoutManager, width, height);
    
    return manager->addElement<Button>(
        rect.x, rect.y, rect.w, rect.h,
        text, 
        callback,
        tupleToColor(100, 150, 200, 255), // default blue
        tupleToColor(130, 180, 230, 255), // hover
        defaultFontPath,
        tupleToColor(255, 255, 255, 255), // white text
        4, 20
    );
}

Label* UIBuilder::label(const std::string& text, SDL_Color color, int fontSize) {
    SDL_Rect rect = UILayout::labelRect(layoutManager, text, fontSize);
    
    return manager->addElement<Label>(
        rect.x, rect.y, text, color, fontSize, defaultFontPath
    );
}

UICheckbox* UIBuilder::checkbox(const std::string& text, bool checked,
                               std::function<void(bool)> onChange) {
    SDL_Rect rect = layoutManager.getNextElementRect(24 + static_cast<int>(text.length() * 8), 28);
    layoutManager.addElementToLayout(rect);
    
    UICheckbox* cb = manager->addElement<UICheckbox>(
        rect.x, rect.y, 24, text, checked,
        tupleToColor(220, 220, 220, 255), // box
        tupleToColor(60, 180, 75, 255),   // check
        tupleToColor(80, 80, 80, 255),    // border
        tupleToColor(255, 255, 255, 255), // text
        18, defaultFontPath
    );
    
    if (onChange) {
        cb->setOnChange(onChange);
    }
    
    return cb;
}

UISlider* UIBuilder::slider(double minVal, double maxVal, double value,
                           int width, std::function<void(double)> onChange) {
    SDL_Rect rect = layoutManager.getNextElementRect(width, 24);
    layoutManager.addElementToLayout(rect);
    
    UISlider* s = manager->addElement<UISlider>(
        rect.x, rect.y, rect.w, rect.h,
        minVal, maxVal, value
    );
    
    if (onChange) {
        s->setOnChange(onChange);
    }
    
    return s;
}

UITextInput* UIBuilder::textInput(const std::string& placeholder, int width,
                                 std::function<void(const std::string&)> onSubmit) {
    SDL_Rect rect = layoutManager.getNextElementRect(width, 32);
    layoutManager.addElementToLayout(rect);
    
    UITextInput* ti = manager->addElement<UITextInput>(
        rect.x, rect.y, rect.w, rect.h,
        placeholder, defaultFontPath
    );
    
    if (onSubmit) {
        ti->setOnSubmit(onSubmit);
    }
    
    return ti;
}

UIDropdown* UIBuilder::dropdown(const std::vector<std::string>& options, int selectedIndex,
                               int width, std::function<void(int, const std::string&)> onChange) {
    SDL_Rect rect = layoutManager.getNextElementRect(width, 32);
    layoutManager.addElementToLayout(rect);
    
    UIDropdown* dd = manager->addElement<UIDropdown>(
        rect.x, rect.y, rect.w, rect.h,
        options, selectedIndex, defaultFontPath
    );
    
    if (onChange) {
        dd->setOnChange(onChange);
    }
    
    return dd;
}

UIDialog* UIBuilder::dialog(const std::string& title, const std::string& message,
                           std::function<void()> onOk, std::function<void()> onCancel) {
    UIDialog* dlg = manager->addElement<UIDialog>(
        200, 160, 400, 220, title, message,
        "OK", "Cancel", defaultFontPath
    );
    
    if (onOk) {
        dlg->setOnOk([dlg, onOk](){ onOk(); dlg->visible = false; });
    } else {
        dlg->setOnOk([dlg](){ dlg->visible = false; });
    }
    
    if (onCancel) {
        dlg->setOnCancel([dlg, onCancel](){ onCancel(); dlg->visible = false; });
    } else {
        dlg->setOnCancel([dlg](){ dlg->visible = false; });
    }
    
    return dlg;
}

UIPanel* UIBuilder::panel(SDL_Rect rect, SDL_Color bg) {
    return manager->addElement<UIPanel>(
        rect.x, rect.y, rect.w, rect.h, bg
    );
}

void UIBuilder::spacing(int pixels) {
    auto* layout = layoutManager.getCurrentLayout();
    if (!layout) return;
    
    if (layout->kind == UILayoutManager::LayoutKind::Vertical) {
        SDL_Rect spaceRect = { layout->pos.x, layout->pos.y + layout->size.y, 0, pixels };
        layoutManager.addElementToLayout(spaceRect);
    } else if (layout->kind == UILayoutManager::LayoutKind::Horizontal) {
        SDL_Rect spaceRect = { layout->pos.x + layout->size.x, layout->pos.y, pixels, 0 };
        layoutManager.addElementToLayout(spaceRect);
    }
}

void UIBuilder::sameLine() {
    // TODO: Implement same-line behavior for next element
    // This would temporarily switch to horizontal layout for one element
}

void UIBuilder::setNextElementPos(int x, int y) {
    // TODO: Implement manual positioning that bypasses current layout
}

void UIBuilder::clear() {
    layoutManager.clear();
    manager->clearElements();
}
