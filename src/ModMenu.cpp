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

    sprite->setScale(0.8f);

    auto const showButton = !areSettingsAtDefault(settingKeys);

    auto button = CCMenuItemExt::createSpriteExtra(
        sprite,
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

    float const right = panel->getContentSize().width - 30.f;
    float const checkboxX = right;
    float const gearX = right - 46.f;
    float const undoX = right - 92.f;

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

CCLabelBMFont* createMenuLabel(char const* text, float scale = 0.45f) {
    auto label = CCLabelBMFont::create(text, "goldFont.fnt");
    label->setScale(scale);
    return label;
}

CCLabelBMFont* createMutedMenuLabel(char const* text, float scale = 0.34f) {
    auto label = CCLabelBMFont::create(text, "chatFont.fnt");
    label->setScale(scale);
    label->setOpacity(180);
    return label;
}

CCSize getResponsivePopupSize(float preferredWidth, float preferredHeight) {
    auto const screen = CCDirector::sharedDirector()->getWinSize();

    float const width = std::max(
        260.f,
        std::min(preferredWidth, screen.width - 24.f)
    );
    float const height = std::max(
        200.f,
        std::min(preferredHeight, screen.height - 24.f)
    );

    return {width, height};
}

bool isCompactMenu(float width) {
    return width < 650.f;
}

extension::CCScale9Sprite* createModernPanel(
    CCNode* parent,
    CCPoint center,
    CCSize size,
    ccColor3B color = {43, 43, 50},
    GLubyte opacity = 238
) {
    if (!parent)
        return nullptr;

    auto panel = extension::CCScale9Sprite::create("square02_small.png");
    if (!panel)
        return nullptr;

    panel->setContentSize(size);
    panel->setColor(color);
    panel->setOpacity(opacity);
    panel->setPosition(center);
    parent->addChild(panel);
    return panel;
}

void addModernDivider(CCNode* parent, CCPoint position, float width) {
    if (!parent)
        return;

    auto divider = extension::CCScale9Sprite::create("square02_small.png");
    if (!divider)
        return;

    divider->setContentSize({width, 1.5f});
    divider->setColor({78, 78, 88});
    divider->setOpacity(190);
    divider->setPosition(position);
    parent->addChild(divider);
}

CCSprite* createNoclipCheckboxSprite(bool enabled, float scale = 0.8f) {
    auto sprite = CCSprite::createWithSpriteFrameName(
        enabled
            ? "GJ_checkOn_001.png"
            : "GJ_checkOff_001.png"
    );

    if (sprite)
        sprite->setScale(scale);

    return sprite;
}

void saveNoclipButtonSettings() {
    if (auto result = Mod::get()->saveData(); !result) {
        log::error(
            "Failed to save Noclip button settings: {}",
            result.unwrapErr()
        );
    }
}

void refreshNoclipCheckbox(
    CCMenuItemSpriteExtra* button,
    bool enabled
) {
    if (!button)
        return;

    auto sprite = typeinfo_cast<CCSprite*>(button->getNormalImage());

    if (!sprite) {
        log::error("Failed to find Noclip checkbox sprite");
        return;
    }

    // Do not replace the menu item's normal image here. Cocos2d's
    // setNormalImage() resets the replacement sprite's anchor point and
    // content geometry, which makes the button visibly jump after a click.
    // Changing the existing sprite frame keeps its position, anchor and
    // scale intact.
    auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
        enabled
            ? "GJ_checkOn_001.png"
            : "GJ_checkOff_001.png"
    );

    if (!frame) {
        log::error("Failed to find Noclip checkbox frame");
        return;
    }

    sprite->setDisplayFrame(frame);
}

CCMenuItemSpriteExtra* createNoclipCheckbox(
    CCNode* parent,
    char const* settingKey,
    CCPoint position
) {
    if (!parent || !settingKey)
        return nullptr;

    auto const key = std::string(settingKey);
    auto initialState = Mod::get()->getSettingValue<bool>(key);

    auto sprite = createNoclipCheckboxSprite(initialState);

    if (!sprite)
        return nullptr;

    auto button = CCMenuItemExt::createSpriteExtra(
        sprite,
        [key](CCMenuItemSpriteExtra* item) {
            auto* mod = Mod::get();

            auto current = mod->getSettingValue<bool>(key);
            auto next = !current;

            if (key == "noclip-enabled") {
                // Route the main Noclip control through the same ModMenu API
                // used by the gameplay code rather than treating it as a
                // generic UI-only boolean.
                ModMenu::setNoclipEnabled(next);
            }
            else {
                mod->setSettingValue<bool>(key, next);
            }

            refreshNoclipCheckbox(item, next);
            saveNoclipButtonSettings();
            updateNoclipDefaultButton();

            log::info(
                "Noclip setting '{}' changed to {}",
                key,
                next ? "ON" : "OFF"
            );
        }
    );

    if (!button)
        return nullptr;

    button->setPosition(position);
    parent->addChild(button);
    return button;
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
        auto const popupSize = getResponsivePopupSize(540.f, 380.f);

        if (!Popup::init(popupSize.width, popupSize.height))
            return false;

        auto title = createMenuLabel("Spike Phasing", 0.68f);
        title->setPosition({36.f, m_size.height - 34.f});
        title->setAnchorPoint({0.f, 0.5f});
        m_mainLayer->addChild(title);

        auto subtitle = createMutedMenuLabel(
            "Choose which hazard categories are ignored by noclip.",
            0.34f
        );
        subtitle->setPosition({36.f, m_size.height - 58.f});
        subtitle->setAnchorPoint({0.f, 0.5f});
        m_mainLayer->addChild(subtitle);

        createModernPanel(
            m_mainLayer,
            {m_size.width / 2.f, (m_size.height - 20.f) / 2.f},
            {m_size.width - 32.f, m_size.height - 116.f},
            {38, 38, 45},
            238
        );

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        float const topY = m_size.height - 101.f;
        float const gap = std::max(
            28.f,
            std::min(35.f, (m_size.height - 156.f) / 5.f)
        );

        addToggleRow(menu, "Spikes", "noclip-hazard-spikes", topY);
        addToggleRow(menu, "Ground / Edge Spikes", "noclip-hazard-ground-spikes", topY - gap);
        addToggleRow(menu, "Sawblades", "noclip-hazard-saws", topY - gap * 2.f);
        addToggleRow(menu, "Pits", "noclip-hazard-pits", topY - gap * 3.f);
        addToggleRow(menu, "Animated Hazards", "noclip-hazard-animated", topY - gap * 4.f);
        addToggleRow(menu, "Other Hazards", "noclip-hazard-other", topY - gap * 5.f);

        auto closeSprite = ButtonSprite::create(
            "Back",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.68f
        );
        auto closeButton = CCMenuItemSpriteExtra::create(
            closeSprite,
            this,
            menu_selector(NoclipHazardPopup::onClosePopup)
        );
        closeButton->setPosition({m_size.width / 2.f, 28.f});
        menu->addChild(closeButton);

        return true;
    }

    void addToggleRow(
        CCMenu* menu,
        char const* labelText,
        char const* settingKey,
        float y
    ) {
        auto label = createMenuLabel(labelText, 0.42f);
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({50.f, y});
        m_mainLayer->addChild(label);

        auto toggle = createNoclipCheckbox(
            menu,
            settingKey,
            {m_size.width - 56.f, y}
        );

        if (!toggle)
            log::warn("Could not create noclip hazard checkbox for {}", settingKey);
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
    ButtonSprite* m_blockModeButton = nullptr;

    bool init() {
        auto const popupSize = getResponsivePopupSize(560.f, 380.f);

        if (!Popup::init(popupSize.width, popupSize.height))
            return false;

        auto title = createMenuLabel("Noclip", 0.64f);
        title->setPosition({26.f, m_size.height - 31.f});
        title->setAnchorPoint({0.f, 0.5f});
        m_mainLayer->addChild(title);

        auto subtitle = createMutedMenuLabel(
            "Configure how Noclip interacts with the level.",
            0.30f
        );
        subtitle->setPosition({26.f, m_size.height - 53.f});
        subtitle->setAnchorPoint({0.f, 0.5f});
        m_mainLayer->addChild(subtitle);

        createModernPanel(
            m_mainLayer,
            {m_size.width / 2.f, (m_size.height - 20.f) / 2.f},
            {m_size.width - 32.f, m_size.height - 116.f},
            {38, 38, 45},
            235
        );

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        float const rowY = m_size.height - 101.f;
        addToggleRow(menu, "Block Phasing", "noclip-phase-blocks", rowY);
        addToggleRow(menu, "Slope Phasing", "noclip-phase-slopes", rowY - 37.f);
        addToggleRow(menu, "Spike Phasing", "noclip-phase-hazards", rowY - 74.f);

        auto modeLabel = createMenuLabel("Block Collision", 0.42f);
        modeLabel->setAnchorPoint({0.f, 0.5f});
        modeLabel->setPosition({55.f, 148.f});
        m_mainLayer->addChild(modeLabel);

        m_blockModeButton = createBlockModeButton();
        auto modeButton = CCMenuItemSpriteExtra::create(
            m_blockModeButton,
            this,
            menu_selector(NoclipSettingsPopup::onBlockMode)
        );
        modeButton->setPosition({410.f, 148.f});
        menu->addChild(modeButton);

        auto hazardButtonSprite = ButtonSprite::create(
            "Configure Hazards",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.60f
        );
        auto hazardButton = CCMenuItemSpriteExtra::create(
            hazardButtonSprite,
            this,
            menu_selector(NoclipSettingsPopup::onHazardSettings)
        );

        // Temporarily disabled while the hazard configuration system is being
        // reworked. Keep the button visible but gray.
        hazardButtonSprite->setColor({120, 120, 120});
        hazardButton->setEnabled(false);
        hazardButton->setPosition({m_size.width / 2.f, 92.f});
        menu->addChild(hazardButton);

        auto info = createMutedMenuLabel(
            "Hazard categories will be configurable here later.",
            0.3f
        );
        info->setPosition({m_size.width / 2.f, 67.f});
        m_mainLayer->addChild(info);

        auto closeSprite = ButtonSprite::create(
            "Back",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.68f
        );
        auto closeButton = CCMenuItemSpriteExtra::create(
            closeSprite,
            this,
            menu_selector(NoclipSettingsPopup::onClosePopup)
        );
        closeButton->setPosition({m_size.width / 2.f, 28.f});
        menu->addChild(closeButton);

        return true;
    }

    CCMenuItemSpriteExtra* addToggleRow(
        CCMenu* menu,
        char const* labelText,
        char const* settingKey,
        float y
    ) {
        auto label = createMenuLabel(labelText, 0.44f);
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({50.f, y});
        m_mainLayer->addChild(label);

        auto toggle = createNoclipCheckbox(
            menu,
            settingKey,
            {448.f, y}
        );

        if (!toggle)
            log::warn("Could not create noclip checkbox for {}", settingKey);

        return toggle;
    }

    ButtonSprite* createBlockModeButton() {
        auto mode = Mod::get()->getSettingValue<std::string>("noclip-block-mode");
        auto text = mode == "no-touch"
            ? "No Hitbox"
            : "Standard";

        return ButtonSprite::create(
            text,
            "goldFont.fnt",
            "GJ_button_01.png",
            0.50f
        );
    }

    void onBlockMode(CCObject*) {
        auto current = Mod::get()->getSettingValue<std::string>("noclip-block-mode");
        auto next = current == "no-touch"
            ? "safe-touch"
            : "no-touch";

        Mod::get()->setSettingValue<std::string>("noclip-block-mode", next);

        if (m_blockModeButton) {
            m_blockModeButton->setString(
                next == "no-touch"
                    ? "No Hitbox"
                    : "Standard"
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

    auto const popupSize = getResponsivePopupSize(760.f, 470.f);

    if (!Popup::init(popupSize.width, popupSize.height))
        return false;

    createHeader();
    createTabBar();
    createContentPanel();

    auto resetSprite = ButtonSprite::create(
        "Set to Default",
        "goldFont.fnt",
        "GJ_button_01.png",
        0.44f
    );

    if (resetSprite) {
        auto resetButton = CCMenuItemSpriteExtra::create(
            resetSprite,
            this,
            menu_selector(ModMenu::onSetAllToDefault)
        );
        resetButton->setPosition({
            m_size.width - resetSprite->getScaledContentSize().width / 2.f - 16.f,
            16.f
        });
        m_mainLayer->addChild(resetButton);
    }

    return true;
}

void ModMenu::createHeader() {
    bool const compact = isCompactMenu(m_size.width);

    auto logo = CCSprite::create("PopupTitle.png"_spr);
    if (logo && (!compact || m_size.width >= 450.f)) {
        float const desiredWidth = compact ? 92.f : 130.f;
        logo->setScale(desiredWidth / logo->getContentSize().width);
        logo->setAnchorPoint({0.f, 0.5f});
        logo->setPosition({18.f, m_size.height - 30.f});
        m_mainLayer->addChild(logo, 100);
    }

    float const titleX =
        (!compact || m_size.width >= 450.f) ? 158.f : 20.f;

    auto title = createMenuLabel(
        "MOD UNIVERSAL",
        compact ? 0.52f : 0.58f
    );
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({titleX, m_size.height - 23.f});
    m_mainLayer->addChild(title);

    auto subtitle = createMutedMenuLabel(
        "Universal Geometry Dash toolkit",
        compact ? 0.25f : 0.28f
    );
    subtitle->setAnchorPoint({0.f, 0.5f});
    subtitle->setPosition({titleX, m_size.height - 44.f});
    m_mainLayer->addChild(subtitle);

    auto version = createMutedMenuLabel("v0.2.0", 0.27f);
    version->setAnchorPoint({1.f, 0.5f});
    version->setPosition({m_size.width - 18.f, m_size.height - 23.f});
    m_mainLayer->addChild(version);

    addModernDivider(
        m_mainLayer,
        {m_size.width / 2.f, m_size.height - 67.f},
        m_size.width - 36.f
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
        createModernPanel(
            m_mainLayer,
            {88.f, (m_size.height - 84.f) / 2.f},
            {138.f, m_size.height - 94.f},
            {34, 34, 41},
            240
        );

        float const startY = m_size.height - 101.f;
        float const gap = 51.f;

        for (int i = 0; i < 5; i++) {
            auto background =
                extension::CCScale9Sprite::create("square02_small.png");
            if (!background)
                continue;

            background->setContentSize({114.f, 38.f});
            background->setColor({48, 48, 56});
            background->setOpacity(225);

            auto label = CCLabelBMFont::create(tabs[i], "goldFont.fnt");
            if (!label)
                continue;

            label->setScale(0.45f);
            label->setColor({215, 215, 220});
            label->setPosition({57.f, 19.f});
            background->addChild(label);

            auto button = CCMenuItemSpriteExtra::create(
                background,
                this,
                menu_selector(ModMenu::onTab)
            );
            if (!button)
                continue;

            button->setTag(i);
            button->setPosition({88.f, startY - i * gap});
            menu->addChild(button);
        }
    }
    else {
        bool const twoRows = m_size.width < 450.f;

        if (twoRows) {
            float const gap = 7.f;
            float const width =
                (m_size.width - 36.f - gap * 2.f) / 3.f;

            for (int i = 0; i < 5; i++) {
                auto background =
                    extension::CCScale9Sprite::create("square02_small.png");
                if (!background)
                    continue;

                background->setContentSize({width, 30.f});
                background->setColor({48, 48, 56});
                background->setOpacity(225);

                auto label = CCLabelBMFont::create(tabs[i], "goldFont.fnt");
                if (!label)
                    continue;

                label->setScale(0.39f);
                label->setColor({215, 215, 220});
                label->setPosition({width / 2.f, 15.f});
                background->addChild(label);

                auto button = CCMenuItemSpriteExtra::create(
                    background,
                    this,
                    menu_selector(ModMenu::onTab)
                );
                if (!button)
                    continue;

                int const row = i / 3;
                int const column = i % 3;

                button->setTag(i);
                button->setPosition({
                    18.f + width / 2.f + column * (width + gap),
                    m_size.height - 79.f - row * 35.f
                });
                menu->addChild(button);
            }
        }
        else {
            float const gap = 6.f;
            float const width =
                (m_size.width - 36.f - gap * 4.f) / 5.f;

            for (int i = 0; i < 5; i++) {
                auto background =
                    extension::CCScale9Sprite::create("square02_small.png");
                if (!background)
                    continue;

                background->setContentSize({width, 32.f});
                background->setColor({48, 48, 56});
                background->setOpacity(225);

                auto label = CCLabelBMFont::create(tabs[i], "goldFont.fnt");
                if (!label)
                    continue;

                label->setScale(0.40f);
                label->setColor({215, 215, 220});
                label->setPosition({width / 2.f, 16.f});
                background->addChild(label);

                auto button = CCMenuItemSpriteExtra::create(
                    background,
                    this,
                    menu_selector(ModMenu::onTab)
                );
                if (!button)
                    continue;

                button->setTag(i);
                button->setPosition({
                    18.f + width / 2.f + i * (width + gap),
                    m_size.height - 83.f
                });
                menu->addChild(button);
            }
        }
    }
}

void ModMenu::createContentPanel() {
    bool const compact = isCompactMenu(m_size.width);

    float const left = compact ? 18.f : 166.f;
    float const top = compact
        ? (m_size.width < 450.f ? 142.f : 128.f)
        : m_size.height - 82.f;

    m_contentPanel = CCNode::create();
    m_contentPanel->setContentSize({
        m_size.width - left - 18.f,
        top - 18.f
    });
    m_contentPanel->setPosition({left, 18.f});
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
                typeinfo_cast<extension::CCScale9Sprite*>(
                    button->getNormalImage()
                );
            if (!background)
                continue;

            background->setColor(
                button == btn
                    ? ccColor3B{64, 85, 66}
                    : ccColor3B{48, 48, 56}
            );
        }
    }

    if (!m_contentPanel)
        return;

    m_contentPanel->removeAllChildrenWithCleanup(true);

    if (!createModernPanel(
        m_contentPanel,
        m_contentPanel->getContentSize() / 2.f,
        m_contentPanel->getContentSize(),
        {42, 42, 49},
        238
    ))
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
        "Level creation and editor tools.",
        "Utilities and quality-of-life features.",
        "ModUniversal configuration."
    };

    bool const compact = isCompactMenu(m_size.width);
    float const width = m_contentPanel->getContentSize().width;
    float const height = m_contentPanel->getContentSize().height;

    auto title = createMenuLabel(
        tabTitles[tab],
        compact ? 0.56f : 0.62f
    );
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({24.f, height - 27.f});
    m_contentPanel->addChild(title);

    auto subtitle = createMutedMenuLabel(
        tabDescriptions[tab],
        compact ? 0.27f : 0.30f
    );
    subtitle->setAnchorPoint({0.f, 0.5f});
    subtitle->setPosition({24.f, height - 48.f});
    m_contentPanel->addChild(subtitle);

    addModernDivider(
        m_contentPanel,
        {width / 2.f, height - 64.f},
        width - 48.f
    );

    if (tab == 0) {
        float const cardHeight = compact ? 112.f : 126.f;
        float const cardY = std::max(
            78.f,
            std::min(height * 0.50f, height - 93.f)
        );

        auto card = createModernPanel(
            m_contentPanel,
            {width / 2.f, cardY},
            {width - 40.f, cardHeight},
            {51, 51, 59},
            246
        );

        if (!card)
            return;

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_contentPanel->addChild(menu);

        auto noclipLabel = createMenuLabel(
            "Noclip",
            compact ? 0.46f : 0.50f
        );
        noclipLabel->setAnchorPoint({0.f, 0.5f});
        noclipLabel->setPosition({34.f, cardY + 23.f});
        m_contentPanel->addChild(noclipLabel);

        auto noclipDescription = createMutedMenuLabel(
            "Pass through level geometry and hazards.",
            compact ? 0.25f : 0.28f
        );
        noclipDescription->setAnchorPoint({0.f, 0.5f});
        noclipDescription->setPosition({34.f, cardY + 4.f});
        m_contentPanel->addChild(noclipDescription);

        s_noclipRowY = cardY - 26.f;

        auto gearSprite = CCSprite::createWithSpriteFrameName(
            "GJ_optionsBtn02_001.png"
        );

        auto noclipDefaultButton = createHackDefaultButton(
            menu,
            NoclipSettingKeys,
            {0.f, s_noclipRowY}
        );
        s_noclipDefaultButton = noclipDefaultButton;

        if (gearSprite) {
            gearSprite->setScale(0.66f);

            auto gearButton = CCMenuItemSpriteExtra::create(
                gearSprite,
                this,
                menu_selector(ModMenu::onNoclipSettings)
            );
            s_noclipSettingsButton = gearButton;
            menu->addChild(gearButton);
        }
        else {
            log::warn("Could not load noclip settings gear sprite");
        }

        auto noclipToggle = createNoclipCheckbox(
            menu,
            "noclip-enabled",
            {0.f, s_noclipRowY}
        );
        s_noclipCheckboxButton = noclipToggle;

        if (!noclipToggle)
            log::warn("Could not create noclip checkbox");

        updateNoclipRowLayout();
        return;
    }

    auto emptyCard = createModernPanel(
        m_contentPanel,
        {width / 2.f, std::max(82.f, height * 0.45f)},
        {width - 40.f, 108.f},
        {51, 51, 59},
        246
    );

    if (!emptyCard)
        return;

    auto emptyTitle = createMenuLabel("No modules yet", 0.45f);
    emptyTitle->setPosition({
        width / 2.f,
        std::max(82.f, height * 0.45f) + 12.f
    });
    m_contentPanel->addChild(emptyTitle);

    auto emptyDescription = createMutedMenuLabel(
        "This section is ready for future ModUniversal features.",
        0.27f
    );
    emptyDescription->setPosition({
        width / 2.f,
        std::max(82.f, height * 0.45f) - 13.f
    });
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
