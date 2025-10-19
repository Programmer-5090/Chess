#include <chess/menus/vsCompMenu.h>
#include <chess/ui/input.h>

void VSCompMenu::render() {
    uiManager.render();
}

void VSCompMenu::update(Input& input) {
    uiManager.update(input);
}