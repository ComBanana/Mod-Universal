#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "../include/ModMenu.hpp"
#include "../include/ui/Overlay.hpp"

using namespace geode::prelude;

namespace {

void saveNoclipSettings() {
    if (auto result = Mod::get()->saveData(); !result) {
        log::error("Failed to save Noclip settings: {}", result.unwrapErr());
    }
}

} // namespace

$on_mod(Loaded) {

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

    listenForKeybindSettingPresses(
        "toggle-noclip",
        [](Keybind const&, bool down, bool repeat, double) {
            if (!down || repeat)
                return;

            ModMenu::setNoclipEnabled(!ModMenu::isNoclipEnabled());
            overlay::saveModData();
            ModMenu::refreshCurrentTab();
        }
    );

    listenForKeybindSettingPresses(
        "toggle-hitboxes",
        [](Keybind const&, bool down, bool repeat, double) {
            if (!down || repeat)
                return;

            bool const enabled =
                !Mod::get()->getSettingValue<bool>("show-hitboxes");
            Mod::get()->setSettingValue<bool>("show-hitboxes", enabled);
            overlay::saveModData();
            ModMenu::refreshCurrentTab();
        }
    );
}
