#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>

#include "../include/ModMenu.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {

    listenForKeybindSettingPresses(
        "toggle-menu",

        [](Keybind const&, bool down, bool repeat, double) {

            if (!down || repeat)
                return;

            ModMenu::toggle();
        }
    );
}
