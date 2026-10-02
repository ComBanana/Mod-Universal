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

    // Keep the atlas sprite inside a fixed-size local container. This makes
    // the icon's visual position independent of the sprite frame's bounds.
    auto iconContainer = CCLayer::create();
    if (!iconContainer)
        return nullptr;

    iconContainer->setContentSize({32.f, 32.f});
    iconContainer->setAnchorPoint({0.5f, 0.5f});
    sprite->setPosition(iconContainer->getContentSize() / 2.f);
    iconContainer->addChild(sprite);

    auto const showButton = !areSettingsAtDefault(settingKeys);

    auto button = CCMenuItemExt::createSpriteExtra(
        iconContainer,
        [keys = std::move(settingKeys)](CCMenuItemSpriteExtra*) {
            resetSettingsToDefault(keys);
            ModMenu::refreshCurrentTab();
        }
    );

    if (!button)
        return nullptr;

    button->setAnchorPoint({0.5f, 0.5f});
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

    float const rightEdge = panel->getContentSize().width - 20.f;
    float const toggleHalfWidth = 39.f;
    float const iconSlot = 32.f;
    float const gap = 12.f;

    // Positions are the centers of each control, measured from the card's
    // actual right edge so the textures stay fully inside the panel.
    float const checkboxX = rightEdge - toggleHalfWidth;
    float const gearX =
        checkboxX - toggleHalfWidth - gap - iconSlot / 2.f;
    float const undoX =
        gearX - iconSlot / 2.f - gap - iconSlot / 2.f;

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

    panel->setAnchorPoint({0.5f, 0.5f});
    panel->setPosition(center);
    parent->addChild(panel);
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

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

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

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

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
            m_size.width - 22.f,
            m_size.height - 22.f
        });
        m_closeBtn->setScale(0.72f);
    }

    createModernPanel(
        m_mainLayer,
        {m_size.width / 2.f, m_size.height / 2.f},
        {m_size.width, m_size.height},
        {18, 20, 25},
        255
    )->setZOrder(-100);

    createHeader();
    createTabBar();
    createContentPanel();

    auto resetMenu = CCMenu::create();
    resetMenu->setPosition({0.f, 0.f});
    m_mainLayer->addChild(resetMenu);

    if (!createModernActionButton(
        resetMenu,
        "Set to Default",
        {m_size.width - 84.f, 22.f},
        {142.f, 34.f},
        [this]() {
            this->onSetAllToDefault(nullptr);
        }
    )) {
        log::warn("Could not create Set to Default button");
    }

    return true;
}

void ModMenu::createHeader() {
    auto title = createMenuLabel(
        "Mod Universal",
        m_size.width < 520.f ? 0.62f : 0.72f
    );
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({20.f, m_size.height - 26.f});
    m_mainLayer->addChild(title, 100);

    auto subtitle = createMutedMenuLabel(
        "Geometry Dash utilities",
        m_size.width < 520.f ? 0.30f : 0.34f
    );
    subtitle->setAnchorPoint({0.f, 0.5f});
    subtitle->setPosition({20.f, m_size.height - 49.f});
    m_mainLayer->addChild(subtitle);

    auto version = createMutedMenuLabel("v0.2.0", 0.31f);
    version->setAnchorPoint({1.f, 0.5f});
    version->setPosition({
        m_size.width - 20.f,
        m_size.height - 27.f
    });
    m_mainLayer->addChild(version);

    createModernDivider(
        m_mainLayer,
        {m_size.width / 2.f, m_size.height - 69.f},
        m_size.width - 40.f
    );
}

void ModMenu::createTabBar() {
    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    m_mainLayer->addChild(menu);

    constexpr const char* tabs[] = {
        "Player",
        "Visuals",
        "Creator",
        "Misc",
        "Settings"
    };

    bool const compact = isCompactMenu(m_size.width);

    if (!compact) {
        float const sidebarX = 86.f;
        float const sidebarTop = m_size.height - 92.f;
        float const sidebarHeight = m_size.height - 120.f;

        createModernPanel(
            m_mainLayer,
            {sidebarX, 18.f + sidebarHeight / 2.f},
            {144.f, sidebarHeight},
            {27, 29, 35},
            255
        );

        auto section = createMenuLabel(
            "MODULES",
            0.32f,
            {122, 127, 140}
        );
        section->setAnchorPoint({0.f, 0.5f});
        section->setPosition({30.f, sidebarTop - 19.f});
        m_mainLayer->addChild(section);

        for (int i = 0; i < 5; i++) {
            float const y = sidebarTop - 55.f - i * 50.f;

            auto background = CCLayerColor::create(
                {44, 47, 55, 255},
                118.f,
                42.f
            );
            if (!background)
                continue;

            background->setAnchorPoint({0.5f, 0.5f});

            auto label = createMenuLabel(
                tabs[i],
                0.45f,
                {218, 221, 227}
            );
            if (!label)
                continue;

            label->setTag(7002);
            label->setPosition({59.f, 21.f});
            background->addChild(label);

            if (i == m_currentTab) {
                background->setColor({75, 190, 138});
                label->setColor({18, 28, 24});
            }

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
    }
    else {
        bool const twoRows = m_size.width < 450.f;
        int const columns = twoRows ? 3 : 5;
        float const gap = 7.f;
        float const pad = 18.f;
        float const width =
            (m_size.width - pad * 2.f - gap * (columns - 1))
            / columns;
        float const height = twoRows ? 33.f : 36.f;
        float const top = m_size.height - 84.f;

        for (int i = 0; i < 5; i++) {
            auto background = CCLayerColor::create(
                {44, 47, 55, 255},
                width,
                height
            );
            if (!background)
                continue;

            background->setAnchorPoint({0.5f, 0.5f});

            auto label = createMenuLabel(
                tabs[i],
                twoRows ? 0.42f : 0.45f,
                {218, 221, 227}
            );
            if (!label)
                continue;

            label->setTag(7002);
            label->setPosition({width / 2.f, height / 2.f});
            background->addChild(label);

            if (i == m_currentTab) {
                background->setColor({75, 190, 138});
                label->setColor({18, 28, 24});
            }

            if (i == m_currentTab) {
                background->setColor({75, 190, 138});
                label->setColor({18, 28, 24});
            }

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
                pad + width / 2.f + col * (width + gap),
                top - row * (height + 6.f)
            });
            menu->addChild(button);
        }
    }
}

void ModMenu::createContentPanel() {
    bool const compact = isCompactMenu(m_size.width);

    float const navHeight = compact
        ? (m_size.width < 450.f ? 90.f : 54.f)
        : 0.f;

    float const left = compact ? 14.f : 158.f;
    float const top = m_size.height - 82.f - navHeight;
    float const width = m_size.width - left - 14.f;
    float const height = top - 14.f;

    m_contentPanel = CCNode::create();
    m_contentPanel->setContentSize({
        std::max(180.f, width),
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

            background->setColor(
                button == btn
                    ? ccColor3B{75, 190, 138}
                    : ccColor3B{44, 47, 55}
            );

            auto label = typeinfo_cast<CCLabelBMFont*>(
                background->getChildByTag(7002)
            );

            if (label) {
                label->setColor(
                    button == btn
                        ? ccColor3B{18, 28, 24}
                        : ccColor3B{218, 221, 227}
                );
            }
        }
    }

    if (!m_contentPanel)
        return;

    m_contentPanel->removeAllChildrenWithCleanup(true);

    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    m_contentPanel->addChild(menu);

    auto background = createModernPanel(
        m_contentPanel,
        m_contentPanel->getContentSize() / 2.f,
        m_contentPanel->getContentSize(),
        {24, 26, 32},
        255
    );
    if (!background)
        return;

    constexpr const char* tabTitles[] = {
        "Player",
        "Visuals",
        "Creator",
        "Misc",
        "Settings"
    };

    constexpr const char* tabDescriptions[] = {
        "Gameplay tools and player controls.",
        "Visual and rendering tools.",
        "Creator and editor utilities.",
        "Quality-of-life utilities.",
        "ModUniversal configuration."
    };

    bool const compact = isCompactMenu(m_size.width);
    float const width = m_contentPanel->getContentSize().width;
    float const height = m_contentPanel->getContentSize().height;

    auto title = createMenuLabel(
        tabTitles[tab],
        compact ? 0.60f : 0.68f
    );
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({24.f, height - 28.f});
    m_contentPanel->addChild(title);

    auto subtitle = createMutedMenuLabel(
        tabDescriptions[tab],
        compact ? 0.30f : 0.34f
    );
    subtitle->setAnchorPoint({0.f, 0.5f});
    subtitle->setPosition({24.f, height - 52.f});
    m_contentPanel->addChild(subtitle);

    createModernDivider(
        m_contentPanel,
        {width / 2.f, height - 69.f},
        width - 48.f
    );

    if (tab == 0) {
        float const cardHeight = std::min(
            150.f,
            std::max(126.f, height - 100.f)
        );
        float const cardY = height - 122.f;

        auto card = createModernPanel(
            m_contentPanel,
            {width / 2.f, cardY},
            {width - 40.f, cardHeight},
            {38, 41, 49},
            255
        );

        if (!card)
            return;

        auto gameSection = createMenuLabel(
            "GAMEPLAY",
            0.32f,
            {122, 127, 140}
        );
        gameSection->setAnchorPoint({0.f, 0.5f});
        gameSection->setPosition({
            30.f,
            cardY + cardHeight / 2.f - 20.f
        });
        m_contentPanel->addChild(gameSection);

        auto noclipLabel = createMenuLabel(
            "Noclip",
            compact ? 0.56f : 0.62f
        );
        noclipLabel->setAnchorPoint({0.f, 0.5f});
        noclipLabel->setPosition({
            30.f,
            cardY + cardHeight / 2.f - 48.f
        });
        m_contentPanel->addChild(noclipLabel);

        auto noclipDescription = createMutedMenuLabel(
            "Pass through level geometry and hazards.",
            compact ? 0.28f : 0.31f
        );
        noclipDescription->setAnchorPoint({0.f, 0.5f});
        noclipDescription->setPosition({
            30.f,
            cardY + cardHeight / 2.f - 69.f
        });
        m_contentPanel->addChild(noclipDescription);

        s_noclipRowY = cardY - cardHeight / 2.f + 30.f;

        auto gearButton = static_cast<CCMenuItemSpriteExtra*>(nullptr);
        auto gearSprite = CCSprite::createWithSpriteFrameName(
            "GJ_optionsBtn02_001.png"
        );

        if (gearSprite) {
            gearSprite->setAnchorPoint({0.5f, 0.5f});
            gearSprite->setScale(0.72f);

            auto gearContainer = CCLayer::create();
            if (gearContainer) {
                gearContainer->setContentSize({32.f, 32.f});
                gearContainer->setAnchorPoint({0.5f, 0.5f});
                gearSprite->setPosition(
                    gearContainer->getContentSize() / 2.f
                );
                gearContainer->addChild(gearSprite);

                gearButton = CCMenuItemSpriteExtra::create(
                    gearContainer,
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
            {78.f, 32.f}
        );
        s_noclipCheckboxButton = noclipToggle;

        if (noclipToggle)
            noclipToggle->getNormalImage()->setTag(7003);

        updateNoclipRowLayout();
        return;
    }

    float const emptyY = std::max(74.f, height * 0.43f);

    auto emptyCard = createModernPanel(
        m_contentPanel,
        {width / 2.f, emptyY},
        {width - 40.f, 122.f},
        {38, 41, 49},
        255
    );

    if (!emptyCard)
        return;

    auto emptyTitle = createMenuLabel("Coming soon", 0.52f);
    emptyTitle->setPosition({width / 2.f, emptyY + 10.f});
    m_contentPanel->addChild(emptyTitle);

    auto emptyDescription = createMutedMenuLabel(
        "This section is ready for future ModUniversal modules.",
        0.29f
    );
    emptyDescription->setPosition({width / 2.f, emptyY - 16.f});
    m_contentPanel->addChild(emptyDescription);
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
