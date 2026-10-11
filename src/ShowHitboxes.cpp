#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace {

constexpr ccColor4F TransparentFill = {0.f, 0.f, 0.f, 0.f};
constexpr ccColor4F PlayerBoundsColor = {1.f, 0.20f, 0.22f, 1.f};
constexpr ccColor4F PlayerSolidColor = {0.22f, 0.48f, 1.f, 1.f};

bool isNativeHitboxDebugActive(PlayLayer* layer) {
    if (!layer)
        return false;

    // Keep Geometry Dash's own Practice Mode and show-hitboxes-on-death
    // behavior working after our optional runtime overlay is disabled.
    return (layer->m_isDebugDrawEnabled && layer->m_isPracticeMode) ||
           (layer->m_hitboxesOnDeath && layer->m_playerDied);
}

void drawPlayerAdvancedHitboxes(PlayLayer* layer) {
    if (!layer || !layer->m_debugDrawNode)
        return;

    auto drawPlayer = [layer](PlayerObject* player) {
        if (!player || !player->isVisible())
            return;

        // This is Geometry Dash's axis-aligned player bounds, so it does
        // not rotate with the cube. The smaller rect is GD's solid collision
        // hitbox (the same dimensions used by established hitbox mods).
        auto const bounds = player->getObjectRect(
            player->m_vehicleSize,
            player->m_vehicleSize
        );
        auto const solid = player->getObjectRect(0.3f, 0.3f);

        layer->m_debugDrawNode->drawRect(
            {bounds.getMinX(), bounds.getMinY()},
            {bounds.getMaxX(), bounds.getMaxY()},
            TransparentFill,
            0.35f,
            PlayerBoundsColor
        );

        layer->m_debugDrawNode->drawRect(
            {solid.getMinX(), solid.getMinY()},
            {solid.getMaxX(), solid.getMaxY()},
            TransparentFill,
            0.35f,
            PlayerSolidColor
        );
    };

    drawPlayer(layer->m_player1);
    if (layer->m_player2 && layer->m_player2->isRunning())
        drawPlayer(layer->m_player2);
}

} // namespace

class $modify(ModUniversalShowHitboxesLayer, PlayLayer) {
    void updateVisibility(float dt) {
        PlayLayer::updateVisibility(dt);

        if (!m_debugDrawNode)
            return;

        bool const showModHitboxes =
            Mod::get()->getSettingValue<bool>("show-hitboxes");
        bool const showAdvancedPlayerHitboxes =
            showModHitboxes &&
            Mod::get()->getSettingValue<bool>("show-hitboxes-advanced");

        if (showModHitboxes) {
            // Use Geometry Dash's own hitbox renderer for level objects.
            // This correctly handles circles, slopes and rotated hitboxes.
            m_debugDrawNode->setVisible(true);
            PlayLayer::updateDebugDraw();

            // Add the optional pair of player-only hitboxes after native draw,
            // so updateDebugDraw clearing each frame cannot erase them.
            if (showAdvancedPlayerHitboxes)
                drawPlayerAdvancedHitboxes(this);
            return;
        }

        // Don't hide the game's own debug hitboxes if Practice Mode or the
        // native death-hitbox option has requested them.
        bool const nativeVisible = isNativeHitboxDebugActive(this);
        m_debugDrawNode->setVisible(nativeVisible);

        if (nativeVisible) {
            PlayLayer::updateDebugDraw();
            if (Mod::get()->getSettingValue<bool>("show-hitboxes-advanced"))
                drawPlayerAdvancedHitboxes(this);
        }
        else {
            m_debugDrawNode->clear();
        }
    }
};
