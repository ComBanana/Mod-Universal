#include "../include/ModMenu.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
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
    return contentWidth >= 560.f ? 2 : 1;
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
            : ccColor3B{subdued ? 150 : 215, subdued ? 153 : 218, subdued ? 163 : 224}
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

    auto accent = CCLayerColor::create(
        active
            ? ccColor4B{75, 190, 138, 255}
            : ccColor4B{63, 67, 78, 255},
        4.f,
        std::max(24.f, size.height - 28.f)
    );

    if (accent) {
        accent->setPosition({
            10.f,
            14.f
        });
        card->addChild(accent, 1);
    }

    auto section = createMenuLabel(
        sectionLabel,
        0.27f,
        {133, 137, 148}
    );

    if (section) {
        section->setAnchorPoint({0.f, 0.5f});
        section->setPosition({24.f, size.height - 18.f});
        card->addChild(section, 2);
    }

    auto title = createMenuLabel(
        titleText,
        size.width < 260.f ? 0.47f : 0.53f,
        {235, 238, 243}
    );

    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({24.f, size.height - 42.f});
        title->limitLabelWidth(
            std::max(100.f, size.width - 164.f),
            size.width < 260.f ? 0.47f : 0.53f,
            0.35f
        );
        card->addChild(title, 2);
    }

    auto desc = createMutedMenuLabel(
        description,
        size.width < 260.f ? 0.24f : 0.27f
    );

    if (desc) {
        desc->setAnchorPoint({0.f, 0.5f});
        desc->setPosition({24.f, 22.f});
        desc->limitLabelWidth(
            std::max(90.f, size.width - 48.f),
            size.width < 260.f ? 0.24f : 0.27f,
            0.16f
        );
        card->addChild(desc, 2);
    }

    if (statusText)
        createStatusPill(
            card,
            {size.width - 48.f, size.height - 29.f},
            statusText,
            active,
            !active
        );

    parent->addChild(card, -5);
    return card;
}

CCSize getResponsivePopupSize(float preferredWidth, float preferredHeight) {
    auto const screen = CCDirector::sharedDirector()->getWinSize();

    // Use the actual available window dimensions independently. This keeps
    // portrait windows tall enough for their vertical UI while allowing
    // landscape windows to stay wide. Nothing is positioned from the screen
    // after this point; all menu layout is derived from m_size.
    float const availableWidth =
        std::max(220.f, screen.width - 24.f);
    float const availableHeight =
        std::max(180.f, screen.height - 24.f);

    return {
        std::min(preferredWidth, availableWidth),
        std::min(preferredHeight, availableHeight)
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
    auto const popupSize = getResponsivePopupSize(
        screen.width < screen.height ? 620.f : 920.f,
        screen.width < screen.height ? 640.f : 570.f
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
        float const footerButtonWidth = compact ? 132.f : 146.f;
        float const footerX = m_size.width - footerButtonWidth / 2.f - 18.f;

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
        float const sidebarWidth = 150.f;
        float const sidebarTop = m_size.height - 88.f;
        float const sidebarBottom = 58.f;
        float const sidebarHeight =
            std::max(170.f, sidebarTop - sidebarBottom);

        auto sidebar = createModernPanel(
            m_mainLayer,
            {18.f + sidebarWidth / 2.f, sidebarBottom + sidebarHeight / 2.f},
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
            section->setPosition({31.f, sidebarTop - 19.f});
            m_mainLayer->addChild(section, 5);
        }

        for (int i = 0; i < 5; ++i) {
            float const y = sidebarTop - 51.f - i * 49.f;

            auto item = CCLayerColor::create(
                i == m_currentTab
                    ? ccColor4B{50, 54, 63, 255}
                    : ccColor4B{43, 46, 54, 255},
                sidebarWidth - 24.f,
                40.f
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
                label->setPosition({28.f, 20.f});
                label->setTag(7002);
                item->addChild(label);
            }

            auto accent = CCLayerColor::create(
                {75, 190, 138, 255},
                4.f,
                26.f
            );

            if (accent) {
                accent->setPosition({6.f, 7.f});
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
                18.f + sidebarWidth / 2.f,
                y
            });
            menu->addChild(button);
        }

        return;
    }

    auto const grid = getCompactTabGrid(m_size.width);
    float const pad = 16.f;
    float const top = m_size.height - 84.f;
    float const width =
        (m_size.width - pad * 2.f - grid.gap * (grid.columns - 1))
        / static_cast<float>(grid.columns);

    for (int i = 0; i < 5; ++i) {
        auto item = CCLayerColor::create(
            i == m_currentTab
                ? ccColor4B{50, 54, 63, 255}
                : ccColor4B{43, 46, 54, 255},
            width,
            grid.height
        );

        if (!item)
            continue;

        item->ignoreAnchorPointForPosition(false);
        item->setAnchorPoint({0.5f, 0.5f});

        auto label = createMenuLabel(
            tabs[i],
            grid.columns == 2 ? 0.36f :
            grid.columns == 3 ? 0.39f : 0.43f,
            {220, 223, 229}
        );

        if (label) {
            label->setTag(7002);
            label->setPosition(item->getContentSize() / 2.f);
            label->limitLabelWidth(
                width - 14.f,
                grid.columns == 2 ? 0.36f :
                grid.columns == 3 ? 0.39f : 0.43f,
                0.15f
            );
            item->addChild(label);
        }

        auto accent = CCLayerColor::create(
            {75, 190, 138, 255},
            std::max(8.f, width - 10.f),
            3.f
        );

        if (accent) {
            accent->setPosition({5.f, grid.height - 6.f});
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
    float const navHeight =
        compact ? getCompactTabGrid(m_size.width).navHeight : 0.f;

    float const left = compact ? 14.f : 180.f;
    float const top = m_size.height - 82.f - navHeight;
    float const width = m_size.width - left - 14.f;
    float const height = top - 12.f;

    m_contentPanel = CCNode::create();
    if (!m_contentPanel)
        return;

    m_contentPanel->setContentSize({
        std::max(150.f, width),
        std::max(120.f, height)
    });
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

    m_contentPanel->removeAllChildrenWithCleanup(true);

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
        m_contentPanel->addChild(headerTitle, 3);
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
        m_contentPanel->addChild(subtitle, 3);
    }

    createModernDivider(
        m_contentPanel,
        {width / 2.f, height - 67.f},
        width - 40.f
    );

    auto menu = createModernMenu(m_contentPanel);
    if (!menu)
        return;

    float const featureTop = height - 82.f;
    int const columns = getFeatureColumns(width);
    float const gap = 12.f;

    if (tab == 0) {
        constexpr float cardHeight = 114.f;

        auto noclipLayout = getFeatureLayout(
            width, featureTop, cardHeight, 0, columns, gap
        );
        createFeatureCard(
            m_contentPanel,
            {noclipLayout.x, noclipLayout.y},
            {noclipLayout.width, noclipLayout.height},
            "Noclip",
            "Pass through selected level geometry and hazards.",
            "GAMEPLAY",
            true,
            Mod::get()->getSettingValue<bool>("noclip-enabled") ? "ON" : "OFF"
        );

        auto toggle = createModernToggle(
            menu,
            "noclip-enabled",
            {
                noclipLayout.x + noclipLayout.width / 2.f - 22.f,
                noclipLayout.y - noclipLayout.height / 2.f + 19.f
            },
            {74.f, 30.f}
        );

        if (toggle)
            s_noclipCheckboxButton = toggle;

        auto gearSprite = CCSprite::createWithSpriteFrameName(
            "GJ_optionsBtn02_001.png"
        );

        if (gearSprite) {
            gearSprite->setAnchorPoint({0.5f, 0.5f});
            gearSprite->setScale(0.70f);

            auto gearButton = CCMenuItemSpriteExtra::create(
                gearSprite,
                this,
                menu_selector(ModMenu::onNoclipSettings)
            );

            if (gearButton) {
                gearButton->setPosition({
                    noclipLayout.x + noclipLayout.width / 2.f - 72.f,
                    noclipLayout.y - noclipLayout.height / 2.f + 19.f
                });
                menu->addChild(gearButton);
                s_noclipSettingsButton = gearButton;
            }
        }

        auto reset = createHackDefaultButton(
            menu,
            NoclipSettingKeys,
            {
                noclipLayout.x + noclipLayout.width / 2.f - 110.f,
                noclipLayout.y - noclipLayout.height / 2.f + 19.f
            }
        );

        s_noclipDefaultButton = reset;
        s_noclipRowY = noclipLayout.y - noclipLayout.height / 2.f + 19.f;

        updateNoclipRowLayout();

        for (int i = 1; i < 3; ++i) {
            auto layout = getFeatureLayout(
                width,
                featureTop,
                cardHeight,
                i,
                columns,
                gap
            );

            createFeatureCard(
                m_contentPanel,
                {layout.x, layout.y},
                {layout.width, layout.height},
                i == 1 ? "Speedhack" : "Show Hitboxes",
                i == 1
                    ? "Adjust the game's global speed."
                    : "Display player collision boundaries.",
                i == 1 ? "GAMEPLAY" : "DEBUG",
                false,
                "SOON"
            );
        }

        return;
    }

    if (tab == 1) {
        constexpr float cardHeight = 114.f;

        const std::array<std::array<char const*, 3>, 2> features = {{
            {{"Player Trail", "Player trail controls.", "PLAYER"}},
            {{"Object Visibility", "Hide or simplify level visuals.", "LEVEL"}}
        }};

        for (int i = 0; i < 2; ++i) {
            auto layout = getFeatureLayout(
                width,
                featureTop,
                cardHeight,
                i,
                columns,
                gap
            );

            createFeatureCard(
                m_contentPanel,
                {layout.x, layout.y},
                {layout.width, layout.height},
                features[i][0],
                features[i][1],
                features[i][2],
                false,
                "SOON"
            );
        }

        return;
    }

    if (tab == 2) {
        constexpr float cardHeight = 114.f;

        const std::array<std::array<char const*, 3>, 2> features = {{
            {{"Practice Tools", "Creator-focused level testing tools.", "CREATOR"}},
            {{"Editor Utilities", "Extra editor workflow helpers.", "CREATOR"}}
        }};

        for (int i = 0; i < 2; ++i) {
            auto layout = getFeatureLayout(
                width,
                featureTop,
                cardHeight,
                i,
                columns,
                gap
            );

            createFeatureCard(
                m_contentPanel,
                {layout.x, layout.y},
                {layout.width, layout.height},
                features[i][0],
                features[i][1],
                features[i][2],
                false,
                "SOON"
            );
        }

        return;
    }

    if (tab == 3) {
        constexpr float cardHeight = 114.f;

        const std::array<std::array<char const*, 3>, 2> features = {{
            {{"Quick Utilities", "Everyday Geometry Dash conveniences.", "MISC"}},
            {{"Diagnostics", "Useful information and debug helpers.", "DEBUG"}}
        }};

        for (int i = 0; i < 2; ++i) {
            auto layout = getFeatureLayout(
                width,
                featureTop,
                cardHeight,
                i,
                columns,
                gap
            );

            createFeatureCard(
                m_contentPanel,
                {layout.x, layout.y},
                {layout.width, layout.height},
                features[i][0],
                features[i][1],
                features[i][2],
                false,
                "SOON"
            );
        }

        return;
    }

    // Settings tab.
    constexpr float cardHeight = 114.f;

    auto layout = getFeatureLayout(
        width,
        featureTop,
        cardHeight,
        0,
        columns,
        gap
    );

    createFeatureCard(
        m_contentPanel,
        {layout.x, layout.y},
        {layout.width, layout.height},
        "Interface",
        "Menu keybind and interface behavior are controlled by Geode.",
        "SETTINGS",
        true,
        "READY"
    );

    auto settingsInfo = createMutedMenuLabel(
        "The menu uses a responsive layout automatically. "
        "Advanced settings remain available through Geode's settings page.",
        compact ? 0.25f : 0.28f
    );

    if (settingsInfo) {
        settingsInfo->setAnchorPoint({0.f, 0.5f});
        settingsInfo->setPosition({20.f, 82.f});
        settingsInfo->limitLabelWidth(
            width - 40.f,
            compact ? 0.25f : 0.28f,
            0.16f
        );
        m_contentPanel->addChild(settingsInfo, 3);
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
