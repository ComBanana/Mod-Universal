#pragma once

#include <Geode/Geode.hpp>

#include <functional>
#include <string>
#include <vector>

using namespace geode::prelude;

// Register tabs and hacks here. The menu lays them out automatically, so a
// new feature is one FeatureDef — not a new layout pass.
namespace mu::catalog {

enum class TabId {
    Player = 0,
    Visuals,
    Creator,
    Misc,
    Settings,
};

struct TabDef {
    TabId id;
    char const* label;
    char const* heading;
    char const* subtitle;
};

enum class FeatureStatus {
    Toggle,
    ComingSoon,
    Ready,
};

struct FeatureDef {
    TabId tab;
    char const* id;
    char const* title;
    char const* description;
    FeatureStatus status = FeatureStatus::ComingSoon;
    char const* toggleKey = nullptr;
    std::vector<char const*> relatedKeys{};
    bool hasSettings = false;
    bool settingsEnabled = true;
    std::function<void()> openSettings{};
};

std::vector<TabDef> const& tabs();
std::vector<FeatureDef> const& features();
std::vector<FeatureDef const*> featuresFor(TabId tab);
int tabIndex(TabId tab);
TabId tabAt(int index);
int tabCount();

bool isToggleOn(FeatureDef const& feature);
bool isDirty(FeatureDef const& feature);
char const* statusLabel(FeatureDef const& feature);
bool isStatusActive(FeatureDef const& feature);

void applyToggle(FeatureDef const& feature, bool enabled);
void resetFeature(FeatureDef const& feature);
void resetAllFeatures();

} // namespace mu::catalog
