#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace {

bool isNativeHitboxDebugActive(PlayLayer* layer) {
    if (!layer)
        return false;

    // Keep Geometry Dash's own Practice Mode and show-hitboxes-on-death
    // behavior working after our optional runtime overlay is disabled.
    return (layer->m_isDebugDrawEnabled && layer->m_isPracticeMode) ||
           (layer->m_hitboxesOnDeath && layer->m_playerDied);
}

} // namespace

class $modify(ModUniversalShowHitboxesLayer, PlayLayer) {
    void updateVisibility(float dt) {
        PlayLayer::updateVisibility(dt);

        if (!m_debugDrawNode)
            return;

        bool const showModHitboxes =
            Mod::get()->getSettingValue<bool>("show-hitboxes");

        if (showModHitboxes) {
            // Reuse Geometry Dash's native debug draw instead of building
            // approximate rectangles ourselves. This preserves native circles,
            // slopes, rotated collision boxes, and object-specific hitboxes.
            m_debugDrawNode->setVisible(true);
            PlayLayer::updateDebugDraw();
            return;
        }

        // Don't hide the game's own debug hitboxes if Practice Mode or the
        // native death-hitbox option has requested them.
        bool const nativeVisible = isNativeHitboxDebugActive(this);
        m_debugDrawNode->setVisible(nativeVisible);

        if (nativeVisible)
            PlayLayer::updateDebugDraw();
        else
            m_debugDrawNode->clear();
    }
};
