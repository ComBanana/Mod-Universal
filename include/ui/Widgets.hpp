#pragma once

#include "Theme.hpp"

#include <functional>
#include <string>

namespace mu::ui {

CCLabelBMFont* label(
    char const* text,
    float scale,
    ccColor3B color = theme::Text
);

CCLabelBMFont* mutedLabel(char const* text, float scale = 0.37f);

CCLayerColor* roundedRect(
    ccColor3B color,
    CCSize size,
    float radius,
    GLubyte opacity = 255
);

void setRoundedRectColor(
    CCLayerColor* layer,
    ccColor3B color,
    float radius,
    GLubyte opacity = 255
);

CCLayerColor* panel(
    CCNode* parent,
    CCPoint center,
    CCSize size,
    ccColor3B color = theme::Panel,
    float radius = theme::PanelRadius,
    GLubyte opacity = 255
);

CCMenu* menuOn(CCNode* parent, int z = 20);

void disablePressAnim(CCMenuItemSpriteExtra* button);

CCMenuItemSpriteExtra* textButton(
    CCMenu* menu,
    char const* text,
    CCPoint position,
    CCSize size,
    std::function<void()> callback,
    ccColor3B background = theme::Button,
    ccColor3B textColor = theme::Text
);

CCMenuItemSpriteExtra* settingsButton(
    CCMenu* menu,
    CCPoint position,
    CCSize size,
    std::function<void()> callback
);

CCMenuItemSpriteExtra* pillButton(
    CCMenu* menu,
    char const* text,
    CCPoint position,
    CCSize size,
    std::function<void(CCMenuItemSpriteExtra*)> callback,
    bool active
);

void setPillAppearance(
    CCMenuItemSpriteExtra* button,
    char const* text,
    bool active
);

} // namespace mu::ui
