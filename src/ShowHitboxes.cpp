#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>

#include <array>
#include <algorithm>
#include <limits>

using namespace geode::prelude;

namespace {

constexpr int HitboxDrawNodeTag = 0x4D554842; // "MUHB"

constexpr ccColor4F SolidHitboxColor = {0.20f, 0.72f, 1.00f, 0.95f};
constexpr ccColor4F PassableHitboxColor = {0.24f, 1.00f, 0.72f, 0.95f};
constexpr ccColor4F SlopeHitboxColor = {0.90f, 0.40f, 1.00f, 0.95f};
constexpr ccColor4F HazardHitboxColor = {1.00f, 0.28f, 0.30f, 0.95f};
constexpr ccColor4F PlayerHitboxColor = {1.00f, 0.90f, 0.24f, 1.00f};
constexpr ccColor4F Transparent = {0.f, 0.f, 0.f, 0.f};

bool getObjectHitboxColor(GameObject* object, ccColor4F& color) {
    if (!object)
        return false;

    // Match the categories Geometry Dash uses for collision-capable objects.
    switch (object->m_objectType) {
        case GameObjectType::Solid:
        case GameObjectType::Breakable:
        case GameObjectType::CollisionObject:
            color = object->m_isPassable
                ? PassableHitboxColor
                : SolidHitboxColor;
            return true;

        case GameObjectType::Slope:
            color = object->m_isPassable
                ? PassableHitboxColor
                : SlopeHitboxColor;
            return true;

        case GameObjectType::Hazard:
        case GameObjectType::AnimatedHazard:
            color = HazardHitboxColor;
            return true;

        default:
            // Some platforms and collision objects expose passability via a
            // flag even when their specific object type differs.
            if (object->m_isPassable) {
                color = PassableHitboxColor;
                return true;
            }
            return false;
    }
}

void drawObjectHitbox(
    PlayLayer* layer,
    CCDrawNode* drawNode,
    GameObject* object,
    ccColor4F const& color,
    CCSize const& viewSize
) {
    if (!layer || !drawNode || !object || !object->isVisible())
        return;

    auto parent = object->getParent();
    if (!parent)
        return;

    // getObjectRect is the game's collision rectangle, not the sprite bounds.
    auto const rect = object->getObjectRect();
    if (rect.size.width <= 0.f || rect.size.height <= 0.f)
        return;

    std::array<CCPoint, 4> corners = {{
        {rect.getMinX(), rect.getMinY()},
        {rect.getMaxX(), rect.getMinY()},
        {rect.getMaxX(), rect.getMaxY()},
        {rect.getMinX(), rect.getMaxY()},
    }};

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    // Objects live under several game-layer batch nodes. Convert each corner
    // through world space so every outline lines up in a single overlay.
    for (auto& corner : corners) {
        corner = layer->convertToNodeSpace(
            parent->convertToWorldSpace(corner)
        );

        minX = std::min(minX, corner.x);
        minY = std::min(minY, corner.y);
        maxX = std::max(maxX, corner.x);
        maxY = std::max(maxY, corner.y);
    }

    // Only submit rectangles near the viewport. The setting is opt-in, and
    // this culling keeps large levels from drawing off-screen geometry.
    if (maxX < 0.f || maxY < 0.f ||
        minX > viewSize.width || minY > viewSize.height) {
        return;
    }

    drawNode->drawPolygon(
        corners.data(),
        static_cast<unsigned int>(corners.size()),
        Transparent,
        1.25f,
        color
    );
}

void updateHitboxes(PlayLayer* layer) {
    if (!layer)
        return;

    auto drawNode = typeinfo_cast<CCDrawNode*>(
        layer->getChildByTag(HitboxDrawNodeTag)
    );

    if (!Mod::get()->getSettingValue<bool>("show-hitboxes")) {
        if (drawNode)
            drawNode->removeFromParentAndCleanup(true);
        return;
    }

    if (!drawNode) {
        drawNode = CCDrawNode::create();
        if (!drawNode)
            return;

        drawNode->setTag(HitboxDrawNodeTag);
        layer->addChild(drawNode, 99999);
    }

    drawNode->clear();

    auto const viewSize = CCDirector::sharedDirector()->getWinSize();

    if (layer->m_objects) {
        for (auto object : CCArrayExt<GameObject, false>(layer->m_objects)) {
            ccColor4F color{};
            if (getObjectHitboxColor(object, color)) {
                drawObjectHitbox(layer, drawNode, object, color, viewSize);
            }
        }
    }

    // Players use their own collision rectangles; keep them visually distinct.
    drawObjectHitbox(layer, drawNode, layer->m_player1, PlayerHitboxColor, viewSize);
    if (layer->m_player2 && layer->m_player2 != layer->m_player1) {
        drawObjectHitbox(layer, drawNode, layer->m_player2, PlayerHitboxColor, viewSize);
    }
}

} // namespace

class $modify(ModUniversalShowHitboxesLayer, PlayLayer) {
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        updateHitboxes(this);
    }
};
