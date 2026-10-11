#include "../../include/ui/Catalog.hpp"
#include "../../include/ui/NoclipPopups.hpp"
#include "../../include/ui/Overlay.hpp"
#include "../../include/ModMenu.hpp"

#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <string_view>

namespace mu::catalog {
namespace {

std::vector<std::string> keysOf(FeatureDef const& feature) {
    std::vector<std::string> keys;

    if (feature.toggleKey)
        keys.emplace_back(feature.toggleKey);

    for (auto const* key : feature.relatedKeys) {
        if (key)
            keys.emplace_back(key);
    }

    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    return keys;
}

} // namespace

std::vector<TabDef> const& tabs() {
    static std::vector<TabDef> const s_tabs = {
        {TabId::Player, "Player", "PLAYER", "Movement and gameplay tools"},
        {TabId::Visuals, "Visuals", "VISUALS", "Visual controls and display utilities"},
        {TabId::Creator, "Creator", "CREATOR", "Creator and editor utilities"},
        {TabId::Misc, "Misc", "MISC", "Quality-of-life and utility modules"},
        {TabId::Settings, "Settings", "SETTINGS", "Interface and Mod Universal configuration"},
    };

    return s_tabs;
}

std::vector<FeatureDef> const& features() {
    static std::vector<FeatureDef> const s_features = {
        {
            .tab = TabId::Player,
            .id = "noclip",
            .title = "Noclip",
            .description = "Move through blocks and hazards.",
            .status = FeatureStatus::Toggle,
            .toggleKey = "noclip-enabled",
            .relatedKeys = {
                "toggle-noclip",
                "noclip-phase-blocks",
                "noclip-block-mode",
                "noclip-phase-slopes",
                "noclip-phase-hazards",
                "noclip-hazard-spikes",
                "noclip-hazard-ground-spikes",
                "noclip-hazard-saws",
                "noclip-hazard-pits",
                "noclip-hazard-animated",
                "noclip-hazard-other",
            },
            .hasSettings = true,
            .settingsEnabled = true,
            .openSettings = []() { mu::ui::openNoclipSettings(); },
        },
        {
            .tab = TabId::Player,
            .id = "speedhack",
            .title = "Speedhack",
            .description = "Change gameplay speed.",
            .status = FeatureStatus::ComingSoon,
        },
        {
            .tab = TabId::Player,
            .id = "show-hitboxes",
            .title = "Show Hitboxes",
            .description = "Display collision outlines during gameplay.",
            .status = FeatureStatus::Toggle,
            .toggleKey = "show-hitboxes",
            .relatedKeys = {"show-hitboxes-advanced", "toggle-hitboxes"},
            .hasSettings = true,
            .settingsEnabled = true,
            .openSettings = []() { mu::ui::openHitboxSettings(); },
        },
        {
            .tab = TabId::Player,
            .id = "player-trail",
            .title = "Player Trail",
            .description = "Visual player trail controls.",
            .status = FeatureStatus::ComingSoon,
        },
        {
            .tab = TabId::Visuals,
            .id = "object-visibility",
            .title = "Object Visibility",
            .description = "Hide or simplify level visuals.",
            .status = FeatureStatus::ComingSoon,
        },
        {
            .tab = TabId::Creator,
            .id = "practice-tools",
            .title = "Practice Tools",
            .description = "Creator-focused level testing tools.",
            .status = FeatureStatus::ComingSoon,
        },
        {
            .tab = TabId::Creator,
            .id = "editor-utilities",
            .title = "Editor Utilities",
            .description = "Extra editor workflow helpers.",
            .status = FeatureStatus::ComingSoon,
        },
        {
            .tab = TabId::Misc,
            .id = "quick-utilities",
            .title = "Quick Utilities",
            .description = "Everyday Geometry Dash conveniences.",
            .status = FeatureStatus::ComingSoon,
        },
        {
            .tab = TabId::Misc,
            .id = "diagnostics",
            .title = "Diagnostics",
            .description = "Useful information and debug helpers.",
            .status = FeatureStatus::ComingSoon,
        },
        {
            .tab = TabId::Settings,
            .id = "interface",
            .title = "Customize Keybind",
            .description = "Change or add keybinds to individual mods.",
            .status = FeatureStatus::Ready,
            .relatedKeys = {"toggle-menu", "toggle-noclip", "toggle-hitboxes"},
            .hasSettings = true,
            .openSettings = []() {
                openSettingsPopup(Mod::get());
            },
        },
    };

    return s_features;
}

std::vector<FeatureDef const*> featuresFor(TabId tab) {
    std::vector<FeatureDef const*> result;

    for (auto const& feature : features()) {
        if (feature.tab == tab)
            result.push_back(&feature);
    }

    return result;
}

int tabIndex(TabId tab) {
    auto const& list = tabs();

    for (int i = 0; i < static_cast<int>(list.size()); ++i) {
        if (list[i].id == tab)
            return i;
    }

    return 0;
}

TabId tabAt(int index) {
    auto const& list = tabs();
    auto const clamped = std::clamp(
        index,
        0,
        static_cast<int>(list.size()) - 1
    );
    return list[clamped].id;
}

int tabCount() {
    return static_cast<int>(tabs().size());
}

bool isToggleOn(FeatureDef const& feature) {
    if (!feature.toggleKey)
        return false;

    return Mod::get()->getSettingValue<bool>(feature.toggleKey);
}

bool isDirty(FeatureDef const& feature) {
    for (auto const& key : keysOf(feature)) {
        if (auto setting = Mod::get()->getSetting(key);
            setting && !setting->isDefaultValue()) {
            return true;
        }
    }

    return false;
}

char const* statusLabel(FeatureDef const& feature) {
    switch (feature.status) {
        case FeatureStatus::ComingSoon:
            return "SOON";
        case FeatureStatus::Ready:
            return "READY";
        case FeatureStatus::Toggle:
            return isToggleOn(feature) ? "ON" : "OFF";
    }

    return "SOON";
}

bool isStatusActive(FeatureDef const& feature) {
    if (feature.status == FeatureStatus::Toggle)
        return isToggleOn(feature);

    return feature.status == FeatureStatus::Ready;
}

void applyToggle(FeatureDef const& feature, bool enabled) {
    if (!feature.toggleKey)
        return;

    if (std::string_view(feature.toggleKey) == "noclip-enabled")
        ModMenu::setNoclipEnabled(enabled);
    else
        Mod::get()->setSettingValue<bool>(feature.toggleKey, enabled);

    mu::overlay::saveModData();
}

void resetFeature(FeatureDef const& feature) {
    for (auto const& key : keysOf(feature)) {
        if (auto setting = Mod::get()->getSetting(key))
            setting->reset();
        else
            log::warn("Could not find setting '{}'", key);
    }

    mu::overlay::saveModData();
}

void resetAllFeatures() {
    for (auto const& key : Mod::get()->getSettingKeys()) {
        if (auto setting = Mod::get()->getSetting(key))
            setting->reset();
    }

    mu::overlay::saveModData();
}

} // namespace mu::catalog
