#include "../include/ModMenu.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/ScrollLayer.hpp>

#include <algorithm>
#include <array>
#include <cstring>
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

CCMenuItemSpriteExtra* createHackDefaultButton(
    CCMenu* menu,
    std::vector<std::string> settingKeys,
    CCPoint position
) {
    if (!menu || settingKeys.empty())
        return nullptr;

    auto sprite = CCSprite::createWithSpriteFrameName(
        "GJ_undoBtn_001.png"
    );

    if (!sprite)
        return nullptr;

    sprite->setAnchorPoint({0.5f, 0.5f});
    sprite->setScale(0.8f);

    // Use the same kind of node hierarchy as the working modern buttons:
    // a fixed-size image container with the atlas texture centered inside.
    auto hitbox = CCLayerColor::create(
        {255, 255, 255, 0},
        32.f,
        32.f
    );

    if (!hitbox)
        return nullptr;

    hitbox->setAnchorPoint({0.5f, 0.5f});
    hitbox->ignoreAnchorPointForPosition(false);
    hitbox->setPosition({0.f, 0.f});

    sprite->setPosition(hitbox->getContentSize() / 2.f);
    hitbox->addChild(sprite);

    auto const showButton = !areSettingsAtDefault(settingKeys);

    auto button = CCMenuItemExt::createSpriteExtra(
        hitbox,
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
            : (compact ? 72.f : 78.f);

    float const iconSize = 32.f;
    float const gap = compact ? 9.f : 12.f;

    float const controlWidth =
        toggleWidth +
        gap + iconSize +
        (hasNoclipChanges ? gap + iconSize : 0.f);

    // When the card becomes narrow, center the whole control group instead of
    // forcing it beside the description. This prevents overlays at unusual
    // aspect ratios and very small windows.
    bool const stackControls = panelWidth < 360.f;
    float const startX = stackControls
        ? (panelWidth - controlWidth) / 2.f
        : panelWidth - (compact ? 18.f : 22.f) - controlWidth;

    float const undoX = startX + iconSize / 2.f;
    float const gearX =
        undoX + iconSize + gap + iconSize / 2.f;
    float const checkboxX =
        gearX + iconSize / 2.f + gap + toggleWidth / 2.f;

    if (defaultButton) {
        defaultButton->setVisible(hasNoclipChanges);
        defaultButton->setPosition({undoX, s_noclipRowY});
    }

    if (settingsButton) {
        settingsButton->setPosition({
            gearX,
            s_noclipRowY
        });
    }

    if (checkboxButton)
        checkboxButton->setPosition({checkboxX, s_noclipRowY});
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
    float scale = 0.5f,
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
    float scale = 0.34f
) {
    return createMenuLabel(text, scale, {155, 158, 168});
}

CCLayerColor* createModernPanel(
    CCNode* parent,
    CCPoint center,
    CCSize size,
    ccColor3B color = {31, 33, 39},
    GLubyte opacity = 255
) {
    if (!parent || size.width <= 0.f || size.height <= 0.f)
        return nullptr;

    auto panel = CCLayerColor::create(
        {color.r, color.g, color.b, opacity},
        size.width,
        size.height
    );

    if (!panel)
        return nullptr;

    panel->ignoreAnchorPointForPosition(false);
    panel->setAnchorPoint({0.5f, 0.5f});
    panel->setPosition(center);

    // Panels are structural backgrounds. Keep them below menus and labels
    // regardless of creation order.
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

    auto background = CCLayerColor::create(
        accent
            ? ccColor4B{75, 190, 138, 255}
            : ccColor4B{52, 56, 66, 255},
        size.width,
        size.height
    );

    if (!background)
        return nullptr;

    background->ignoreAnchorPointForPosition(false);
    background->setAnchorPoint({0.5f, 0.5f});

    auto label = createMenuLabel(
        text,
        size.height >= 34.f ? 0.46f : 0.40f,
        accent ? ccColor3B{18, 28, 24} : ccColor3B{232, 234, 238}
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

    auto background = CCLayerColor::create(
        enabled
            ? ccColor4B{75, 190, 138, 255}
            : ccColor4B{52, 56, 66, 255},
        size.width,
        size.height
    );

    if (!background)
        return nullptr;

    background->ignoreAnchorPointForPosition(false);
    background->setAnchorPoint({0.5f, 0.5f});

    auto label = createMenuLabel(
        enabled ? "ON" : "OFF",
        0.43f,
        enabled ? ccColor3B{18, 28, 24} : ccColor3B{215, 218, 224}
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

            auto bg = typeinfo_cast<CCLayerColor*>(item->getNormalImage());
            if (!bg)
                return;

            bg->setColor(next
                ? ccColor3B{75, 190, 138}
                : ccColor3B{52, 56, 66}
            );

            auto stateLabel = typeinfo_cast<CCLabelBMFont*>(
                bg->getChildByTag(7001)
            );
            if (stateLabel) {
                stateLabel->setString(next ? "ON" : "OFF");
                stateLabel->setColor(
                    next
                        ? ccColor3B{18, 28, 24}
                        : ccColor3B{215, 218, 224}
                );
            }

            saveNoclipButtonSettings();
            updateNoclipDefaultButton();

            auto parentMenu = item->getParent();
            auto content = parentMenu ? parentMenu->getParent() : nullptr;
            if (content) {
                for (auto* child : CCArrayExt<CCNode*>(content->getChildren())) {
                    if (child->getTag() != 7100)
                        continue;

                    auto pill = child->getChildByTag(7101);
                    auto stateLabelNode = pill
                        ? pill->getChildByTag(7102)
                        : nullptr;

                    auto stateLabel =
                        typeinfo_cast<CCLabelBMFont*>(stateLabelNode);
                    auto statePill =
                        typeinfo_cast<CCLayerColor*>(pill);

                    if (stateLabel) {
                        stateLabel->setString(next ? "ON" : "OFF");
                        stateLabel->setColor(
                            next
                                ? ccColor3B{18, 28, 24}
                                : ccColor3B{215, 218, 224}
                        );
                    }

                    if (statePill) {
                        statePill->setColor(
                            next
                                ? ccColor3B{75, 190, 138}
                                : ccColor3B{52, 56, 66}
                        );
                    }

                    break;
                }
            }
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
    // Three columns are only used when the cards have enough room to remain
    // readable. Narrow layouts fall back to one column so nothing is forced
    // into a cramped grid; vertical overflow is handled by ScrollLayer.
    if (contentWidth >= 760.f)
        return 3;

    if (contentWidth >= 470.f)
        return 2;

    return 1;
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

    float const width = std::max(
        52.f,
        static_cast<float>(std::strlen(text)) * 5.2f + 18.f
    );

    auto pill = CCLayerColor::create(
        active
            ? ccColor4B{75, 190, 138, 255}
            : ccColor4B{52, 56, 66, 255},
        width,
        24.f
    );

    if (!pill)
        return nullptr;

    pill->ignoreAnchorPointForPosition(false);
    pill->setAnchorPoint({0.5f, 0.5f});
    pill->setPosition(center);
    pill->setTag(7101);

    auto label = createMenuLabel(
        text,
        0.29f,
        active
            ? ccColor3B{18, 28, 24}
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
    if (!parent || !titleText || size.width <= 0.f || size.height <= 0.f)
        return nullptr;

    auto card = CCLayerColor::create(
        {38, 41, 49, 255},
        size.width,
        size.height
    );

    if (!card)
        return nullptr;

    card->ignoreAnchorPointForPosition(false);
    card->setAnchorPoint({0.5f, 0.5f});
    card->setPosition(center);
    card->setTag(7100);

    bool const narrow = size.width < 400.f;
    bool const veryNarrow = size.width < 300.f;

    auto accent = CCLayerColor::create(
        active
            ? ccColor4B{75, 190, 138, 255}
            : ccColor4B{63, 67, 78, 255},
        4.f,
        std::max(24.f, size.height - 28.f)
    );

    if (accent) {
        accent->setPosition({10.f, 14.f});
        card->addChild(accent, 1);
    }

    auto section = createMenuLabel(
        sectionLabel,
        narrow ? 0.25f : 0.27f,
        {133, 137, 148}
    );

    if (section) {
        section->setAnchorPoint({0.f, 0.5f});
        section->setPosition({
            24.f,
            size.height - (narrow ? 17.f : 18.f)
        });
        card->addChild(section, 2);
    }

    float const titleScale =
        veryNarrow ? 0.42f :
        narrow ? 0.46f :
        size.width < 520.f ? 0.49f :
        0.53f;

    auto title = createMenuLabel(
        titleText,
        titleScale,
        {235, 238, 243}
    );

    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({
            24.f,
            size.height - (narrow ? 39.f : 42.f)
        });

        title->limitLabelWidth(
            std::max(96.f, size.width - 164.f),
            titleScale,
            0.32f
        );

        card->addChild(title, 2);
    }

    float const descScale =
        veryNarrow ? 0.21f :
        narrow ? 0.23f :
        0.27f;

    auto desc = createMutedMenuLabel(
        description,
        descScale
    );

    if (desc) {
        desc->setAnchorPoint({0.f, 0.5f});

        // Leave the bottom strip clear on narrow cards because the live
        // control group sits there. Wider cards can use the original compact
        // description position.
        float const descY = narrow ? size.height - 72.f : 22.f;
        desc->setPosition({24.f, descY});

        desc->limitLabelWidth(
            std::max(90.f, size.width - 48.f),
            descScale,
            0.14f
        );

        card->addChild(desc, 2);
    }

    if (statusText)
        createStatusPill(
            card,
            {
                size.width - 48.f,
                size.height - (narrow ? 27.f : 29.f)
            },
            statusText,
            active,
            !active
        );

    parent->addChild(card, -5);
    return card;
}


CCSize getResponsivePopupSize(float preferredWidth, float preferredHeight) {
    auto const screen = CCDirector::sharedDirector()->getWinSize();

    // The popup chooses its shape from the actual window aspect ratio first,
    // then clamps itself to the available pixels. This is deliberately
    // independent of any one monitor resolution.
    float const aspect =
        screen.height > 1.f ? screen.width / screen.height : 1.f;

    float targetWidth = preferredWidth;
    float targetHeight = preferredHeight;

    if (aspect < 0.95f) {
        // Tall / portrait windows get more vertical room.
        targetWidth = std::max(targetWidth, 640.f);
        targetHeight = std::max(targetHeight, 720.f);
    }
    else if (aspect < 1.45f) {
        // Near-square windows trade some width for a taller workspace.
        targetWidth = std::max(targetWidth, 760.f);
        targetHeight = std::max(targetHeight, 620.f);
    }
    else {
        // Wide windows get a larger canvas; the content itself still adapts
        // to whatever width remains after the sidebar.
        targetWidth = std::max(targetWidth, 1040.f);
        targetHeight = std::max(targetHeight, 600.f);
    }

    float const availableWidth =
        std::max(220.f, screen.width - 24.f);
    float const availableHeight =
        std::max(180.f, screen.height - 24.f);

    return {
        std::min(targetWidth, availableWidth),
        std::min(targetHeight, availableHeight)
    };
}

bool isCompactMenu(float width) {
    return width < 680.f;
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
            {52, 56, 66, 255},
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
                    ? ccColor3B{75, 190, 138}
                    : ccColor3B{52, 56, 66}
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

    auto const screen = CCDirector::sharedDirector()->getWinSize();
    float const aspect =
        screen.height > 1.f ? screen.width / screen.height : 1.f;

    auto const popupSize = getResponsivePopupSize(
        aspect < 0.95f ? 640.f : aspect < 1.45f ? 760.f : 1040.f,
        aspect < 0.95f ? 720.f : aspect < 1.45f ? 620.f : 600.f
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
        m_closeBtn->setScale(0.70f);
    }

    auto background = createModernPanel(
        m_mainLayer,
        m_size / 2.f,
        m_size,
        {18, 20, 25},
        255
    );

    if (background)
        background->setZOrder(-100);

    createHeader();
    createTabBar();
    createContentPanel();

    auto footerMenu = createModernMenu(m_mainLayer);

    if (footerMenu) {
        bool const compact = isCompactMenu(m_size.width);
        float const footerButtonWidth =
            std::clamp(m_size.width * 0.20f, 126.f, 164.f);
        float const footerX =
            m_size.width - footerButtonWidth / 2.f -
            std::clamp(m_size.width * 0.018f, 12.f, 20.f);

        auto reset = createModernActionButton(
            footerMenu,
            "Set to Default",
            {
                compact && m_size.width < 430.f
                    ? m_size.width / 2.f
                    : footerX,
                20.f
            },
            {footerButtonWidth, 34.f},
            [this]() {
                this->onSetAllToDefault(nullptr);
            }
        );

        if (!reset)
            log::warn("Could not create Set to Default button");
    }

    return true;
}

void ModMenu::createHeader() {
    bool const compact = isCompactMenu(m_size.width);
    float const left = compact ? 18.f : 22.f;

    auto logo = CCSprite::create("PopupTitle.png"_spr);

    float titleX = left;
    if (logo && !compact) {
        logo->ignoreAnchorPointForPosition(false);
        logo->setAnchorPoint({0.5f, 0.5f});
        logo->setPosition({left + 16.f, m_size.height - 28.f});

        float const maxLogoHeight = 26.f;
        float const scale =
            maxLogoHeight / std::max(1.f, logo->getContentSize().height);

        logo->setScale(std::min(1.f, scale));
        m_mainLayer->addChild(logo, 10);
        titleX += 38.f;
    }

    auto title = createMenuLabel(
        "Mod Universal",
        compact ? 0.60f : 0.70f
    );

    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({titleX, m_size.height - 25.f});
        title->limitLabelWidth(
            compact ? m_size.width - 120.f : m_size.width - 190.f,
            compact ? 0.60f : 0.70f,
            0.45f
        );
        m_mainLayer->addChild(title, 10);
    }

    auto subtitle = createMutedMenuLabel(
        "A modular toolkit for Geometry Dash",
        compact ? 0.27f : 0.31f
    );

    if (subtitle) {
        subtitle->setAnchorPoint({0.f, 0.5f});
        subtitle->setPosition({titleX, m_size.height - 47.f});
        subtitle->limitLabelWidth(
            compact ? m_size.width - 120.f : m_size.width - 190.f,
            compact ? 0.27f : 0.31f,
            0.18f
        );
        m_mainLayer->addChild(subtitle, 10);
    }

    auto version = createStatusPill(
        m_mainLayer,
        {
            m_size.width - (m_closeBtn ? 52.f : 18.f),
            m_size.height - 26.f
        },
        "v0.2.0",
        false,
        false
    );

    if (version)
        version->setZOrder(9);

    createModernDivider(
        m_mainLayer,
        {m_size.width / 2.f, m_size.height - 69.f},
        m_size.width - (compact ? 32.f : 44.f)
    );
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

    bool const compact = isCompactMenu(m_size.width);

    if (!compact) {
        float const sidebarWidth =
            std::clamp(m_size.width * 0.18f, 150.f, 190.f);
        float const sidebarLeft = 18.f;
        float const sidebarTop = m_size.height - 88.f;
        float const sidebarBottom = 58.f;
        float const sidebarHeight =
            std::max(170.f, sidebarTop - sidebarBottom);

        auto sidebar = createModernPanel(
            m_mainLayer,
            {
                sidebarLeft + sidebarWidth / 2.f,
                sidebarBottom + sidebarHeight / 2.f
            },
            {sidebarWidth, sidebarHeight},
            {27, 29, 35},
            255
        );

        if (sidebar)
            sidebar->setZOrder(-2);

        auto section = createMenuLabel(
            "MODULES",
            0.30f,
            {126, 131, 143}
        );

        if (section) {
            section->setAnchorPoint({0.f, 0.5f});
            section->setPosition({
                sidebarLeft + 13.f,
                sidebarTop - 19.f
            });
            m_mainLayer->addChild(section, 5);
        }

        float const itemWidth = sidebarWidth - 24.f;
        float const startY = sidebarTop - 51.f;
        float const itemHeight = std::clamp(
            sidebarHeight * 0.115f,
            38.f,
            44.f
        );
        float const itemGap = std::clamp(
            sidebarHeight * 0.025f,
            6.f,
            10.f
        );

        for (int i = 0; i < 5; ++i) {
            float const y = startY - i * (itemHeight + itemGap);

            auto item = CCLayerColor::create(
                i == m_currentTab
                    ? ccColor4B{50, 54, 63, 255}
                    : ccColor4B{43, 46, 54, 255},
                itemWidth,
                itemHeight
            );

            if (!item)
                continue;

            item->ignoreAnchorPointForPosition(false);
            item->setAnchorPoint({0.5f, 0.5f});

            auto label = createMenuLabel(
                tabs[i],
                0.43f,
                {220, 223, 229}
            );

            if (label) {
                label->setAnchorPoint({0.f, 0.5f});
                label->setPosition({
                    28.f,
                    itemHeight / 2.f
                });
                label->setTag(7002);
                label->limitLabelWidth(
                    itemWidth - 42.f,
                    0.43f,
                    0.18f
                );
                item->addChild(label);
            }

            auto accent = CCLayerColor::create(
                {75, 190, 138, 255},
                4.f,
                itemHeight - 14.f
            );

            if (accent) {
                accent->setPosition({
                    6.f,
                    7.f
                });
                accent->setTag(7003);
                accent->setVisible(i == m_currentTab);
                item->addChild(accent, 2);
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
                sidebarLeft + sidebarWidth / 2.f,
                y
            });
            menu->addChild(button);
        }

        return;
    }

    auto const grid = getCompactTabGrid(m_size.width);
    float const pad = std::clamp(m_size.width * 0.035f, 12.f, 18.f);
    float const top = m_size.height - 84.f;
    float const width =
        (m_size.width - pad * 2.f - grid.gap * (grid.columns - 1))
        / static_cast<float>(grid.columns);

    for (int i = 0; i < 5; ++i) {
        auto item = CCLayerColor::create(
            i == m_currentTab
                ? ccColor4B{50, 54, 63, 255}
                : ccColor4B{43, 46, 54, 255},
            std::max(72.f, width),
            grid.height
        );

        if (!item)
            continue;

        item->ignoreAnchorPointForPosition(false);
        item->setAnchorPoint({0.5f, 0.5f});

        float const labelScale =
            grid.columns == 2 ? 0.34f :
            grid.columns == 3 ? 0.38f :
            0.42f;

        auto label = createMenuLabel(
            tabs[i],
            labelScale,
            {220, 223, 229}
        );

        if (label) {
            label->setTag(7002);
            label->setPosition(item->getContentSize() / 2.f);
            label->limitLabelWidth(
                width - 12.f,
                labelScale,
                0.14f
            );
            item->addChild(label);
        }

        auto accent = CCLayerColor::create(
            {75, 190, 138, 255},
            std::max(8.f, width - 10.f),
            3.f
        );

        if (accent) {
            accent->setPosition({
                5.f,
                grid.height - 6.f
            });
            accent->setTag(7003);
            accent->setVisible(i == m_currentTab);
            item->addChild(accent, 2);
        }

        auto button = CCMenuItemSpriteExtra::create(
            item,
            this,
            menu_selector(ModMenu::onTab)
        );

        if (!button)
            continue;

        int const row = i / grid.columns;
        int const col = i % grid.columns;

        button->setTag(i);
        button->setPosition({
            pad + width / 2.f + col * (width + grid.gap),
            top - row * (grid.height + 6.f)
        });
        menu->addChild(button);
    }
}


void ModMenu::createContentPanel() {
    bool const compact = isCompactMenu(m_size.width);

    float const sidebarWidth = compact
        ? 0.f
        : std::clamp(m_size.width * 0.18f, 150.f, 190.f);

    float const navHeight =
        compact ? getCompactTabGrid(m_size.width).navHeight : 0.f;

    float const left = compact
        ? 12.f
        : sidebarWidth + 28.f;

    float const top =
        m_size.height - 82.f - navHeight;

    float const width =
        std::max(150.f, m_size.width - left - 14.f);

    float const height =
        std::max(120.f, top - 12.f);

    m_contentPanel = CCNode::create();
    if (!m_contentPanel)
        return;

    m_contentPanel->setContentSize({width, height});
    m_contentPanel->setPosition({left, 14.f});
    m_mainLayer->addChild(m_contentPanel);

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

        for (auto* child : CCArrayExt<CCNode*>(tabMenu->getChildren())) {
            auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
            if (!button)
                continue;

            auto background =
                typeinfo_cast<CCLayerColor*>(button->getNormalImage());
            if (!background)
                continue;

            bool const selected = button == btn;

            background->setColor(
                selected
                    ? ccColor3B{50, 54, 63}
                    : ccColor3B{43, 46, 54}
            );

            auto label = typeinfo_cast<CCLabelBMFont*>(
                background->getChildByTag(7002)
            );

            if (label) {
                label->setColor(
                    selected
                        ? ccColor3B{238, 241, 245}
                        : ccColor3B{220, 223, 229}
                );
            }

            auto accent = background->getChildByTag(7003);
            if (accent)
                accent->setVisible(selected);
        }
    }

    if (!m_contentPanel)
        return;

    // Rebuild the fixed header + scroll viewport for the newly selected tab.
    m_contentPanel->removeAllChildrenWithCleanup(true);
    m_contentScroll = nullptr;

    float const width = m_contentPanel->getContentSize().width;
    float const height = m_contentPanel->getContentSize().height;
    bool const compact = isCompactMenu(m_size.width);

    auto background = createModernPanel(
        m_contentPanel,
        {width / 2.f, height / 2.f},
        {width, height},
        {24, 26, 32},
        255
    );

    if (!background)
        return;

    auto headerTitle = createMenuLabel(
        (std::array<char const*, 5>{
            "Player",
            "Visuals",
            "Creator",
            "Misc",
            "Settings"
        })[tab],
        compact ? 0.56f : 0.63f
    );

    if (headerTitle) {
        headerTitle->setAnchorPoint({0.f, 0.5f});
        headerTitle->setPosition({20.f, height - 25.f});
        headerTitle->limitLabelWidth(
            width - 40.f,
            compact ? 0.56f : 0.63f,
            0.42f
        );
        m_contentPanel->addChild(headerTitle, 8);
    }

    constexpr char const* descriptions[] = {
        "Gameplay tools, movement helpers, and player controls.",
        "Visual controls and display utilities.",
        "Creator and editor utilities.",
        "Quality-of-life and utility modules.",
        "Interface and Mod Universal configuration."
    };

    auto subtitle = createMutedMenuLabel(
        descriptions[tab],
        compact ? 0.27f : 0.30f
    );

    if (subtitle) {
        subtitle->setAnchorPoint({0.f, 0.5f});
        subtitle->setPosition({20.f, height - 47.f});
        subtitle->limitLabelWidth(
            width - 40.f,
            compact ? 0.27f : 0.30f,
            0.16f
        );
        m_contentPanel->addChild(subtitle, 8);
    }

    createModernDivider(
        m_contentPanel,
        {width / 2.f, height - 67.f},
        width - 40.f
    );

    constexpr char const* sections[] = {
        "GAMEPLAY",
        "VISUALS",
        "CREATOR",
        "MISC",
        "SETTINGS"
    };

    struct FeatureSpec {
        char const* title;
        char const* description;
        char const* section;
        bool active;
        char const* status;
    };

    std::vector<FeatureSpec> features;

    if (tab == 0) {
        features = {
            {
                "Noclip",
                "Pass through selected level geometry and hazards.",
                "GAMEPLAY",
                true,
                Mod::get()->getSettingValue<bool>("noclip-enabled")
                    ? "ON"
                    : "OFF"
            },
            {
                "Speedhack",
                "Adjust the game's global speed.",
                "GAMEPLAY",
                false,
                "SOON"
            },
            {
                "Show Hitboxes",
                "Display player collision boundaries.",
                "DEBUG",
                false,
                "SOON"
            }
        };
    }
    else if (tab == 1) {
        features = {
            {
                "Player Trail",
                "Player trail controls.",
                "PLAYER",
                false,
                "SOON"
            },
            {
                "Object Visibility",
                "Hide or simplify level visuals.",
                "LEVEL",
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
                "CREATOR",
                false,
                "SOON"
            },
            {
                "Editor Utilities",
                "Extra editor workflow helpers.",
                "CREATOR",
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
                "MISC",
                false,
                "SOON"
            },
            {
                "Diagnostics",
                "Useful information and debug helpers.",
                "DEBUG",
                false,
                "SOON"
            }
        };
    }
    else {
        features = {
            {
                "Interface",
                "Menu keybind and interface behavior are controlled by Geode.",
                "SETTINGS",
                true,
                "READY"
            }
        };
    }

    float const viewportPad = std::clamp(width * 0.025f, 10.f, 18.f);
    float const scrollY = 10.f;
    float const scrollHeight =
        std::max(82.f, height - 82.f);

    // Reserve a little breathing room on the right for the scrollbar / edge
    // affordance without making cards depend on one exact popup width.
    float const scrollbarSpace = width >= 600.f ? 12.f : 8.f;
    float const viewportWidth = std::max(
        120.f,
        width - viewportPad * 2.f - scrollbarSpace
    );

    auto scroll = geode::ScrollLayer::create(
        {viewportWidth, scrollHeight},
        true,
        true
    );

    if (!scroll)
        return;

    scroll->setPosition({viewportPad, scrollY});
    scroll->setZOrder(5);
    m_contentPanel->addChild(scroll, 5);
    m_contentScroll = scroll;

    int const columns = getFeatureColumns(viewportWidth);
    float const gap = std::clamp(viewportWidth * 0.018f, 10.f, 14.f);
    float const cardHeight =
        columns == 1 ? 136.f :
        columns == 2 ? 116.f :
        112.f;

    int const rowCount =
        static_cast<int>((features.size() + columns - 1) / columns);

    float const extraBottomSpace = tab == 4 ? 56.f : 0.f;

    float const contentHeight =
        18.f +
        rowCount * cardHeight +
        std::max(0, rowCount - 1) * gap +
        18.f +
        extraBottomSpace;

    scroll->m_contentLayer->setContentSize({
        viewportWidth,
        std::max(scrollHeight, contentHeight)
    });

    // All feature cards and their controls live inside the scroll content
    // layer. The visible viewport never needs to know how many rows exist.
    auto contentMenu = createModernMenu(scroll->m_contentLayer);
    if (!contentMenu)
        return;

    float const featureTop =
        std::max(scrollHeight, contentHeight) - 18.f;

    for (int i = 0; i < static_cast<int>(features.size()); ++i) {
        auto const& feature = features[i];

        auto layout = getFeatureLayout(
            viewportWidth,
            featureTop,
            cardHeight,
            i,
            columns,
            gap
        );

        createFeatureCard(
            scroll->m_contentLayer,
            {layout.x, layout.y},
            {layout.width, layout.height},
            feature.title,
            feature.description,
            feature.section,
            feature.active,
            feature.status
        );

        if (tab == 0 && i == 0) {
            auto toggle = createModernToggle(
                contentMenu,
                "noclip-enabled",
                {
                    layout.x + layout.width / 2.f - 22.f,
                    layout.y - layout.height / 2.f + 19.f
                },
                {74.f, 30.f}
            );

            if (toggle)
                s_noclipCheckboxButton = toggle;

            auto gearHitbox = CCLayerColor::create(
                {255, 255, 255, 0},
                32.f,
                32.f
            );

            if (gearHitbox) {
                gearHitbox->ignoreAnchorPointForPosition(false);
                gearHitbox->setAnchorPoint({0.5f, 0.5f});

                auto gearSprite = CCSprite::createWithSpriteFrameName(
                    "GJ_optionsBtn02_001.png"
                );

                if (gearSprite) {
                    gearSprite->setAnchorPoint({0.5f, 0.5f});
                    gearSprite->setScale(0.70f);
                    gearSprite->setPosition(
                        gearHitbox->getContentSize() / 2.f
                    );
                    gearHitbox->addChild(gearSprite);
                }

                auto gearButton = CCMenuItemExt::createSpriteExtra(
                    gearHitbox,
                    [this](CCMenuItemSpriteExtra*) {
                        this->onNoclipSettings(nullptr);
                    }
                );

                if (gearButton) {
                    gearButton->setPosition({
                        layout.x + layout.width / 2.f - 72.f,
                        layout.y - layout.height / 2.f + 19.f
                    });
                    contentMenu->addChild(gearButton);
                    s_noclipSettingsButton = gearButton;
                }
            }

            auto reset = createHackDefaultButton(
                contentMenu,
                NoclipSettingKeys,
                {
                    layout.x + layout.width / 2.f - 110.f,
                    layout.y - layout.height / 2.f + 19.f
                }
            );

            s_noclipDefaultButton = reset;
            s_noclipRowY =
                layout.y - layout.height / 2.f + 19.f;

            updateNoclipRowLayout();
        }
    }

    if (tab == 4) {
        auto settingsInfo = createMutedMenuLabel(
            "Advanced settings remain available through Geode's settings page.",
            compact ? 0.24f : 0.27f
        );

        if (settingsInfo) {
            settingsInfo->setAnchorPoint({0.f, 0.5f});
            settingsInfo->setPosition({18.f, 30.f});
            settingsInfo->limitLabelWidth(
                viewportWidth - 36.f,
                compact ? 0.24f : 0.27f,
                0.12f
            );
            scroll->m_contentLayer->addChild(settingsInfo, 3);
        }
    }

    // Put the scrollbar on top of the viewport only when there is something
    // to scroll through. ScrollLayer still supports mouse-wheel and touch
    // scrolling on all layouts.
    if (contentHeight > scrollHeight + 1.f) {
        auto bar = geode::Scrollbar::create(scroll);
        if (bar) {
            bar->setAnchorPoint({0.5f, 0.5f});
            bar->setPosition({
                width - viewportPad / 2.f,
                scrollY + scrollHeight / 2.f
            });
            bar->setZOrder(9);
            m_contentPanel->addChild(bar, 9);
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
