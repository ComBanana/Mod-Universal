#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include "../include/ModMenu.hpp"

using namespace geode::prelude;

namespace {

enum class HazardCategory {
    Spikes = 0,
    GroundSpikes = 1,
    Saws = 2,
    Pits = 3,
    Animated = 4,
    Other = 5,
};

HazardCategory classifyHazard(GameObject* object) {
    if (!object)
        return HazardCategory::Other;

    switch (object->m_objectID) {
        // Standard and small spike families.
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

        // Half, edge, and slope-oriented spike variants.
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

        // Saw and blade families.
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

        // Pit / ground hazard families.
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

        // 2.0 animated hazards.
        case 1701:
        case 1702:
        case 1703:
            return HazardCategory::Animated;

        default:
            return HazardCategory::Other;
    }
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
    }

    return false;
}

bool shouldPhaseDeath(GameObject* object) {
    if (!ModMenu::isNoclipEnabled())
        return false;

    if (!ModMenu::isPhaseThroughHazardsEnabled())
        return false;

    // A null source is typically a non-object death such as falling out.
    // Keep full noclip behavior for those deaths while hazard phasing is enabled.
    if (!object)
        return true;

    return isHazardCategoryEnabled(classifyHazard(object));
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
    bool collidedWithObject(
        float dt,
        GameObject* object,
        CCRect rect,
        bool skipCheck
    ) {
        if (
            ModMenu::isNoclipEnabled() &&
            ModMenu::isPhaseThroughBlocksEnabled() &&
            ModMenu::isNoBlockTouchMode()
        ) {
            return false;
        }

        return PlayerObject::collidedWithObject(
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
