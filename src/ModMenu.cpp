#include "../include/ModMenu.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
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

    float const rightEdge = panelWidth - (compact ? 18.f : 22.f);
    float const toggleHalfWidth =
        checkboxButton
            ? checkboxButton->getContentSize().width / 2.f
            : (compact ? 36.f : 39.f);

    float const iconHalfWidth = 16.f;
    float const gap = compact ? 9.f : 12.f;

    float const checkboxX = rightEdge - toggleHalfWidth;
    float const gearX =
        checkboxX - toggleHalfWidth - gap - iconHalfWidth;
    float const undoX =
        gearX - iconHalfWidth - gap - iconHalfWidth;

    if (defaultButton) {
        defaultButton->setVisible(hasNoclipChanges);
        defaultButton->setPosition({undoX, s_noclipRowY});
    }

    if (settingsButton) {
        settingsButton->setPosition({
            hasNoclipChanges ? gearX : undoX,
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
        }
    );

    if (!button)
        return nullptr;

    button->setPosition(position);
    menu->addChild(button);
    return button;
}

CCSize getResponsivePopupSize(float preferredWidth, float preferredHeight) {
    auto const screen = CCDirector::sharedDirector()->getWinSize();

    float const availableWidth = std::max(220.f, screen.width - 24.f);
    float const availableHeight = std::max(180.f, screen.height - 24.f);

    return {
        std::min(preferredWidth, availableWidth),
        std::min(preferredHeight, availableHeight)
    };
}

bool isCompactMenu(float width) {
    return width < 680.f;
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
    parent->addChild(menu);
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

    // Keep the menu comfortably large on desktop while allowing it to fit
    // smaller windows and portrait layouts.
    float const preferredWidth =
        screen.width < screen.height ? 620.f : 900.f;
    float const preferredHeight =
        screen.width < screen.height ? 640.f : 560.f;

    auto const popupSize = getResponsivePopupSize(
        preferredWidth,
        preferredHeight
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
        {m_size.width / 2.f, m_size.height / 2.f},
        m_size,
        {18, 20, 25},
        255
    );
    if (background)
        background->setZOrder(-100);

    createHeader();
    createTabBar();
    createContentPanel();

    auto resetMenu = createModernMenu(m_mainLayer);
    if (resetMenu) {
        bool const compact = isCompactMenu(m_size.width);
        float const resetWidth = compact ? 128.f : 142.f;
        float const resetX =
            compact && m_size.width < 450.f
                ? m_size.width / 2.f
                : m_size.width - resetWidth / 2.f - 12.f;

        auto resetButton = createModernActionButton(
            resetMenu,
            "Set to Default",
            {resetX, 20.f},
            {resetWidth, 34.f},
            [this]() {
                this->onSetAllToDefault(nullptr);
            }
        );

        if (!resetButton)
            log::warn("Could not create Set to Default button");
    }

    return true;
}

void ModMenu::createHeader() {
    bool const compact = isCompactMenu(m_size.width);
    float const left = compact ? 18.f : 22.f;

    auto title = createMenuLabel(
        "Mod Universal",
        compact ? 0.62f : 0.72f
    );
    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({left, m_size.height - 25.f});

        float const maxWidth = compact
            ? m_size.width - 120.f
            : m_size.width - 170.f;

        title->limitLabelWidth(maxWidth, compact ? 0.62f : 0.72f, 0.45f);
        m_mainLayer->addChild(title, 10);
    }

    auto subtitle = createMutedMenuLabel(
        "Geometry Dash utilities",
        compact ? 0.28f : 0.33f
    );
    if (subtitle) {
        subtitle->setAnchorPoint({0.f, 0.5f});
        subtitle->setPosition({left, m_size.height - 47.f});
        subtitle->limitLabelWidth(
            compact ? m_size.width - 120.f : m_size.width - 170.f,
            compact ? 0.28f : 0.33f,
            0.22f
        );
        m_mainLayer->addChild(subtitle);
    }

    auto versionPanel = CCLayerColor::create(
        {40, 43, 51, 255},
        compact ? 54.f : 62.f,
        24.f
    );

    if (versionPanel) {
        versionPanel->ignoreAnchorPointForPosition(false);
        versionPanel->setAnchorPoint({0.5f, 0.5f});

        float const rightReserve = m_closeBtn
            ? 50.f
            : 18.f;

        versionPanel->setPosition({
            m_size.width - rightReserve - versionPanel->getContentSize().width / 2.f,
            m_size.height - 26.f
        });

        auto version = createMutedMenuLabel(
            "v0.2.0",
            compact ? 0.27f : 0.29f
        );

        if (version) {
            version->setColor({191, 195, 204});
            version->setPosition(
                versionPanel->getContentSize() / 2.f
            );
            versionPanel->addChild(version);
        }

        m_mainLayer->addChild(versionPanel, 9);
    }

    createModernDivider(
        m_mainLayer,
        {m_size.width / 2.f, m_size.height - 69.f},
        m_size.width - (compact ? 32.f : 44.f),
        1.f
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
        float const sidebarX = 18.f + sidebarWidth / 2.f;
        float const sidebarBottom = 58.f;
        float const sidebarTop = m_size.height - 86.f;
        float const sidebarHeight = sidebarTop - sidebarBottom;

        auto sidebar = createModernPanel(
            m_mainLayer,
            {sidebarX, sidebarBottom + sidebarHeight / 2.f},
            {sidebarWidth, sidebarHeight},
            {27, 29, 35},
            255
        );

        if (sidebar)
            sidebar->setZOrder(0);

        auto section = createMenuLabel(
            "MODULES",
            0.31f,
            {126, 131, 143}
        );

        if (section) {
            section->setAnchorPoint({0.f, 0.5f});
            section->setPosition({31.f, sidebarTop - 19.f});
            m_mainLayer->addChild(section, 2);
        }

        for (int i = 0; i < 5; ++i) {
            float const y = sidebarTop - 52.f - i * 49.f;

            auto background = CCLayerColor::create(
                {43, 46, 54, 255},
                sidebarWidth - 24.f,
                40.f
            );
            if (!background)
                continue;

            background->ignoreAnchorPointForPosition(false);
            background->setAnchorPoint({0.5f, 0.5f});

            auto label = createMenuLabel(
                tabs[i],
                0.44f,
                {220, 223, 229}
            );
            if (!label)
                continue;

            label->setAnchorPoint({0.f, 0.5f});
            label->setPosition({24.f, 20.f});
            label->setTag(7002);
            background->addChild(label);

            auto accent = CCLayerColor::create(
                {75, 190, 138, 255},
                4.f,
                26.f
            );
            if (accent) {
                accent->setPosition({4.f, 7.f});
                accent->setTag(7003);
                accent->setVisible(i == m_currentTab);
                background->addChild(accent);
            }

            if (i == m_currentTab)
                background->setColor({50, 54, 63});

            auto button = CCMenuItemSpriteExtra::create(
                background,
                this,
                menu_selector(ModMenu::onTab)
            );
            if (!button)
                continue;

            button->setTag(i);
            button->setPosition({sidebarX, y});
            menu->addChild(button);
        }

        return;
    }

    // Compact layouts use a two-row tab grid only when the window is narrow
    // enough to make five tabs in one row cramped.
    bool const twoRows = m_size.width < 450.f;
    int const columns = twoRows ? 3 : 5;
    float const gap = 7.f;
    float const horizontalPad = 16.f;
    float const top = m_size.height - 85.f;
    float const height = twoRows ? 34.f : 36.f;
    float const width =
        (m_size.width - horizontalPad * 2.f - gap * (columns - 1))
        / static_cast<float>(columns);

    for (int i = 0; i < 5; ++i) {
        auto background = CCLayerColor::create(
            {43, 46, 54, 255},
            width,
            height
        );
        if (!background)
            continue;

        background->ignoreAnchorPointForPosition(false);
        background->setAnchorPoint({0.5f, 0.5f});

        auto label = createMenuLabel(
            tabs[i],
            twoRows ? 0.40f : 0.44f,
            {220, 223, 229}
        );
        if (!label)
            continue;

        label->setTag(7002);
        label->setPosition({
            width / 2.f,
            height / 2.f + (twoRows ? 0.5f : 0.f)
        });
        background->addChild(label);

        auto accent = CCLayerColor::create(
            {75, 190, 138, 255},
            std::max(12.f, width - 8.f),
            3.f
        );
        if (accent) {
            accent->setPosition({4.f, height - 5.f});
            accent->setTag(7003);
            accent->setVisible(i == m_currentTab);
            background->addChild(accent);
        }

        if (i == m_currentTab)
            background->setColor({50, 54, 63});

        auto button = CCMenuItemSpriteExtra::create(
            background,
            this,
            menu_selector(ModMenu::onTab)
        );
        if (!button)
            continue;

        int const row = twoRows ? i / columns : 0;
        int const col = twoRows ? i % columns : i;

        button->setTag(i);
        button->setPosition({
            horizontalPad + width / 2.f + col * (width + gap),
            top - row * (height + 6.f)
        });
        menu->addChild(button);
    }
}

void ModMenu::createContentPanel() {
    bool const compact = isCompactMenu(m_size.width);

    float const navHeight = compact
        ? (m_size.width < 450.f ? 89.f : 54.f)
        : 0.f;

    float const left = compact ? 14.f : 178.f;
    float const top = m_size.height - 82.f - navHeight;
    float const width = m_size.width - left - 14.f;
    float const height = top - 14.f;

    m_contentPanel = CCNode::create();
    if (!m_contentPanel)
        return;

    m_contentPanel->setContentSize({
        std::max(190.f, width),
        std::max(140.f, height)
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
    // F3 acts like a back action: close the deepest open submenu first.
    if (closeTopNestedPopup())
        return;

    // The ModUniversal menu is intentionally unavailable during active,
    // unpaused gameplay, but remains usable from the pause screen.
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
        tab = btn->getTag();
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

    auto menu = createModernMenu(m_contentPanel);
    if (!menu)
        return;

    auto background = createModernPanel(
        m_contentPanel,
        m_contentPanel->getContentSize() / 2.f,
        m_contentPanel->getContentSize(),
        {24, 26, 32},
        255
    );
    if (!background)
        return;

    constexpr char const* tabTitles[] = {
        "Player",
        "Visuals",
        "Creator",
        "Misc",
        "Settings"
    };

    constexpr char const* tabDescriptions[] = {
        "Gameplay tools and player controls.",
        "Visual and rendering tools.",
        "Creator and editor utilities.",
        "Quality-of-life utilities.",
        "Mod Universal configuration."
    };

    bool const compact = isCompactMenu(m_size.width);
    float const width = m_contentPanel->getContentSize().width;
    float const height = m_contentPanel->getContentSize().height;

    auto title = createMenuLabel(
        tabTitles[tab],
        compact ? 0.60f : 0.68f
    );
    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({24.f, height - 28.f});
        title->limitLabelWidth(width - 48.f, compact ? 0.60f : 0.68f, 0.40f);
        m_contentPanel->addChild(title, 2);
    }

    auto subtitle = createMutedMenuLabel(
        tabDescriptions[tab],
        compact ? 0.29f : 0.33f
    );
    if (subtitle) {
        subtitle->setAnchorPoint({0.f, 0.5f});
        subtitle->setPosition({24.f, height - 51.f});
        subtitle->limitLabelWidth(
            width - 48.f,
            compact ? 0.29f : 0.33f,
            0.20f
        );
        m_contentPanel->addChild(subtitle, 2);
    }

    createModernDivider(
        m_contentPanel,
        {width / 2.f, height - 69.f},
        width - 48.f
    );

    if (tab == 0) {
        float const cardWidth = width - 40.f;
        float const cardHeight = std::min(
            166.f,
            std::max(136.f, height - 112.f)
        );
        float const cardY = height - 122.f;

        auto card = createModernPanel(
            m_contentPanel,
            {width / 2.f, cardY},
            {cardWidth, cardHeight},
            {38, 41, 49},
            255
        );

        if (!card)
            return;

        auto section = createMenuLabel(
            "GAMEPLAY",
            0.31f,
            {133, 137, 148}
        );
        if (section) {
            section->setAnchorPoint({0.f, 0.5f});
            section->setPosition({
                26.f,
                cardY + cardHeight / 2.f - 18.f
            });
            m_contentPanel->addChild(section, 2);
        }

        auto noclipLabel = createMenuLabel(
            "Noclip",
            compact ? 0.55f : 0.61f
        );
        if (noclipLabel) {
            noclipLabel->setAnchorPoint({0.f, 0.5f});
            noclipLabel->setPosition({
                26.f,
                cardY + cardHeight / 2.f - 47.f
            });
            m_contentPanel->addChild(noclipLabel, 2);
        }

        auto noclipDescription = createMutedMenuLabel(
            "Pass through level geometry and configured hazards.",
            compact ? 0.27f : 0.30f
        );
        if (noclipDescription) {
            noclipDescription->setAnchorPoint({0.f, 0.5f});
            noclipDescription->setPosition({
                26.f,
                cardY + cardHeight / 2.f - 68.f
            });

            float const controlReserve = compact ? 154.f : 168.f;
            noclipDescription->limitLabelWidth(
                std::max(100.f, cardWidth - 52.f - controlReserve),
                compact ? 0.27f : 0.30f,
                0.20f
            );

            m_contentPanel->addChild(noclipDescription, 2);
        }

        // Keep the controls anchored to the content panel, so their spacing
        // remains stable when the popup changes between desktop and compact
        // layouts.
        s_noclipRowY = cardY - cardHeight / 2.f + 31.f;

        auto gearButton = static_cast<CCMenuItemSpriteExtra*>(nullptr);
        auto gearSprite = CCSprite::createWithSpriteFrameName(
            "GJ_optionsBtn02_001.png"
        );

        if (gearSprite) {
            gearSprite->setAnchorPoint({0.5f, 0.5f});
            gearSprite->setScale(0.72f);

            auto gearHitbox = CCLayerColor::create(
                {255, 255, 255, 0},
                32.f,
                32.f
            );

            if (gearHitbox) {
                gearHitbox->setAnchorPoint({0.5f, 0.5f});
                gearHitbox->ignoreAnchorPointForPosition(false);
                gearHitbox->setPosition({0.f, 0.f});
                gearSprite->setPosition(
                    gearHitbox->getContentSize() / 2.f
                );
                gearHitbox->addChild(gearSprite);

                gearButton = CCMenuItemSpriteExtra::create(
                    gearHitbox,
                    this,
                    menu_selector(ModMenu::onNoclipSettings)
                );
            }
        }

        auto noclipDefaultButton = createHackDefaultButton(
            menu,
            NoclipSettingKeys,
            {0.f, s_noclipRowY}
        );
        s_noclipDefaultButton = noclipDefaultButton;

        if (gearButton) {
            s_noclipSettingsButton = gearButton;
            menu->addChild(gearButton);
        }

        auto noclipToggle = createModernToggle(
            menu,
            "noclip-enabled",
            {0.f, s_noclipRowY},
            compact
                ? CCSize{72.f, 30.f}
                : CCSize{78.f, 32.f}
        );
        s_noclipCheckboxButton = noclipToggle;

        if (noclipToggle)
            noclipToggle->getNormalImage()->setTag(7003);

        updateNoclipRowLayout();
        return;
    }

    // Other tabs intentionally remain informational until their 0.2.0
    // modules are implemented. The important difference is that they now
    // use the same visual language and spacing as the live Player tab.
    float const cardWidth = width - 40.f;
    float const cardHeight = std::min(168.f, std::max(136.f, height - 116.f));
    float const cardY = height / 2.f - 8.f;

    auto card = createModernPanel(
        m_contentPanel,
        {width / 2.f, cardY},
        {cardWidth, cardHeight},
        {38, 41, 49},
        255
    );
    if (!card)
        return;

    auto marker = CCLayerColor::create(
        {75, 190, 138, 255},
        5.f,
        std::min(78.f, cardHeight - 28.f)
    );
    if (marker) {
        marker->setPosition({
            20.f,
            cardY - std::min(78.f, cardHeight - 28.f) / 2.f
        });
        m_contentPanel->addChild(marker, 2);
    }

    auto emptyTitle = createMenuLabel(
        tab == 4 ? "Configuration" : "Coming soon",
        compact ? 0.51f : 0.58f
    );
    if (emptyTitle) {
        emptyTitle->setAnchorPoint({0.f, 0.5f});
        emptyTitle->setPosition({
            40.f,
            cardY + 24.f
        });
        m_contentPanel->addChild(emptyTitle, 2);
    }

    auto emptyDescription = createMutedMenuLabel(
        tab == 4
            ? "Use Geode's settings panel for keybinds and advanced options."
            : "This section is ready for the next Mod Universal modules.",
        compact ? 0.28f : 0.31f
    );
    if (emptyDescription) {
        emptyDescription->setAnchorPoint({0.f, 0.5f});
        emptyDescription->setPosition({
            40.f,
            cardY - 9.f
        });
        emptyDescription->limitLabelWidth(
            cardWidth - 72.f,
            compact ? 0.28f : 0.31f,
            0.19f
        );
        m_contentPanel->addChild(emptyDescription, 2);
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
