#include "../../include/ui/Widgets.hpp"

#include <Geode/cocos/draw_nodes/CCDrawNode.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace mu::ui {
namespace {

constexpr int DrawTag = 9001;
constexpr int LabelTag = 7001;

std::vector<CCPoint> roundedPolygon(CCSize size, float radius) {
    float const r = std::clamp(
        radius,
        0.f,
        std::min(size.width, size.height) / 2.f
    );

    std::vector<CCPoint> points;
    points.reserve(36);

    auto arc = [&](float cx, float cy, float start, float end) {
        int const segments = 8;
        for (int i = 0; i <= segments; ++i) {
            float const t = start + (end - start) * (i / static_cast<float>(segments));
            points.push_back({
                cx + r * std::cos(t),
                cy + r * std::sin(t)
            });
        }
    };

    constexpr float pi = 3.14159265f;
    arc(size.width - r, r, -pi / 2.f, 0.f);
    arc(size.width - r, size.height - r, 0.f, pi / 2.f);
    arc(r, size.height - r, pi / 2.f, pi);
    arc(r, r, pi, pi * 1.5f);
    return points;
}

void paintRounded(
    CCDrawNode* draw,
    CCSize size,
    float radius,
    ccColor3B color,
    GLubyte opacity
) {
    if (!draw)
        return;

    draw->clear();

    auto const points = roundedPolygon(size, radius);
    if (points.size() < 3)
        return;

    ccColor4F const fill = {
        color.r / 255.f,
        color.g / 255.f,
        color.b / 255.f,
        opacity / 255.f
    };

    draw->drawPolygon(
        const_cast<CCPoint*>(points.data()),
        static_cast<unsigned int>(points.size()),
        fill,
        0.f,
        {0.f, 0.f, 0.f, 0.f}
    );
}

float inferRadius(CCSize size) {
    if (size.height <= 30.f)
        return size.height / 2.f;
    if (size.height <= 38.f)
        return theme::ButtonRadius;
    if (size.height <= 50.f)
        return theme::TabRadius;
    if (size.height <= 80.f)
        return theme::HeaderRadius;
    if (size.height <= 150.f)
        return theme::CardRadius;
    return theme::PanelRadius;
}

} // namespace

CCLabelBMFont* label(char const* text, float scale, ccColor3B color) {
    auto node = CCLabelBMFont::create(text, "chatFont.fnt");
    if (!node)
        return nullptr;

    node->setScale(scale);
    node->setColor(color);
    return node;
}

CCLabelBMFont* mutedLabel(char const* text, float scale) {
    return label(text, scale, theme::Muted);
}

CCLayerColor* roundedRect(
    ccColor3B color,
    CCSize size,
    float radius,
    GLubyte opacity
) {
    if (size.width <= 0.f || size.height <= 0.f)
        return nullptr;

    auto layer = CCLayerColor::create({0, 0, 0, 0}, size.width, size.height);
    if (!layer)
        return nullptr;

    layer->ignoreAnchorPointForPosition(false);
    layer->setAnchorPoint({0.5f, 0.5f});
    layer->setContentSize(size);

    auto draw = CCDrawNode::create();
    if (!draw)
        return layer;

    draw->setTag(DrawTag);
    paintRounded(draw, size, radius, color, opacity);
    layer->addChild(draw, 0);
    return layer;
}

void setRoundedRectColor(
    CCLayerColor* layer,
    ccColor3B color,
    GLubyte opacity
) {
    if (!layer)
        return;

    auto draw = typeinfo_cast<CCDrawNode*>(layer->getChildByTag(DrawTag));
    if (!draw)
        return;

    paintRounded(
        draw,
        layer->getContentSize(),
        inferRadius(layer->getContentSize()),
        color,
        opacity
    );
}

CCLayerColor* panel(
    CCNode* parent,
    CCPoint center,
    CCSize size,
    ccColor3B color,
    float radius,
    GLubyte opacity
) {
    if (!parent)
        return nullptr;

    auto node = roundedRect(color, size, radius, opacity);
    if (!node)
        return nullptr;

    node->setPosition(center);
    parent->addChild(node, -10);
    return node;
}

CCMenu* menuOn(CCNode* parent, int z) {
    if (!parent)
        return nullptr;

    auto menu = CCMenu::create();
    if (!menu)
        return nullptr;

    menu->setContentSize(parent->getContentSize());
    menu->setAnchorPoint({0.f, 0.f});
    menu->ignoreAnchorPointForPosition(false);
    menu->setPosition({0.f, 0.f});
    parent->addChild(menu, z);
    return menu;
}

void disablePressAnim(CCMenuItemSpriteExtra* button) {
    if (!button)
        return;

    button->m_scaleMultiplier = 1.f;
    button->m_baseScale = 1.f;
    button->m_animationEnabled = false;
}

CCMenuItemSpriteExtra* textButton(
    CCMenu* menu,
    char const* text,
    CCPoint position,
    CCSize size,
    std::function<void()> callback,
    ccColor3B background,
    ccColor3B textColor
) {
    if (!menu || !text)
        return nullptr;

    auto bg = roundedRect(background, size, theme::ButtonRadius);
    if (!bg)
        return nullptr;

    auto caption = label(
        text,
        size.height >= 34.f ? 0.50f : size.height >= 26.f ? 0.42f : 0.38f,
        textColor
    );

    if (!caption)
        return nullptr;

    caption->setTag(LabelTag);
    caption->setPosition(size / 2.f);
    caption->limitLabelWidth(size.width - 12.f, caption->getScale(), 0.22f);
    bg->addChild(caption);

    auto button = CCMenuItemExt::createSpriteExtra(
        bg,
        [callback = std::move(callback)](CCMenuItemSpriteExtra*) {
            if (callback)
                callback();
        }
    );

    if (!button)
        return nullptr;

    disablePressAnim(button);
    button->setPosition(position);
    menu->addChild(button);
    return button;
}

void setPillAppearance(
    CCMenuItemSpriteExtra* button,
    char const* text,
    bool active
) {
    if (!button || !text)
        return;

    auto bg = typeinfo_cast<CCLayerColor*>(button->getNormalImage());
    if (!bg)
        return;

    setRoundedRectColor(
        bg,
        active ? theme::StatusOn : theme::StatusIdle
    );

    if (auto caption = typeinfo_cast<CCLabelBMFont*>(bg->getChildByTag(LabelTag))) {
        caption->setString(text);
        caption->setColor(theme::Text);
    }
}

CCMenuItemSpriteExtra* pillButton(
    CCMenu* menu,
    char const* text,
    CCPoint position,
    CCSize size,
    std::function<void(CCMenuItemSpriteExtra*)> callback,
    bool active
) {
    if (!menu || !text)
        return nullptr;

    auto bg = roundedRect(
        active ? theme::StatusOn : theme::StatusIdle,
        size,
        size.height / 2.f
    );

    if (!bg)
        return nullptr;

    auto caption = label(text, 0.46f, theme::Text);
    if (!caption)
        return nullptr;

    caption->setTag(LabelTag);
    caption->setPosition(size / 2.f);
    bg->addChild(caption);

    auto button = CCMenuItemExt::createSpriteExtra(
        bg,
        [callback = std::move(callback)](CCMenuItemSpriteExtra* item) {
            if (callback)
                callback(item);
        }
    );

    if (!button)
        return nullptr;

    disablePressAnim(button);
    button->setPosition(position);
    menu->addChild(button);
    return button;
}

} // namespace mu::ui
