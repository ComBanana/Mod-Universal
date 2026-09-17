#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>

#include "../include/ModMenu.hpp"

using namespace geode::prelude;

namespace {

void saveNoclipSettings() {
    if (auto result = Mod::get()->saveData(); !result) {
        log::error("Failed to save Noclip settings: {}", result.unwrapErr());
    }
}

template <typename T>
void saveNoclipSetting(T) {
    saveNoclipSettings();
}

} // namespace

$on_mod(Loaded) {

    // Noclip settings are edited directly by the custom ModMenu UI.
    // Save them immediately so changes persist when the menu is closed.
    listenForSettingChanges<bool>("noclip-enabled", saveNoclipSetting<bool>);
    listenForSettingChanges<bool>("noclip-phase-blocks", saveNoclipSetting<bool>);
    listenForSettingChanges<std::string>("noclip-block-mode", saveNoclipSetting<std::string>);
    listenForSettingChanges<bool>("noclip-phase-slopes", saveNoclipSetting<bool>);
    listenForSettingChanges<bool>("noclip-phase-hazards", saveNoclipSetting<bool>);
    listenForSettingChanges<bool>("noclip-hazard-spikes", saveNoclipSetting<bool>);
    listenForSettingChanges<bool>("noclip-hazard-ground-spikes", saveNoclipSetting<bool>);
    listenForSettingChanges<bool>("noclip-hazard-saws", saveNoclipSetting<bool>);
    listenForSettingChanges<bool>("noclip-hazard-pits", saveNoclipSetting<bool>);
    listenForSettingChanges<bool>("noclip-hazard-animated", saveNoclipSetting<bool>);
    listenForSettingChanges<bool>("noclip-hazard-other", saveNoclipSetting<bool>);

    listenForKeybindSettingPresses(
        "toggle-menu",

        [](Keybind const&, bool down, bool repeat, double) {

            if (!down || repeat)
                return;

            // F3 is the ModUniversal menu's open/close key. The menu's
            // toggle path removes the popup directly, so persist settings
            // here to guarantee that the close action writes to disk.
            saveNoclipSettings();
            ModMenu::toggle();
        }
    );
}
