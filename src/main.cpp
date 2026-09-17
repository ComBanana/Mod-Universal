#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include "../include/ModMenu.hpp"

using namespace geode::prelude;

class $modify(ModUniversalPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (ModMenu::isNoclipEnabled()) {
            log::debug("Noclip blocked player death");
            return;
        }

        PlayLayer::destroyPlayer(player, object);
    }
};

class $modify(ModUniversalPlayerObject, PlayerObject) {
    bool collidedWithObject(float dt, GameObject* object) {
        if (ModMenu::isNoclipEnabled())
            return false;

        return PlayerObject::collidedWithObject(dt, object);
    }

    bool collidedWithObject(
        float dt,
        GameObject* object,
        CCRect rect,
        bool skipCheck
    ) {
        if (ModMenu::isNoclipEnabled())
            return false;

        return PlayerObject::collidedWithObject(
            dt,
            object,
            rect,
            skipCheck
        );
    }

    void collidedWithSlope(
        float dt,
        GameObject* object,
        bool skipPre
    ) {
        if (ModMenu::isNoclipEnabled())
            return;

        PlayerObject::collidedWithSlope(dt, object, skipPre);
    }
};

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
