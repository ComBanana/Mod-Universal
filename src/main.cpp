#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

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

    // Apply the new 0.1.3 Noclip defaults once to existing installs. After
    // this migration, normal setting changes are preserved as usual.
    if (!Mod::get()->getSavedValue<bool>(
            "noclip-defaults-v0.1.3-applied",
            false
        )) {
        Mod::get()->setSettingValue<bool>("noclip-enabled", true);
        Mod::get()->setSettingValue<bool>("noclip-phase-blocks", true);
        Mod::get()->setSettingValue<std::string>("noclip-block-mode", "safe-touch");
        Mod::get()->setSettingValue<bool>("noclip-phase-slopes", true);
        Mod::get()->setSettingValue<bool>("noclip-phase-hazards", true);
        Mod::get()->setSettingValue<bool>("noclip-hazard-spikes", true);
        Mod::get()->setSettingValue<bool>("noclip-hazard-ground-spikes", true);
        Mod::get()->setSettingValue<bool>("noclip-hazard-saws", true);
        Mod::get()->setSettingValue<bool>("noclip-hazard-pits", true);
        Mod::get()->setSettingValue<bool>("noclip-hazard-animated", true);
        Mod::get()->setSettingValue<bool>("noclip-hazard-other", true);

        Mod::get()->setSavedValue<bool>(
            "noclip-defaults-v0.1.3-applied",
            true
        );

        saveNoclipSettings();
        log::info("0.1.3 Noclip defaults initialized.");
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
