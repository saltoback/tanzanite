// Tanzanite — Auto Deafen
// When a run (not in practice) reaches the chosen percentage, the mod presses
// the keyboard shortcut you set in Discord for "Toggle Deafen". When the run
// ends (death / restart / leaving the level) it presses it again to undeafen.
#include "Deafen.hpp"
#include "State.hpp"

#include <vector>

#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#endif

using namespace geode::prelude;
using namespace tanzanite;

namespace {
    bool s_deafened = false;     // we pressed the shortcut during this run
    float s_runStart = 0.f;      // % where this attempt started
}

bool deafen::supported() {
#ifdef GEODE_IS_WINDOWS
    return true;
#else
    return false;
#endif
}

std::string deafen::shortcutName() {
    std::string name;
    if (s.deafenCtrl) name += "Ctrl + ";
    if (s.deafenShift) name += "Shift + ";
    if (s.deafenAlt) name += "Alt + ";
#ifdef GEODE_IS_WINDOWS
    char buf[64] = {};
    LONG scan = static_cast<LONG>(MapVirtualKeyA(static_cast<UINT>(s.deafenKey), MAPVK_VK_TO_VSC)) << 16;
    if (s.deafenKey >= VK_PRIOR && s.deafenKey <= VK_HELP) scan |= 1 << 24;   // extended keys
    if (GetKeyNameTextA(scan, buf, sizeof(buf)) > 0) name += buf;
    else name += fmt::format("Key {}", s.deafenKey);
#else
    name += fmt::format("Key {}", s.deafenKey);
#endif
    return name;
}

void deafen::press() {
#ifdef GEODE_IS_WINDOWS
    std::vector<INPUT> inputs;
    auto key = [&](WORD vk, bool up) {
        INPUT in = {};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = vk;
        in.ki.wScan = static_cast<WORD>(MapVirtualKeyA(vk, MAPVK_VK_TO_VSC));
        in.ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
        if (vk >= VK_PRIOR && vk <= VK_HELP) in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        inputs.push_back(in);
    };
    if (s.deafenCtrl) key(VK_CONTROL, false);
    if (s.deafenShift) key(VK_SHIFT, false);
    if (s.deafenAlt) key(VK_MENU, false);
    key(static_cast<WORD>(s.deafenKey), false);
    key(static_cast<WORD>(s.deafenKey), true);
    if (s.deafenAlt) key(VK_MENU, true);
    if (s.deafenShift) key(VK_SHIFT, true);
    if (s.deafenCtrl) key(VK_CONTROL, true);
    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
#endif
}

bool deafen::capture(int vk) {
    if (!capturing) return false;
#ifdef GEODE_IS_WINDOWS
    // Modifiers alone: wait for the real key
    switch (vk) {
        case VK_SHIFT: case VK_CONTROL: case VK_MENU:
        case VK_LSHIFT: case VK_RSHIFT: case VK_LCONTROL: case VK_RCONTROL:
        case VK_LMENU: case VK_RMENU: case VK_LWIN: case VK_RWIN:
            return true;
        case VK_ESCAPE:
            capturing = false;
            version++;
            return true;
        default: break;
    }
    // The modifiers held right now become part of the shortcut
    s.deafenCtrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    s.deafenShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    s.deafenAlt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
#endif
    s.deafenKey = vk;
    capturing = false;
    version++;
    tanzanite::save();
    return true;
}

void deafen::onUpdate(PlayLayer* pl) {
    if (!supported() || !s.autoDeafen || s_deafened || !pl) return;
    if (pl->m_isPracticeMode || pl->m_hasCompletedLevel) return;
    if (pl->m_player1 && pl->m_player1->m_isDead) return;
    // Starting past the % (start pos) doesn't count
    if (s_runStart >= s.deafenPercent) return;
    if (pl->getCurrentPercent() >= s.deafenPercent) {
        press();
        s_deafened = true;
    }
}

void deafen::onReset() {
    // Undeafen when the run ends. With the option off, stay deafened until
    // leaving the level (and don't press again on the next runs).
    if (s_deafened && s.undeafenOnDeath) {
        press();
        s_deafened = false;
    }
    if (auto pl = PlayLayer::get()) s_runStart = pl->getCurrentPercent();
    else s_runStart = 0.f;
}

void deafen::onQuit() {
    if (s_deafened) press();
    s_deafened = false;
    s_runStart = 0.f;
}
