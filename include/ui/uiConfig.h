#pragma once

namespace UIConfig {
    // Global flag for user-toggled callbacks state
    inline bool& callbacksEnabledFlag() {
        static bool flag = true; // enabled by default
        return flag;
    }
    // Edit mode override - when true, all callbacks are disabled except bypass buttons
    inline bool& editModeActive() {
        static bool active = false;
        return active;
    }
    
    inline void setCallbacksEnabled(bool enabled) { callbacksEnabledFlag() = enabled; }
    inline void setEditModeActive(bool active) { editModeActive() = active; }
    inline bool areCallbacksEnabled() { return callbacksEnabledFlag() && !editModeActive(); }
}
