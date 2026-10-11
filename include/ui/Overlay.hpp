#pragma once

#include <Geode/Geode.hpp>
#include <functional>

using namespace geode::prelude;

namespace mu::overlay {

void registerPopup(Popup* popup, std::function<void()> close);
void unregisterPopup(Popup* popup);
bool closeTop();
void closeAll();

bool saveModData();
void ensureSettingsFile();

CCSize popupSize(float preferredWidth, float preferredHeight);
bool isCompact();
float windowAspect();
float desktopScale(CCSize size);

} // namespace mu::overlay
