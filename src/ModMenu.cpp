#include "../include/ModMenu.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Scrollbar.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <functional>
#include <vector>

namespace {

std::vector<std::string> const NoclipSettingKeys = {
    "noclip-enabled",
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
};

void ensureSettingsFile() {
    auto const path = Mod::get()->getSaveDir() / "settings.json";
    std::error_code ec;

    if (std::filesystem::exists(path, ec))
        return;

    if (ec) {
        log::error(
            "Failed to check settings file: {}",
            ec.message()
        );
        return;
    }

    if (auto result = Mod::get()->saveData(); !result) {
        log::error(
            "Failed to create default settings file: {}",
            result.unwrapErr()
        );
    }
}

bool areSettingsAtDefault(std::vector<std::string> const& keys) {
    for (auto const& key : keys) {
        if (auto setting = Mod::get()->getSetting(key);
            setting && !setting->isDefaultValue()) {
            return false;
        }
    }

    return true;
}

void resetSettingsToDefault(std::vector<std::string> const& keys) {
    for (auto const& key : keys) {
        if (auto setting = Mod::get()->getSetting(key))
            setting->reset();
        else
            log::warn("Could not find setting '{}'", key);
    }

    if (auto result = Mod::get()->saveData(); !result) {
        log::error(
            "Failed to save reset settings: {}",
            result.unwrapErr()
        );
    }
}

CCLayerColor* createRoundedLayerColor(
    ccColor3B color,
    CCSize size,
    float radius,
    GLubyte opacity = 255
);

void setRoundedLayerColor(
    CCLayerColor* layer,
    ccColor3B color,
    GLubyte opacity = 255
);

CCMenuItemSpriteExtra* createHackDefaultButton(
    CCMenu* menu,
    std::vector<std::string> settingKeys,
    CCPoint position
) {
    if (!menu || settingKeys.empty())
        return nullptr;

    bool const showButton = !areSettingsAtDefault(settingKeys);

    auto background = CCLayerColor::create(
        {31, 36, 33, 255},
        58.f,
        28.f
    );

    if (!background)
        return nullptr;

    background->ignoreAnchorPointForPosition(false);
    background->setAnchorPoint({0.5f, 0.5f});

    auto label = CCLabelBMFont::create("RESET", "chatFont.fnt");

    if (label) {
        label->setScale(0.36f);
        label->setColor({229, 235, 229});
        label->setPosition(background->getContentSize() / 2.f);
        background->addChild(label);
    }

    auto button = CCMenuItemExt::createSpriteExtra(
        background,
        [keys = std::move(settingKeys)](CCMenuItemSpriteExtra*) {
            resetSettingsToDefault(keys);
            ModMenu::refreshCurrentTab();
        }
    );

    if (!button)
        return nullptr;

    button->setPosition(position);
    button->setVisible(showButton);
    menu->addChild(button);
    return button;
}

WeakRef<CCMenuItemSpriteExtra> s_noclipDefaultButton;
WeakRef<CCMenuItemSpriteExtra> s_noclipSettingsButton;
WeakRef<CCMenuItemSpriteExtra> s_noclipCheckboxButton;
float s_noclipRowY = 0.f;

void updateNoclipRowLayout() {
    bool const hasNoclipChanges = !areSettingsAtDefault(NoclipSettingKeys);

    auto defaultButton = s_noclipDefaultButton.lock();
    auto settingsButton = s_noclipSettingsButton.lock();
    auto checkboxButton = s_noclipCheckboxButton.lock();

    CCNode* anchorNode =
        defaultButton ? static_cast<CCNode*>(defaultButton) :
        settingsButton ? static_cast<CCNode*>(settingsButton) :
        checkboxButton ? static_cast<CCNode*>(checkboxButton) :
        nullptr;

    if (!anchorNode)
        return;

    auto menu = anchorNode->getParent();
    auto panel = menu ? menu->getParent() : nullptr;
    if (!panel)
        return;

    float const panelWidth = panel->getContentSize().width;
    bool const compact = panelWidth < 560.f;

    float const toggleWidth =
        checkboxButton
            ? checkboxButton->getContentSize().width
            : (compact ? 72.f : 74.f);

    float const settingsWidth =
        settingsButton
            ? settingsButton->getContentSize().width
            : 74.f;

    float const resetWidth =
        defaultButton
            ? defaultButton->getContentSize().width
            : 58.f;

    float const gap = compact ? 8.f : 10.f;

    float const controlWidth =
        toggleWidth +
        gap + settingsWidth +
        (hasNoclipChanges ? gap + resetWidth : 0.f);

    bool const stackControls = panelWidth < 360.f;

    float const startX = stackControls
        ? (panelWidth - controlWidth) / 2.f
        : panelWidth - (compact ? 18.f : 22.f) - controlWidth;

    float currentX = startX;

    if (defaultButton) {
        defaultButton->setVisible(hasNoclipChanges);
        defaultButton->setPosition({
            currentX + resetWidth / 2.f,
            s_noclipRowY
        });
    }

    if (hasNoclipChanges)
        currentX += resetWidth + gap;

    if (settingsButton) {
        settingsButton->setPosition({
            currentX + settingsWidth / 2.f,
            s_noclipRowY
        });
    }

    currentX += settingsWidth + gap;

    if (checkboxButton)
        checkboxButton->setPosition({
            currentX + toggleWidth / 2.f,
            s_noclipRowY
        });
}

void updateNoclipDefaultButton() {
    updateNoclipRowLayout();
}

void saveNoclipButtonSettings() {
    if (auto result = Mod::get()->saveData(); !result) {
        log::error(
            "Failed to save Noclip button settings: {}",
            result.unwrapErr()
        );
    }
}

CCLabelBMFont* createMenuLabel(
    char const* text,
    float scale = 0.54f,
    ccColor3B color = {235, 235, 240}
) {
    auto label = CCLabelBMFont::create(text, "chatFont.fnt");
    if (!label)
        return nullptr;

    label->setScale(scale);
    label->setColor(color);
    return label;
}

CCLabelBMFont* createMutedMenuLabel(
    char const* text,
    float scale = 0.37f
) {
    return createMenuLabel(text, scale, {155, 158, 168});
}

CCLayerColor* createRoundedLayerColor(
    ccColor3B color,
    CCSize size,
    float radius,
    GLubyte opacity
) {
    if (size.width <= 0.f || size.height <= 0.f)
        return nullptr;

    auto layer = CCLayerColor::create(
        {0, 0, 0, 0},
        size.width,
        size.height
    );

    if (!layer)
        return nullptr;

    layer->ignoreAnchorPointForPosition(false);
    layer->setAnchorPoint({0.5f, 0.5f});

    auto draw = CCDrawNode::create();
    if (!draw)
        return layer;

    draw->setTag(9001);

    float const radiusClamped = std::clamp(
        radius,
        0.f,
        std::min(size.width, size.height) / 2.f
    );

    ccColor4F const fill = {
        color.r / 255.f,
        color.g / 255.f,
        color.b / 255.f,
        opacity / 255.f
    };

    if (size.width > radiusClamped * 2.f) {
        draw->drawRect(
            {radiusClamped, 0.f},
            {size.width - radiusClamped, size.height},
            fill,
            0.f,
            {0.f, 0.f, 0.f, 0.f}
        );
    }

    if (size.height > radiusClamped * 2.f) {
        draw->drawRect(
            {0.f, radiusClamped},
            {size.width, size.height - radiusClamped},
            fill,
            0.f,
            {0.f, 0.f, 0.f, 0.f}
        );
    }

    draw->drawCircle(
        {radiusClamped, radiusClamped},
        radiusClamped,
        fill,
        0.f,
        {0.f, 0.f, 0.f, 0.f},
        32
    );
    draw->drawCircle(
        {size.width - radiusClamped, radiusClamped},
        radiusClamped,
        fill,
        0.f,
        {0.f, 0.f, 0.f, 0.f},
        32
    );
    draw->drawCircle(
        {size.width - radiusClamped, size.height - radiusClamped},
        radiusClamped,
        fill,
        0.f,
        {0.f, 0.f, 0.f, 0.f},
        32
    );
    draw->drawCircle(
        {radiusClamped, size.height - radiusClamped},
        radiusClamped,
        fill,
        0.f,
        {0.f, 0.f, 0.f, 0.f},
        32
    );

    layer->addChild(draw, 0);
    return layer;
}

void setRoundedLayerColor(
    CCLayerColor* layer,
    ccColor3B color,
    GLubyte opacity
) {
    if (!layer)
        return;

    auto draw = typeinfo_cast<CCDrawNode*>(
        layer->getChildByTag(9001)
    );

    if (!draw)
        return;

    CCSize const size = layer->getContentSize();
    float const radius =
        size.height <= 30.f
            ? size.height / 2.f
            : size.height <= 38.f
                ? 7.f
                : size.height <= 50.f
                    ? 8.f
                    : 10.f;

    draw->clear();

    float const radiusClamped = std::clamp(
        radius,
        0.f,
        std::min(size.width, size.height) / 2.f
    );

    ccColor4F const fill = {
        color.r / 255.f,
        color.g / 255.f,
        color.b / 255.f,
        opacity / 255.f
    };

    if (size.width > radiusClamped * 2.f) {
        draw->drawRect(
            {radiusClamped, 0.f},
            {size.width - radiusClamped, size.height},
            fill,
            0.f,
            {0.f, 0.f, 0.f, 0.f}
        );
    }

    if (size.height > radiusClamped * 2.f) {
        draw->drawRect(
            {0.f, radiusClamped},
            {size.width, size.height - radiusClamped},
            fill,
            0.f,
            {0.f, 0.f, 0.f, 0.f}
        );
    }

    draw->drawDot({radiusClamped, radiusClamped}, radiusClamped, fill);
    draw->drawDot(
        {size.width - radiusClamped, radiusClamped},
        radiusClamped,
        fill
    );
    draw->drawDot(
        {size.width - radiusClamped, size.height - radiusClamped},
        radiusClamped,
        fill
    );
    draw->drawDot(
        {radiusClamped, size.height - radiusClamped},
        radiusClamped,
        fill
    );
}

CCLayerColor* createModernPanel(
    CCNode* parent,
    CCPoint center,
    CCSize size,
    ccColor3B color = {19, 22, 20},
    GLubyte opacity = 255
) {
    if (!parent || size.width <= 0.f || size.height <= 0.f)
        return nullptr;

    auto panel = createRoundedLayerColor(
        color,
        size,
        12.f,
        opacity
    );

    if (!panel)
        return nullptr;

    panel->setAnchorPoint({0.5f, 0.5f});
    panel->setPosition(center);
    panel->setZOrder(-10);
    parent->addChild(panel, -10);
    return panel;
}

CCLayerColor* createModernDivider(
    CCNode* parent,
    CCPoint center,
    float width,
    float height = 1.f
) {
    return createModernPanel(
        parent,
        center,
        {std::max(1.f, width), std::max(1.f, height)},
        {68, 72, 82},
        210
    );
}

CCMenuItemSpriteExtra* createModernActionButton(
    CCMenu* menu,
    char const* text,
    CCPoint position,
    CCSize size,
    std::function<void()> callback,
    bool accent = false
) {
    if (!menu || !text)
        return nullptr;

    auto background = createRoundedLayerColor(
        accent
            ? ccColor3B{75, 190, 138}
            : ccColor3B{31, 36, 33},
        size,
        7.f
    );

    if (!background)
        return nullptr;

    auto label = createMenuLabel(
        text,
        size.height >= 34.f ? 0.50f : size.height >= 28.f ? 0.46f : 0.42f,
        accent
            ? ccColor3B{18, 28, 24}
            : ccColor3B{229, 235, 229}
    );

    if (!label)
        return nullptr;

    label->setTag(7001);
    label->setPosition(size / 2.f);
    background->addChild(label);

    auto button = CCMenuItemExt::createSpriteExtra(
        background,
        [callback = std::move(callback)](CCMenuItemSpriteExtra*) {
            if (callback)
                callback();
        }
    );

    if (!button)
        return nullptr;

    button->setPosition(position);
    menu->addChild(button);
    return button;
}

CCMenuItemSpriteExtra* createModernToggle(
    CCMenu* menu,
    char const* settingKey,
    CCPoint position,
    CCSize size = {78.f, 32.f}
) {
    if (!menu || !settingKey)
        return nullptr;

    auto const key = std::string(settingKey);
    bool const enabled = Mod::get()->getSettingValue<bool>(key);

    auto background = createRoundedLayerColor(
        enabled
            ? ccColor3B{75, 190, 138}
            : ccColor3B{31, 36, 33},
        size,
        size.height / 2.f
    );

    if (!background)
        return nullptr;

    auto label = createMenuLabel(
        enabled ? "ON" : "OFF",
        0.43f,
        enabled
            ? ccColor3B{18, 28, 24}
            : ccColor3B{229, 235, 229}
    );

    if (!label)
        return nullptr;

    label->setTag(7001);
    label->setPosition(size / 2.f);
    background->addChild(label);

    auto button = CCMenuItemExt::createSpriteExtra(
        background,
        [key](CCMenuItemSpriteExtra* item) {
            auto* mod = Mod::get();
            bool const next = !mod->getSettingValue<bool>(key);

            if (key == "noclip-enabled")
                ModMenu::setNoclipEnabled(next);
            else
                mod->setSettingValue<bool>(key, next);

            auto bg = typeinfo_cast<CCLayerColor*>(
                item->getNormalImage()
            );
            if (!bg)
                return;

            setRoundedLayerColor(
                bg,
                next
                    ? ccColor3B{75, 190, 138}
                    : ccColor3B{31, 36, 33},
                255
            );

            auto stateLabel = typeinfo_cast<CCLabelBMFont*>(
                bg->getChildByTag(7001)
            );

            if (stateLabel) {
                stateLabel->setString(next ? "ON" : "OFF");
                stateLabel->setColor(
                    next
                        ? ccColor3B{18, 28, 24}
                        : ccColor3B{229, 235, 229}
                );
            }

            saveNoclipButtonSettings();
            updateNoclipDefaultButton();
            ModMenu::refreshCurrentTab();
        }
    );

    if (!button)
        return nullptr;

    button->setPosition(position);
    menu->addChild(button);
    return button;
}

struct FeatureLayout {
    float x = 0.f;
    float y = 0.f;
    float width = 0.f;
    float height = 0.f;
};



int getFeatureColumns(float contentWidth) {
    // The main workspace is intentionally a two-column feature grid. Only
    // genuinely narrow windows fall back to a single column; vertical
    // overflow is handled by ScrollLayer.
    return contentWidth >= 360.f ? 2 : 1;
}

FeatureLayout getFeatureLayout(
    float contentWidth,
    float areaTop,
    float cardHeight,
    int index,
    int columns,
    float gap = 12.f
) {
    float const outerPad = 20.f;
    float const availableWidth =
        std::max(120.f, contentWidth - outerPad * 2.f);
    float const cardWidth =
        (availableWidth - gap * (columns - 1)) / static_cast<float>(columns);

    int const row = index / columns;
    int const col = index % columns;

    return {
        outerPad + cardWidth / 2.f + col * (cardWidth + gap),
        areaTop - cardHeight / 2.f - row * (cardHeight + gap),
        cardWidth,
        cardHeight
    };
}

CCLayerColor* createStatusPill(
    CCNode* parent,
    CCPoint center,
    char const* text,
    bool active,
    bool subdued = false
) {
    if (!parent || !text)
        return nullptr;

    bool const isOn = std::strcmp(text, "ON") == 0;
    float const width = isOn ? 72.f : 62.f;

    auto pill = createRoundedLayerColor(
        active
            ? ccColor3B{38, 89, 59}
            : ccColor3B{46, 46, 46},
        {width, 26.f},
        13.f
    );

    if (!pill)
        return nullptr;

    pill->setPosition(center);

    auto label = createMenuLabel(
        text,
        0.36f,
        active
            ? ccColor3B{229, 235, 229}
            : ccColor3B{
                static_cast<GLubyte>(subdued ? 150 : 215),
                static_cast<GLubyte>(subdued ? 153 : 218),
                static_cast<GLubyte>(subdued ? 163 : 224)
            }
    );

    if (label) {
        label->setTag(7102);
        label->setPosition(pill->getContentSize() / 2.f);
        pill->addChild(label);
    }

    parent->addChild(pill, 4);
    return pill;
}

CCLayerColor* createFeatureCard(
    CCNode* parent,
    CCPoint center,
    CCSize size,
    char const* titleText,
    char const* description,
    char const* sectionLabel,
    bool active,
    char const* statusText
) {
    (void)sectionLabel;

    if (!parent || !titleText || size.width <= 0.f || size.height <= 0.f)
        return nullptr;

    auto card = createRoundedLayerColor(
        {24, 27, 26},
        size,
        10.f
    );

    if (!card)
        return nullptr;

    card->setPosition(center);
    card->setTag(7100);

    float const scale = size.width / 430.f;

    auto title = createMenuLabel(
        titleText,
        0.69f * scale,
        {229, 235, 229}
    );

    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({
            18.f * scale,
            110.5f * scale
        });
        title->limitLabelWidth(
            size.width - 120.f * scale,
            0.69f * scale,
            0.34f * scale
        );
        card->addChild(title, 2);
    }

    auto desc = createMenuLabel(
        description,
        0.37f * scale,
        {229, 235, 229}
    );

    if (desc) {
        desc->setAnchorPoint({0.f, 0.5f});
        desc->setPosition({
            18.f * scale,
            82.5f * scale
        });
        desc->limitLabelWidth(
            290.f * scale,
            0.37f * scale,
            0.19f * scale
        );
        card->addChild(desc, 2);
    }

    if (statusText) {
        auto pill = createStatusPill(
            card,
            {
                size.width -
                    ((std::strcmp(statusText, "ON") == 0 ? 18.f : 28.f) +
                    (std::strcmp(statusText, "ON") == 0 ? 36.f : 31.f)) *
                    scale,
                109.f * scale
            },
            statusText,
            active,
            false
        );

        if (pill)
            pill->setTag(7101);
    }

    parent->addChild(card, -5);
    return card;
}

CCSize getAspectReferenceSize() {
    auto const director = CCDirector::sharedDirector();

    if (director) {
        auto const view = director->getOpenGLView();

        if (view) {
            auto const& frame = view->getFrameSize();

            // getFrameSize() is the real EGL/window frame size. This avoids
            // making the menu's orientation decision from a scaled popup
            // size, which can incorrectly turn a small landscape window into
            // the portrait layout.
            if (frame.width > 1.f && frame.height > 1.f)
                return frame;
        }

        auto const winSize = director->getWinSize();
        if (winSize.width > 1.f && winSize.height > 1.f)
            return winSize;
    }

    return {1.f, 1.f};
}

float getWindowAspect() {
    auto const size = getAspectReferenceSize();
    return size.width / std::max(1.f, size.height);
}

CCSize getResponsivePopupSize(float preferredWidth, float preferredHeight) {
    // Use the real window aspect ratio to decide orientation, while keeping
    // the requested popup's own reference size. Popup dimensions are scaled
    // in Cocos window points so high-DPI physical pixels do not inflate them.
    auto const screen = CCDirector::sharedDirector()->getWinSize();

    float const targetWidth = preferredWidth;
    float const targetHeight = preferredHeight;

    float const availableWidth = std::max(220.f, screen.width - 24.f);
    float const availableHeight = std::max(180.f, screen.height - 24.f);

    float const scale = std::min(
        1.f,
        std::min(
            availableWidth / targetWidth,
            availableHeight / targetHeight
        )
    );

    return {
        targetWidth * scale,
        targetHeight * scale
    };
}

bool isCompactMenu() {
    // Layout mode must follow the real screen orientation, not the popup's
    // scaled width. A small landscape window is still landscape.
    return getWindowAspect() < 0.95f;
}

struct CompactTabGrid {
    int columns = 5;
    int rows = 1;
    float gap = 7.f;
    float height = 36.f;
    float navHeight = 54.f;
};

CompactTabGrid getCompactTabGrid(float width) {
    CompactTabGrid grid;

    if (width < 450.f) {
        grid.columns = width < 340.f ? 2 : 3;
        grid.height = 34.f;
    }

    grid.rows = (5 + grid.columns - 1) / grid.columns;

    // Reserve exactly the space consumed by the rows plus a small breathing
    // room so the content panel can never overlap the tab grid.
    float const tabRowsHeight =
        grid.rows * grid.height +
        std::max(0, grid.rows - 1) * 6.f;

    grid.navHeight = tabRowsHeight + 15.f;
    return grid;
}

CCMenu* createModernMenu(CCNode* parent) {
    if (!parent)
        return nullptr;

    auto menu = CCMenu::create();
    if (!menu)
        return nullptr;

    // CCMenu defaults to the game window's coordinate space. For a menu
    // nested inside one of our panels/popups, make its coordinate space
    // exactly match that parent.
    menu->setContentSize(parent->getContentSize());
    menu->setAnchorPoint({0.f, 0.f});
    menu->ignoreAnchorPointForPosition(false);
    menu->setPosition({0.f, 0.f});
    menu->setZOrder(20);
    parent->addChild(menu, 20);
    return menu;
}


struct NestedPopupEntry {
    WeakRef<Popup> popup;
    std::function<void()> close;
};

std::vector<NestedPopupEntry> s_nestedPopups;

void registerNestedPopup(Popup* popup, std::function<void()> close) {
    if (!popup)
        return;

    s_nestedPopups.erase(
        std::remove_if(
            s_nestedPopups.begin(),
            s_nestedPopups.end(),
            [popup](NestedPopupEntry const& entry) {
                return entry.popup == popup;
            }
        ),
        s_nestedPopups.end()
    );

    s_nestedPopups.push_back({
        popup,
        std::move(close),
    });
}

void unregisterNestedPopup(Popup* popup) {
    s_nestedPopups.erase(
        std::remove_if(
            s_nestedPopups.begin(),
            s_nestedPopups.end(),
            [popup](NestedPopupEntry const& entry) {
                return entry.popup == popup;
            }
        ),
        s_nestedPopups.end()
    );
}

bool closeTopNestedPopup() {
    while (!s_nestedPopups.empty()) {
        auto entry = std::move(s_nestedPopups.back());
        s_nestedPopups.pop_back();

        // The popup may already have been destroyed by its own close path.
        // WeakRef::lock() safely returns null instead of dereferencing freed
        // memory.
        auto popup = entry.popup.lock();

        if (!popup || !popup->getParent())
            continue;

        if (entry.close)
            entry.close();

        return true;
    }

    return false;
}

void closeAllNestedPopups() {
    while (closeTopNestedPopup()) {
    }
}

void saveMenuSettings() {
    if (auto result = Mod::get()->saveData(); !result) {
        log::error("Failed to save ModUniversal settings: {}", result.unwrapErr());
    }
}

class NoclipHazardPopup : public Popup {
protected:
    bool init() {
        auto const screen = CCDirector::sharedDirector()->getWinSize();
        auto const popupSize = getResponsivePopupSize(
            560.f,
            screen.width < screen.height ? 620.f : 430.f
        );

        if (!Popup::init(popupSize.width, popupSize.height))
            return false;

        if (m_bgSprite)
            m_bgSprite->setVisible(false);

        if (m_closeBtn) {
            m_closeBtn->setAnchorPoint({0.5f, 0.5f});
            m_closeBtn->setPosition({
                m_size.width - 20.f,
                m_size.height - 20.f
            });
            m_closeBtn->setScale(0.68f);
        }

        createModernPanel(
            m_mainLayer,
            {m_size.width / 2.f, m_size.height / 2.f},
            {m_size.width, m_size.height},
            {18, 20, 25},
            255
        )->setZOrder(-100);

        auto title = createMenuLabel(
            "Spike Phasing",
            m_size.width < 440.f ? 0.56f : 0.66f
        );
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({20.f, m_size.height - 28.f});
        m_mainLayer->addChild(title);

        auto subtitle = createMutedMenuLabel(
            "Choose which hazard categories are ignored by Noclip.",
            m_size.width < 440.f ? 0.27f : 0.31f
        );
        subtitle->setAnchorPoint({0.f, 0.5f});
        subtitle->setPosition({20.f, m_size.height - 49.f});
        m_mainLayer->addChild(subtitle);

        createModernDivider(
            m_mainLayer,
            {m_size.width / 2.f, m_size.height - 67.f},
            m_size.width - 40.f
        );

        auto menu = createModernMenu(m_mainLayer);

        float const contentTop = m_size.height - 94.f;
        float const rowGap = std::min(
            43.f,
            std::max(
                35.f,
                (contentTop - 80.f) / 5.f
            )
        );

        addToggleRow(menu, "Spikes", "noclip-hazard-spikes", contentTop);
        addToggleRow(menu, "Ground / Edge Spikes", "noclip-hazard-ground-spikes", contentTop - rowGap);
        addToggleRow(menu, "Sawblades", "noclip-hazard-saws", contentTop - rowGap * 2.f);
        addToggleRow(menu, "Pits", "noclip-hazard-pits", contentTop - rowGap * 3.f);
        addToggleRow(menu, "Animated Hazards", "noclip-hazard-animated", contentTop - rowGap * 4.f);
        addToggleRow(menu, "Other Hazards", "noclip-hazard-other", contentTop - rowGap * 5.f);

        auto closeButton = createModernActionButton(
            menu,
            "Back",
            {m_size.width / 2.f, 25.f},
            {110.f, 32.f},
            [this]() {
                this->onClosePopup(nullptr);
            }
        );

        if (!closeButton)
            log::warn("Could not create hazard popup Back button");

        return true;
    }

    void addToggleRow(
        CCMenu* menu,
        char const* labelText,
        char const* settingKey,
        float y
    ) {
        auto label = createMenuLabel(
            labelText,
            m_size.width < 440.f ? 0.40f : 0.45f
        );
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({24.f, y});
        m_mainLayer->addChild(label);

        auto toggle = createModernToggle(
            menu,
            settingKey,
            {m_size.width - 62.f, y},
            {82.f, 32.f}
        );

        if (!toggle)
            log::warn(
                "Could not create hazard toggle for {}",
                settingKey
            );

        createModernDivider(
            m_mainLayer,
            {m_size.width / 2.f, y - 20.f},
            m_size.width - 48.f
        );
    }

    void onClosePopup(CCObject*) {
        unregisterNestedPopup(this);
        this->onClose(nullptr);
    }

    void keyBackClicked() override {
        onClosePopup(nullptr);
    }

public:
    void showAndRegister() {
        this->show();
        registerNestedPopup(
            this,
            [this]() {
                this->onClosePopup(nullptr);
            }
        );
    }

    static NoclipHazardPopup* create() {
        auto ret = new NoclipHazardPopup();

        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};

class NoclipSettingsPopup : public Popup {
protected:
    CCLayerColor* m_blockModeBackground = nullptr;
    CCLabelBMFont* m_blockModeLabel = nullptr;

    bool init() {
        auto const screen = CCDirector::sharedDirector()->getWinSize();
        auto const popupSize = getResponsivePopupSize(
            560.f,
            screen.width < screen.height ? 620.f : 450.f
        );

        if (!Popup::init(popupSize.width, popupSize.height))
            return false;

        if (m_bgSprite)
            m_bgSprite->setVisible(false);

        if (m_closeBtn) {
            m_closeBtn->setAnchorPoint({0.5f, 0.5f});
            m_closeBtn->setPosition({
                m_size.width - 20.f,
                m_size.height - 20.f
            });
            m_closeBtn->setScale(0.68f);
        }

        createModernPanel(
            m_mainLayer,
            {m_size.width / 2.f, m_size.height / 2.f},
            {m_size.width, m_size.height},
            {18, 20, 25},
            255
        )->setZOrder(-100);

        auto title = createMenuLabel(
            "Noclip",
            m_size.width < 440.f ? 0.56f : 0.66f
        );
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({20.f, m_size.height - 28.f});
        m_mainLayer->addChild(title);

        auto subtitle = createMutedMenuLabel(
            "Control how Noclip interacts with the level.",
            m_size.width < 440.f ? 0.27f : 0.31f
        );
        subtitle->setAnchorPoint({0.f, 0.5f});
        subtitle->setPosition({20.f, m_size.height - 49.f});
        m_mainLayer->addChild(subtitle);

        createModernDivider(
            m_mainLayer,
            {m_size.width / 2.f, m_size.height - 67.f},
            m_size.width - 40.f
        );

        auto menu = createModernMenu(m_mainLayer);

        float const topY = m_size.height - 101.f;
        addToggleRow(menu, "Block Phasing", "noclip-phase-blocks", topY);
        addToggleRow(menu, "Slope Phasing", "noclip-phase-slopes", topY - 40.f);
        addToggleRow(menu, "Spike Phasing", "noclip-phase-hazards", topY - 80.f);

        auto collisionLabel = createMenuLabel(
            "Block Collision",
            m_size.width < 440.f ? 0.42f : 0.48f
        );
        collisionLabel->setAnchorPoint({0.f, 0.5f});
        collisionLabel->setPosition({
            24.f,
            topY - 127.f
        });
        m_mainLayer->addChild(collisionLabel);

        m_blockModeBackground = CCLayerColor::create(
            {31, 36, 33, 255},
            132.f,
            34.f
        );

        if (m_blockModeBackground) {
            m_blockModeBackground->ignoreAnchorPointForPosition(false);
            m_blockModeBackground->setAnchorPoint({0.5f, 0.5f});

            m_blockModeLabel = createMenuLabel(
                getBlockModeText(),
                0.40f,
                {232, 234, 238}
            );

            if (m_blockModeLabel) {
                m_blockModeLabel->setPosition({66.f, 17.f});
                m_blockModeBackground->addChild(m_blockModeLabel);
            }

            auto modeButton = CCMenuItemExt::createSpriteExtra(
                m_blockModeBackground,
                [this](CCMenuItemSpriteExtra*) {
                    this->onBlockMode(nullptr);
                }
            );

            if (modeButton) {
                modeButton->setPosition({
                    m_size.width - 104.f,
                    topY - 127.f
                });
                menu->addChild(modeButton);
            }
        }

        createModernDivider(
            m_mainLayer,
            {m_size.width / 2.f, topY - 151.f},
            m_size.width - 48.f
        );

        auto hazardButton = createModernActionButton(
            menu,
            "Configure Hazards",
            {m_size.width / 2.f, topY - 186.f},
            {190.f, 34.f},
            [this]() {
                this->onHazardSettings(nullptr);
            }
        );

        if (hazardButton) {
            auto hazardBackground =
                typeinfo_cast<CCLayerColor*>(
                    hazardButton->getNormalImage()
                );

            if (hazardBackground) {
                hazardBackground->setColor({67, 69, 76});
                hazardBackground->setOpacity(170);

                auto hazardLabel =
                    typeinfo_cast<CCLabelBMFont*>(
                        hazardBackground->getChildByTag(7001)
                    );

                if (hazardLabel)
                    hazardLabel->setColor({128, 131, 140});
            }

            hazardButton->setEnabled(false);
        }

        auto info = createMutedMenuLabel(
            "Hazard categories will be configurable later.",
            0.29f
        );
        info->setAnchorPoint({0.5f, 0.5f});
        info->setPosition({
            m_size.width / 2.f,
            topY - 213.f
        });
        m_mainLayer->addChild(info);

        auto backButton = createModernActionButton(
            menu,
            "Back",
            {m_size.width / 2.f, 24.f},
            {110.f, 32.f},
            [this]() {
                this->onClosePopup(nullptr);
            }
        );

        if (!backButton)
            log::warn("Could not create Noclip popup Back button");

        return true;
    }

    char const* getBlockModeText() const {
        return Mod::get()->getSettingValue<std::string>("noclip-block-mode") == "no-touch"
            ? "No Hitbox"
            : "Standard";
    }

    void addToggleRow(
        CCMenu* menu,
        char const* labelText,
        char const* settingKey,
        float y
    ) {
        auto label = createMenuLabel(
            labelText,
            m_size.width < 440.f ? 0.40f : 0.46f
        );
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({24.f, y});
        m_mainLayer->addChild(label);

        auto toggle = createModernToggle(
            menu,
            settingKey,
            {m_size.width - 62.f, y},
            {82.f, 32.f}
        );

        if (!toggle)
            log::warn("Could not create Noclip toggle for {}", settingKey);

        createModernDivider(
            m_mainLayer,
            {m_size.width / 2.f, y - 20.f},
            m_size.width - 48.f
        );
    }

    void onBlockMode(CCObject*) {
        auto current =
            Mod::get()->getSettingValue<std::string>("noclip-block-mode");
        std::string const next = current == "no-touch"
            ? "safe-touch"
            : "no-touch";

        Mod::get()->setSettingValue<std::string>(
            "noclip-block-mode",
            next
        );

        if (m_blockModeBackground)
            m_blockModeBackground->setColor(
                next == "no-touch"
                    ? ccColor3B{38, 89, 59}
                    : ccColor3B{31, 36, 33}
            );

        if (m_blockModeLabel) {
            m_blockModeLabel->setString(
                next == "no-touch"
                    ? "No Hitbox"
                    : "Standard"
            );
            m_blockModeLabel->setColor(
                next == "no-touch"
                    ? ccColor3B{18, 28, 24}
                    : ccColor3B{232, 234, 238}
            );
        }

        updateNoclipDefaultButton();
    }

    void onHazardSettings(CCObject*) {
        if (auto popup = NoclipHazardPopup::create())
            popup->showAndRegister();
    }

    void onClosePopup(CCObject*) {
        unregisterNestedPopup(this);
        this->onClose(nullptr);
    }

    void keyBackClicked() override {
        onClosePopup(nullptr);
    }

public:
    void showAndRegister() {
        this->show();
        registerNestedPopup(
            this,
            [this]() {
                this->onClosePopup(nullptr);
            }
        );
    }

    static NoclipSettingsPopup* create() {
        auto ret = new NoclipSettingsPopup();

        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};

} // namespace



bool ModMenu::init() {
    ensureSettingsFile();

    float const aspect = getWindowAspect();

    auto const popupSize = getResponsivePopupSize(
        aspect < 0.95f ? 430.f : aspect < 1.45f ? 960.f : 1200.f,
        aspect < 0.95f ? 760.f : aspect < 1.45f ? 700.f : 720.f
    );

    if (!Popup::init(popupSize.width, popupSize.height))
        return false;

    if (m_bgSprite)
        m_bgSprite->setVisible(false);

    if (m_closeBtn)
        m_closeBtn->setVisible(false);

    auto background = createRoundedLayerColor(
        {14, 17, 15},
        m_size,
        18.f
    );

    if (background) {
        background->setAnchorPoint({0.5f, 0.5f});
        background->setPosition(m_size / 2.f);
        background->setZOrder(-100);
        m_mainLayer->addChild(background, -100);
    }

    auto closeMenu = createModernMenu(m_mainLayer);
    if (closeMenu) {
        auto closeBackground = createRoundedLayerColor(
            {31, 36, 33},
            {28.f, 28.f},
            7.f
        );

        if (closeBackground) {
            auto closeLabel = CCLabelBMFont::create(
                "X",
                "chatFont.fnt"
            );

            if (closeLabel) {
                closeLabel->setScale(0.55f);
                closeLabel->setColor({229, 235, 229});
                closeLabel->setPosition(
                    closeBackground->getContentSize() / 2.f
                );
                closeBackground->addChild(closeLabel);
            }

            auto closeButton = CCMenuItemExt::createSpriteExtra(
                closeBackground,
                [this](CCMenuItemSpriteExtra*) {
                    this->onClose(nullptr);
                }
            );

            if (closeButton) {
                closeButton->setPosition({
                    m_size.width - 20.f,
                    m_size.height - 56.f
                });
                closeMenu->addChild(closeButton);
            }
        }
    }

    createHeader();
    createTabBar();
    createContentPanel();

    return true;
}

void ModMenu::createHeader() {
    bool const compact = isCompactMenu();

    if (compact) {
        float const scale = std::min(
            1.f,
            std::min(
                m_size.width / 430.f,
                m_size.height / 760.f
            )
        );

        auto header = createModernPanel(
            m_mainLayer,
            {
                m_size.width / 2.f,
                m_size.height - 49.f * scale
            },
            {
                398.f * scale,
                66.f * scale
            },
            {20, 23, 22},
            255
        );

        if (header)
            header->setZOrder(-20);

        auto title = createMenuLabel(
            "MOD UNIVERSAL",
            0.60f * scale,
            {229, 235, 229}
        );

        if (title) {
            title->setAnchorPoint({0.f, 0.5f});
            title->setPosition({
                30.f * scale,
                m_size.height - 41.f * scale
            });
            title->limitLabelWidth(
                210.f * scale,
                0.60f * scale,
                0.29f * scale
            );
            m_mainLayer->addChild(title, 10);
        }

        return;
    }

    float const scale = std::min(
        1.f,
        m_size.width / 1200.f
    );

    auto header = createModernPanel(
        m_mainLayer,
        {
            m_size.width / 2.f,
            m_size.height - 56.f * scale
        },
        {
            1152.f * scale,
            72.f * scale
        },
        {20, 23, 22},
        255
    );

    if (header)
        header->setZOrder(-20);

    auto title = createMenuLabel(
        "MOD UNIVERSAL",
        0.84f * scale,
        {229, 235, 229}
    );

    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({
            48.f * scale,
            m_size.height - 55.f * scale
        });
        title->limitLabelWidth(
            270.f * scale,
            0.84f * scale,
            0.30f * scale
        );
        m_mainLayer->addChild(title, 10);
    }

    auto version = createMenuLabel(
        "v0.2.0",
        0.46f * scale,
        {229, 235, 229}
    );

    if (version) {
        version->setAnchorPoint({0.f, 0.5f});
        version->setPosition({
            48.f * scale,
            m_size.height - 78.f * scale
        });
        m_mainLayer->addChild(version, 10);
    }

    auto headerMenu = createModernMenu(m_mainLayer);

    if (headerMenu) {
        auto reset = createModernActionButton(
            headerMenu,
            "Set to Default",
            {
                1100.f * scale,
                m_size.height - 56.f * scale
            },
            {
                96.f * scale,
                28.f * scale
            },
            [this]() {
                this->onSetAllToDefault(nullptr);
            }
        );

        if (!reset)
            log::warn("Could not create header Set to Default button");
    }
}

void ModMenu::createTabBar() {
    auto menu = createModernMenu(m_mainLayer);
    if (!menu)
        return;

    constexpr char const* tabs[] = {
        "Player",
        "Visuals",
        "Creator",
        "Misc",
        "Settings"
    };

    bool const compact = isCompactMenu();

    if (compact) {
        float const scale = std::min(
            1.f,
            std::min(
                m_size.width / 430.f,
                m_size.height / 760.f
            )
        );

        float const navWidth = 398.f * scale;
        float const navHeight = 62.f * scale;
        float const navX = 16.f * scale;
        float const navBottom = m_size.height - 154.f * scale;

        auto navigation = createModernPanel(
            m_mainLayer,
            {
                navX + navWidth / 2.f,
                navBottom + navHeight / 2.f
            },
            {navWidth, navHeight},
            {19, 22, 20},
            255
        );

        if (navigation)
            navigation->setZOrder(-10);

        float const itemWidth = 70.f * scale;
        float const itemHeight = 34.f * scale;
        float const gap = 7.f * scale;
        float const firstCenterX = 60.f * scale;
        float const centerY = navBottom + navHeight / 2.f;

        for (int i = 0; i < 5; ++i) {
            auto item = createRoundedLayerColor(
                i == m_currentTab
                    ? ccColor3B{31, 64, 46}
                    : ccColor3B{24, 27, 26},
                {itemWidth, itemHeight},
                7.f * scale
            );

            if (!item)
                continue;

            auto label = createMenuLabel(
                tabs[i],
                0.38f * scale,
                {229, 235, 229}
            );

            if (label) {
                label->setTag(7002);
                label->setPosition(
                    item->getContentSize() / 2.f
                );
                label->limitLabelWidth(
                    itemWidth - 10.f * scale,
                    0.38f * scale,
                    0.14f * scale
                );
                item->addChild(label);
            }

            auto button = CCMenuItemSpriteExtra::create(
                item,
                this,
                menu_selector(ModMenu::onTab)
            );

            if (!button)
                continue;

            button->setTag(i);
            button->setPosition({
                firstCenterX +
                    i * (itemWidth + gap),
                centerY
            });
            menu->addChild(button);
        }

        return;
    }

    float const scale = std::min(
        1.f,
        m_size.width / 1200.f
    );

    float const sidebarLeft = 24.f * scale;
    float const sidebarWidth = 188.f * scale;
    float const sidebarHeight = 570.f * scale;
    float const sidebarBottom = 40.f * scale;

    auto sidebar = createModernPanel(
        m_mainLayer,
        {
            sidebarLeft + sidebarWidth / 2.f,
            sidebarBottom + sidebarHeight / 2.f
        },
        {sidebarWidth, sidebarHeight},
        {19, 22, 20},
        255
    );

    if (sidebar)
        sidebar->setZOrder(-10);

    auto modules = createMenuLabel(
        "MODULES",
        0.48f * scale,
        {229, 235, 229}
    );

    if (modules) {
        modules->setAnchorPoint({0.f, 0.5f});
        modules->setPosition({
            45.f * scale,
            m_size.height - 137.5f * scale
        });
        m_mainLayer->addChild(modules, 5);
    }

    float const buttonWidth = 154.f * scale;
    float const buttonHeight = 42.f * scale;
    float const buttonCenterX = 118.f * scale;

    for (int i = 0; i < 5; ++i) {
        float const centerFromTop =
            (179.f + i * 55.f) * scale;

        auto item = createRoundedLayerColor(
            i == m_currentTab
                ? ccColor3B{31, 64, 46}
                : ccColor3B{24, 27, 26},
            {buttonWidth, buttonHeight},
            8.f * scale
        );

        if (!item)
            continue;

        auto label = createMenuLabel(
            tabs[i],
            0.56f * scale,
            {229, 235, 229}
        );

        if (label) {
            label->setTag(7002);
            label->setAnchorPoint({0.f, 0.5f});
            label->setPosition({
                15.f * scale,
                buttonHeight / 2.f
            });
            item->addChild(label);
        }

        auto button = CCMenuItemSpriteExtra::create(
            item,
            this,
            menu_selector(ModMenu::onTab)
        );

        if (!button)
            continue;

        button->setTag(i);
        button->setPosition({
            buttonCenterX,
            m_size.height - centerFromTop
        });
        menu->addChild(button);
    }
}

void ModMenu::createContentPanel() {
    bool const compact = isCompactMenu();

    if (compact) {
        float const scale = std::min(
            1.f,
            std::min(
                m_size.width / 430.f,
                m_size.height / 760.f
            )
        );

        m_contentPanel = CCNode::create();
        if (!m_contentPanel)
            return;

        float const width = 398.f * scale;
        float const height = 574.f * scale;

        m_contentPanel->setContentSize({
            width,
            height
        });

        m_contentPanel->setPosition({
            16.f * scale,
            16.f * scale
        });

        m_mainLayer->addChild(m_contentPanel);
        onTab(nullptr);
        return;
    }

    float const scale = std::min(
        1.f,
        m_size.width / 1200.f
    );

    m_contentPanel = CCNode::create();
    if (!m_contentPanel)
        return;

    float const panelWidth = 924.f * scale;
    float const panelHeight = 570.f * scale;

    m_contentPanel->setContentSize({
        panelWidth,
        panelHeight
    });

    m_contentPanel->setPosition({
        220.f * scale,
        40.f * scale
    });

    m_mainLayer->addChild(m_contentPanel);

    auto panel = createModernPanel(
        m_contentPanel,
        {
            panelWidth / 2.f,
            panelHeight / 2.f
        },
        {panelWidth, panelHeight},
        {19, 22, 20},
        255
    );

    if (panel)
        panel->setZOrder(-10);

    onTab(nullptr);
}

ModMenu* ModMenu::create() {
    auto ret = new ModMenu();

    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

void ModMenu::toggle() {
    if (closeTopNestedPopup())
        return;

    if (auto* playLayer = PlayLayer::get();
        playLayer && !playLayer->m_isPaused) {
        return;
    }

    if (s_instance) {
        s_instance->onClose(nullptr);
        return;
    }

    s_instance = create();

    if (s_instance)
        s_instance->show();
}

void ModMenu::onClose(CCObject* sender) {
    log::info("Popup closed!");

    closeAllNestedPopups();
    saveMenuSettings();
    s_instance = nullptr;
    Popup::onClose(sender);
}


void ModMenu::onTab(CCObject* sender) {
    int tab = m_currentTab;

    if (sender) {
        auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
        tab = std::clamp(btn->getTag(), 0, 4);
        m_currentTab = tab;

        auto tabMenu = static_cast<CCMenu*>(btn->getParent());

        if (tabMenu) {
            for (auto* child : CCArrayExt<CCNode*>(tabMenu->getChildren())) {
                auto button =
                    typeinfo_cast<CCMenuItemSpriteExtra*>(child);
                if (!button)
                    continue;

                auto background =
                    typeinfo_cast<CCLayerColor*>(button->getNormalImage());
                if (!background)
                    continue;

                bool const selected = button == btn;

                setRoundedLayerColor(
                    background,
                    selected
                        ? ccColor3B{31, 64, 46}
                        : ccColor3B{24, 27, 26},
                    255
                );

                auto label = typeinfo_cast<CCLabelBMFont*>(
                    background->getChildByTag(7002)
                );

                if (label) {
                    label->setColor(
                        selected
                            ? ccColor3B{229, 235, 229}
                            : ccColor3B{210, 216, 210}
                    );
                }
            }
        }
    }

    if (!m_contentPanel)
        return;

    m_contentPanel->removeAllChildrenWithCleanup(true);
    m_contentScroll = nullptr;

    float const scale = std::min(
        1.f,
        m_size.width / 1200.f
    );

    bool const compact = isCompactMenu();

    float const width = m_contentPanel->getContentSize().width;
    float const height = m_contentPanel->getContentSize().height;

    auto background = createModernPanel(
        m_contentPanel,
        {
            width / 2.f,
            height / 2.f
        },
        {width, height},
        {19, 22, 20},
        255
    );

    if (background)
        background->setZOrder(-10);

    constexpr char const* tabNames[] = {
        "Player",
        "Visuals",
        "Creator",
        "Misc",
        "Settings"
    };

    constexpr char const* descriptions[] = {
        "Movement and gameplay tools",
        "Visual controls and display utilities",
        "Creator and editor utilities",
        "Quality-of-life and utility modules",
        "Interface and Mod Universal configuration"
    };

    constexpr char const* headingNames[] = {
        "PLAYER",
        "VISUALS",
        "CREATOR",
        "MISC",
        "SETTINGS"
    };

    auto heading = createMenuLabel(
        headingNames[tab],
        0.82f * scale,
        {229, 235, 229}
    );

    if (heading) {
        heading->setAnchorPoint({0.f, 0.5f});
        heading->setPosition({
            compact ? 20.f : 26.f * scale,
            height - (34.f * scale)
        });
        heading->limitLabelWidth(
            width - 52.f * scale,
            0.82f * scale,
            0.42f * scale
        );
        m_contentPanel->addChild(heading, 8);
    }

    auto subtitle = createMenuLabel(
        descriptions[tab],
        0.43f * scale,
        {229, 235, 229}
    );

    if (subtitle) {
        subtitle->setAnchorPoint({0.f, 0.5f});
        subtitle->setPosition({
            compact ? 20.f : 26.f * scale,
            height - (58.f * scale)
        });
        subtitle->limitLabelWidth(
            width - 52.f * scale,
            0.43f * scale,
            0.21f * scale
        );
        m_contentPanel->addChild(subtitle, 8);
    }

    struct FeatureSpec {
        char const* title;
        char const* description;
        bool active;
        char const* status;
    };

    std::vector<FeatureSpec> features;

    if (tab == 0) {
        features = {
            {
                "Noclip",
                "Move through blocks and hazards.",
                Mod::get()->getSettingValue<bool>("noclip-enabled"),
                Mod::get()->getSettingValue<bool>("noclip-enabled")
                    ? "ON"
                    : "OFF"
            },
            {
                "Speedhack",
                "Change gameplay speed.",
                false,
                "SOON"
            },
            {
                "Show Hitboxes",
                "Display collision hitboxes.",
                false,
                "SOON"
            },
            {
                "Player Trail",
                "Visual player trail controls.",
                false,
                "SOON"
            }
        };
    }
    else if (tab == 1) {
        features = {
            {
                "Player Trail",
                "Visual player trail controls.",
                false,
                "SOON"
            },
            {
                "Object Visibility",
                "Hide or simplify level visuals.",
                false,
                "SOON"
            }
        };
    }
    else if (tab == 2) {
        features = {
            {
                "Practice Tools",
                "Creator-focused level testing tools.",
                false,
                "SOON"
            },
            {
                "Editor Utilities",
                "Extra editor workflow helpers.",
                false,
                "SOON"
            }
        };
    }
    else if (tab == 3) {
        features = {
            {
                "Quick Utilities",
                "Everyday Geometry Dash conveniences.",
                false,
                "SOON"
            },
            {
                "Diagnostics",
                "Useful information and debug helpers.",
                false,
                "SOON"
            }
        };
    }
    else {
        features = {
            {
                "Interface",
                "Menu keybind and interface behavior.",
                true,
                "READY"
            }
        };
    }

    float const cardWidth = compact
        ? std::max(0.f, width - 40.f)
        : 430.f * scale;

    float const cardHeight = compact
        ? 138.f * scale
        : 138.f * scale;

    float const gapX = 24.f * scale;
    float const gapY = 18.f * scale;

    int const columns = compact ? 1 : 2;

    float const usableWidth = compact
        ? width - 40.f
        : 884.f * scale;

    float const actualCardWidth = compact
        ? usableWidth
        : (usableWidth - gapX) / 2.f;

    float const firstX = compact
        ? width / 2.f
        : 241.f * scale;

    float const secondX =
        compact
            ? firstX
            : 695.f * scale;

    float const firstY =
        compact
            ? height - 170.f * scale
            : 415.f * scale;

    float const secondRowY =
        compact
            ? firstY - cardHeight - gapY
            : 259.f * scale;

    auto contentMenu = createModernMenu(m_contentPanel);
    if (!contentMenu)
        return;

    for (int i = 0; i < static_cast<int>(features.size()); ++i) {
        auto const& feature = features[i];

        int const row = i / columns;
        int const column = i % columns;

        float const centerX =
            compact
                ? firstX
                : column == 0
                    ? firstX
                    : secondX;

        float const centerY =
            compact
                ? firstY - row * (cardHeight + gapY)
                : row == 0
                    ? firstY
                    : secondRowY;

        auto card = createFeatureCard(
            m_contentPanel,
            {centerX, centerY},
            {actualCardWidth, cardHeight},
            feature.title,
            feature.description,
            nullptr,
            feature.active,
            feature.status
        );

        if (!card)
            continue;

        if (tab == 0 && i == 0) {
            // The status pill is the visible control. It keeps the Figma card
            // visually clean while remaining a real Noclip toggle.
            float const statusWidth = 72.f * scale;
            float const statusHeight = 26.f * scale;

            auto toggleHitbox = CCLayerColor::create(
                {255, 255, 255, 0},
                statusWidth,
                statusHeight
            );

            if (toggleHitbox) {
                toggleHitbox->ignoreAnchorPointForPosition(false);
                toggleHitbox->setAnchorPoint({0.5f, 0.5f});

                auto toggleButton = CCMenuItemExt::createSpriteExtra(
                    toggleHitbox,
                    [](CCMenuItemSpriteExtra*) {
                        bool const next =
                            !Mod::get()->getSettingValue<bool>(
                                "noclip-enabled"
                            );

                        ModMenu::setNoclipEnabled(next);
                        saveNoclipButtonSettings();
                        ModMenu::refreshCurrentTab();
                    }
                );

                if (toggleButton) {
                    toggleButton->setPosition({
                        centerX + actualCardWidth / 2.f -
                            48.f * scale,
                        centerY +
                            cardHeight / 2.f -
                            29.f * scale
                    });
                    contentMenu->addChild(toggleButton, 12);
                }
            }

            // The main text area opens the detailed Noclip settings popup.
            // It is transparent so it does not alter the Figma visual.
            auto settingsHitbox = CCLayerColor::create(
                {255, 255, 255, 0},
                300.f * scale,
                62.f * scale
            );

            if (settingsHitbox) {
                settingsHitbox->ignoreAnchorPointForPosition(false);
                settingsHitbox->setAnchorPoint({0.5f, 0.5f});

                auto settingsButton = CCMenuItemExt::createSpriteExtra(
                    settingsHitbox,
                    [this](CCMenuItemSpriteExtra*) {
                        this->onNoclipSettings(nullptr);
                    }
                );

                if (settingsButton) {
                    settingsButton->setPosition({
                        centerX -
                            (actualCardWidth / 2.f -
                            168.f * scale),
                        centerY +
                            22.f * scale
                    });
                    contentMenu->addChild(settingsButton, 11);
                }
            }
        }
    }
}

void ModMenu::onNoclipSettings(CCObject*) {
    openNoclipSettings();
}

void ModMenu::refreshCurrentTab() {
    if (s_instance && s_instance->m_contentPanel)
        s_instance->onTab(nullptr);
}

void ModMenu::onSetAllToDefault(CCObject*) {
    createQuickPopup(
        "Set to Default",
        "This will change all previously changed <cr>settings</c> "
        "back to their <cy>default values</c>. This cannot be undone.",
        "Cancel",
        "Reset",
        [this](auto, bool btn2) {
            if (!btn2)
                return;

            resetSettingsToDefault(Mod::get()->getSettingKeys());
            this->onTab(nullptr);
        }
    );
}

void ModMenu::openNoclipSettings() {
    if (auto popup = NoclipSettingsPopup::create())
        popup->showAndRegister();
}

void ModMenu::openNoclipHazardSettings() {
    if (auto popup = NoclipHazardPopup::create())
        popup->showAndRegister();
}
