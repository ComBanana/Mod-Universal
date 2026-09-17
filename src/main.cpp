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

} // namespace

$on_mod(Loaded) {

    // Noclip settings are edited directly by the custom ModMenu UI.
    // Save them immediately so closing the menu does not lose changes.
    listenForSettingChanges<bool>("noclip-enabled", saveNoclipSettings);
    listenForSettingChanges<bool>("noclip-phase-blocks", saveNoclipSettings);
    listenForSettingChanges<std::string>("noclip-block-mode", saveNoclipSettings);
    listenForSettingChanges<bool>("noclip-phase-slopes", saveNoclipSettings);
    listenForSettingChanges<bool>("noclip-phase-hazards", saveNoclipSettings);
    listenForSettingChanges<bool>("noclip-hazard-spikes", saveNoclipSettings);
    listenForSettingChanges<bool>("noclip-hazard-ground-spikes", saveNoclipSettings);
    listenForSettingChanges<bool>("noclip-hazard-saws", saveNoclipSettings);
    listenForSettingChanges<bool>("noclip-hazard-pits", saveNoclipSettings);
    listenForSettingChanges<bool>("noclip-hazard-animated", saveNoclipSettings);
    listenForSettingChanges<bool>("noclip-hazard-other", saveNoclipSettings);

    listenForKeybindSettingPresses(
        "toggle-menu",

        [](Keybind const&, bool down, bool repeat, double) {

            if (!down || repeat)
                return;

            ModMenu::toggle();
        }
    );
}
