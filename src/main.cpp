#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/PlayLayer.hpp>

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

    // EXPERIMENTAL ONLY: establish a clean baseline for the rewritten
    // Noclip UI the first time this branch is run. This happens once and
    // does not repeatedly overwrite later choices.
    if (!Mod::get()->getSavedValue<bool>(
            "experimental-noclip-button-rewrite-v1",
            false
        )) {
        Mod::get()->setSettingValue<bool>("noclip-enabled", false);
        Mod::get()->setSettingValue<bool>("noclip-phase-blocks", false);
        Mod::get()->setSettingValue<std::string>("noclip-block-mode", "safe-touch");
        Mod::get()->setSettingValue<bool>("noclip-phase-slopes", false);
        Mod::get()->setSettingValue<bool>("noclip-phase-hazards", false);
        Mod::get()->setSettingValue<bool>("noclip-hazard-spikes", false);
        Mod::get()->setSettingValue<bool>("noclip-hazard-ground-spikes", false);
        Mod::get()->setSettingValue<bool>("noclip-hazard-saws", false);
        Mod::get()->setSettingValue<bool>("noclip-hazard-pits", false);
        Mod::get()->setSettingValue<bool>("noclip-hazard-animated", false);
        Mod::get()->setSettingValue<bool>("noclip-hazard-other", false);

        Mod::get()->setSavedValue<bool>(
            "experimental-noclip-button-rewrite-v1",
            true
        );

        saveNoclipSettings();
        log::info("Experimental Noclip UI baseline initialized.");
    }

    listenForKeybindSettingPresses(
        "toggle-menu",

        [](Keybind const&, bool down, bool repeat, double) {

            if (!down || repeat)
                return;

            // Allow the menu while the level is paused, but never during
            // active unpaused gameplay.
            if (auto* playLayer = PlayLayer::get();
                playLayer && !playLayer->m_isPaused) {
                return;
            }

            // F3 is the ModUniversal menu's back/open key. ModMenu::toggle()
            // closes the deepest nested popup first, then the main menu.
            saveNoclipSettings();
            ModMenu::toggle();
        }
    );
}
