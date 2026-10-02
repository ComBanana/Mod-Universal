#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include "../include/ModMenu.hpp"

using namespace geode::prelude;

namespace {

enum class HazardCategory {
    None = -1,
    Spikes = 0,
    GroundSpikes = 1,
    Saws = 2,
    Pits = 3,
    Animated = 4,
    Other = 5,
};

HazardCategory classifyHazard(GameObject* object) {
    if (!object)
        return HazardCategory::None;

    switch (object->m_objectID) {
        case 8:
        case 103:
        case 144:
        case 151:
        case 152:
        case 153:
        case 177:
        case 179:
        case 216:
        case 217:
        case 363:
        case 364:
        case 365:
        case 392:
        case 397:
        case 398:
        case 399:
        case 458:
        case 459:
        case 1708:
        case 1709:
        case 1710:
        case 1716:
        case 1725:
        case 1732:
        case 1733:
            return HazardCategory::Spikes;

        case 39:
        case 145:
        case 178:
        case 205:
        case 218:
        case 421:
        case 422:
        case 1717:
        case 1718:
        case 1726:
        case 1727:
            return HazardCategory::GroundSpikes;

        case 88:
        case 89:
        case 98:
        case 183:
        case 184:
        case 185:
        case 186:
        case 187:
        case 188:
        case 1705:
        case 1706:
        case 1707:
        case 1734:
        case 1735:
        case 1736:
            return HazardCategory::Saws;

        case 9:
        case 61:
        case 135:
        case 243:
        case 244:
        case 1711:
        case 1712:
        case 1713:
        case 1714:
        case 1715:
        case 1719:
        case 1720:
        case 1721:
            return HazardCategory::Pits;

        case 1701:
        case 1702:
        case 1703:
            return HazardCategory::Animated;

        default:
            break;
    }

    // Use Geometry Dash's object type as the fallback so that unknown
    // hazard objects are covered by the "Other Hazards" setting.
    if (object->m_objectType == GameObjectType::AnimatedHazard)
        return HazardCategory::Animated;

    if (object->m_objectType == GameObjectType::Hazard)
        return HazardCategory::Other;

    return HazardCategory::None;
}

bool isHazardCategoryEnabled(HazardCategory category) {
    switch (category) {
        case HazardCategory::Spikes:
            return ModMenu::isHazardCategoryEnabled(0);
        case HazardCategory::GroundSpikes:
            return ModMenu::isHazardCategoryEnabled(1);
        case HazardCategory::Saws:
            return ModMenu::isHazardCategoryEnabled(2);
        case HazardCategory::Pits:
            return ModMenu::isHazardCategoryEnabled(3);
        case HazardCategory::Animated:
            return ModMenu::isHazardCategoryEnabled(4);
        case HazardCategory::Other:
            return ModMenu::isHazardCategoryEnabled(5);
        case HazardCategory::None:
            return false;
    }

    return false;
}

bool isSolidObject(GameObject* object) {
    if (!object)
        return false;

    switch (object->m_objectType) {
        case GameObjectType::Solid:
        case GameObjectType::Breakable:
        case GameObjectType::CollisionObject:
            return true;

        default:
            return false;
    }
}

bool isSlopeObject(GameObject* object) {
    return object && object->m_objectType == GameObjectType::Slope;
}

bool shouldIgnoreCollision(GameObject* object) {
    if (!ModMenu::isNoclipEnabled() || !object)
        return false;

    auto category = classifyHazard(object);

    if (category != HazardCategory::None) {
        return ModMenu::isPhaseThroughHazardsEnabled() &&
               isHazardCategoryEnabled(category);
    }

    if (isSlopeObject(object))
        return ModMenu::isPhaseThroughSlopesEnabled();

    if (isSolidObject(object)) {
        return ModMenu::isPhaseThroughBlocksEnabled() &&
               ModMenu::isNoBlockTouchMode();
    }

    return false;
}

bool shouldPhaseDeath(GameObject* object) {
    if (!ModMenu::isNoclipEnabled())
        return false;

    // Null death objects cover falling / out-of-bounds and other
    // non-object deaths, which are controlled by the pit setting.
    if (!object) {
        return ModMenu::isPhaseThroughHazardsEnabled() &&
               ModMenu::isHazardCategoryEnabled(3);
    }

    auto category = classifyHazard(object);

    if (category != HazardCategory::None) {
        return ModMenu::isPhaseThroughHazardsEnabled() &&
               isHazardCategoryEnabled(category);
    }

    // Safe Block Touch keeps normal solid collision but prevents a solid
    // object from being passed to the actual death routine.
    if (isSolidObject(object)) {
        return ModMenu::isPhaseThroughBlocksEnabled() &&
               !ModMenu::isNoBlockTouchMode();
    }

    return false;
}

} // namespace

bool ModMenu::isNoclipEnabled() {
    return Mod::get()->getSettingValue<bool>("noclip-enabled");
}

void ModMenu::setNoclipEnabled(bool enabled) {
    Mod::get()->setSettingValue<bool>("noclip-enabled", enabled);

    log::info(
        "Noclip {}",
        enabled ? "enabled" : "disabled"
    );
}

bool ModMenu::isPhaseThroughBlocksEnabled() {
    return Mod::get()->getSettingValue<bool>("noclip-phase-blocks");
}

bool ModMenu::isNoBlockTouchMode() {
    return Mod::get()->getSettingValue<std::string>("noclip-block-mode") == "no-touch";
}

bool ModMenu::isPhaseThroughSlopesEnabled() {
    return Mod::get()->getSettingValue<bool>("noclip-phase-slopes");
}

bool ModMenu::isPhaseThroughHazardsEnabled() {
    return Mod::get()->getSettingValue<bool>("noclip-phase-hazards");
}

bool ModMenu::isHazardCategoryEnabled(int category) {
    static constexpr const char* keys[] = {
        "noclip-hazard-spikes",
        "noclip-hazard-ground-spikes",
        "noclip-hazard-saws",
        "noclip-hazard-pits",
        "noclip-hazard-animated",
        "noclip-hazard-other",
    };

    if (category < 0 || category >= 6)
        return false;

    return Mod::get()->getSettingValue<bool>(keys[category]);
}

class $modify(ModUniversalPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (shouldPhaseDeath(object)) {
            log::debug(
                "Noclip blocked death from object {}",
                object ? object->m_objectID : -1
            );
            return;
        }

        PlayLayer::destroyPlayer(player, object);
    }
};

class $modify(ModUniversalPlayerObject, PlayerObject) {
    // Public collision entry point.
    bool collidedWithObject(
        float dt,
        GameObject* object,
        CCRect rect,
        bool skipCheck
    ) {
        if (shouldIgnoreCollision(object))
            return false;

        return PlayerObject::collidedWithObject(
            dt,
            object,
            rect,
            skipCheck
        );
    }

    // Lower-level collision path. Hooking only collidedWithObject()
    // leaves this path able to process collisions independently.
    bool collidedWithObjectInternal(
        float dt,
        GameObject* object,
        CCRect rect,
        bool skipCheck
    ) {
        if (shouldIgnoreCollision(object))
            return false;

        return PlayerObject::collidedWithObjectInternal(
            dt,
            object,
            rect,
            skipCheck
        );
    }

    void collidedWithSlopeInternal(
        float dt,
        GameObject* object,
        bool forced
    ) {
        if (
            ModMenu::isNoclipEnabled() &&
            ModMenu::isPhaseThroughSlopesEnabled()
        ) {
            return;
        }

        PlayerObject::collidedWithSlopeInternal(dt, object, forced);
    }
};
