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

constexpr float NoclipRowY = 166.f;
constexpr float NoclipUndoX = 332.f;
constexpr float NoclipGearXWithUndo = 374.f;
constexpr float NoclipGearXWithoutUndo = 416.f;
constexpr float NoclipCheckboxX = 462.f;

void updateNoclipRowLayout() {
    bool const hasNoclipChanges = !areSettingsAtDefault(NoclipSettingKeys);

    if (auto button = s_noclipDefaultButton.lock()) {
        button->setVisible(hasNoclipChanges);
        button->setPosition({NoclipUndoX, NoclipRowY});
    }

    if (auto button = s_noclipSettingsButton.lock()) {
        button->setPosition({
            hasNoclipChanges ? NoclipGearXWithUndo : NoclipGearXWithoutUndo,
            NoclipRowY
        });
    }

    if (auto button = s_noclipCheckboxButton.lock())
        button->setPosition({NoclipCheckboxX, NoclipRowY});
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
    auto label = CCLabelBMFont::create(text, "goldFont.fnt");
    label->setScale(scale);
    label->setOpacity(175);
    return label;
}

extension::CCScale9Sprite* createModernPanel(
    CCNode* parent,
    CCPoint center,
    CCSize size,
    ccColor3B color = {45, 45, 52},
    GLubyte opacity = 220
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
    divider->setColor({70, 70, 78});
    divider->setOpacity(170);
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
        if (!Popup::init(520.f, 360.f))
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
            {m_size.width / 2.f, 170.f},
            {m_size.width - 36.f, 210.f},
            {38, 38, 45},
            235
        );

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        addToggleRow(menu, "Spikes", "noclip-hazard-spikes", 246.f);
        addToggleRow(menu, "Ground / Edge Spikes", "noclip-hazard-ground-spikes", 213.f);
        addToggleRow(menu, "Sawblades", "noclip-hazard-saws", 180.f);
        addToggleRow(menu, "Pits", "noclip-hazard-pits", 147.f);
        addToggleRow(menu, "Animated Hazards", "noclip-hazard-animated", 114.f);
        addToggleRow(menu, "Other Hazards", "noclip-hazard-other", 81.f);

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
        label->setPosition({55.f, y});
        m_mainLayer->addChild(label);

        auto toggle = createNoclipCheckbox(
            menu,
            settingKey,
            {448.f, y}
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
        if (!Popup::init(520.f, 340.f))
            return false;

        auto title = createMenuLabel("Noclip", 0.68f);
        title->setPosition({36.f, m_size.height - 34.f});
        title->setAnchorPoint({0.f, 0.5f});
        m_mainLayer->addChild(title);

        auto subtitle = createMutedMenuLabel(
            "Configure how Noclip interacts with the level.",
            0.34f
        );
        subtitle->setPosition({36.f, m_size.height - 58.f});
        subtitle->setAnchorPoint({0.f, 0.5f});
        m_mainLayer->addChild(subtitle);

        createModernPanel(
            m_mainLayer,
            {m_size.width / 2.f, 199.f},
            {m_size.width - 36.f, 188.f},
            {38, 38, 45},
            235
        );

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        addToggleRow(menu, "Block Phasing", "noclip-phase-blocks", 253.f);
        addToggleRow(menu, "Slope Phasing", "noclip-phase-slopes", 220.f);
        addToggleRow(menu, "Spike Phasing", "noclip-phase-hazards", 187.f);

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
        label->setPosition({55.f, y});
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

    if (!Popup::init(680.f, 400.f))
        return false;

    createHeader();
    createTabBar();
    createContentPanel();

    auto resetSprite = ButtonSprite::create(
        "Set to Default",
        "goldFont.fnt",
        "GJ_button_01.png",
        0.48f
    );

    if (resetSprite) {
        auto resetButton = CCMenuItemSpriteExtra::create(
            resetSprite,
            this,
            menu_selector(ModMenu::onSetAllToDefault)
        );
        resetButton->setPosition({m_size.width - 73.f, 17.f});
        m_mainLayer->addChild(resetButton);
    }

    return true;
}

void ModMenu::createHeader() {
    auto title = createMenuLabel("Mod Universal", 0.78f);
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({22.f, m_size.height - 25.f});
    m_mainLayer->addChild(title);

    auto subtitle = createMutedMenuLabel(
        "Universal Geometry Dash toolkit",
        0.31f
    );
    subtitle->setAnchorPoint({0.f, 0.5f});
    subtitle->setPosition({22.f, m_size.height - 48.f});
    m_mainLayer->addChild(subtitle);

    auto version = createMutedMenuLabel("v0.1.3", 0.30f);
    version->setAnchorPoint({1.f, 0.5f});
    version->setPosition({m_size.width - 46.f, m_size.height - 32.f});
    m_mainLayer->addChild(version);

    auto divider = extension::CCScale9Sprite::create("square02_small.png");
    if (divider) {
        divider->setContentSize({m_size.width - 32.f, 1.5f});
        divider->setColor({70, 70, 78});
        divider->setOpacity(170);
        divider->setPosition({m_size.width / 2.f, m_size.height - 63.f});
        m_mainLayer->addChild(divider);
    }
}

void ModMenu::createTabBar() {
    auto sidebar = createModernPanel(
        m_mainLayer,
        {74.f, 191.f},
        {132.f, 294.f},
        {33, 33, 39},
        238
    );

    if (!sidebar)
        log::warn("Could not create sidebar panel");

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

    constexpr float startY = 292.f;
    constexpr float gap = 52.f;

    for (int i = 0; i < 5; i++) {
        auto spr = ButtonSprite::create(
            tabs[i],
            "goldFont.fnt",
            "GJ_button_04.png",
            0.52f
        );
        if (!spr)
            continue;

        spr->setScale(0.82f);

        auto btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            menu_selector(ModMenu::onTab)
        );
        if (!btn)
            continue;

        btn->setTag(i);
        btn->setPosition({74.f, startY - i * gap});

        spr->setColor(
            i == 0
                ? ccColor3B{120, 255, 120}
                : ccColor3B{220, 220, 220}
        );

        menu->addChild(btn);
    }
}

void ModMenu::createContentPanel() {
    m_contentPanel = CCNode::create();

    m_contentPanel->setContentSize({
        m_size.width - 170.f,
        294.f
    });

    m_contentPanel->setPosition({
        148.f,
        42.f
    });

    m_mainLayer->addChild(m_contentPanel);

    createModernPanel(
        m_contentPanel,
        m_contentPanel->getContentSize() / 2.f,
        m_contentPanel->getContentSize(),
        {42, 42, 49},
        236
    );

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
            auto otherButton = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
            if (!otherButton)
                continue;

            auto sprite = typeinfo_cast<ButtonSprite*>(otherButton->getNormalImage());
            if (!sprite)
                continue;

            sprite->setColor(
                otherButton == btn
                    ? ccColor3B{120, 255, 120}
                    : ccColor3B{220, 220, 220}
            );
        }
    }

    if (!m_contentPanel)
        return;

    m_contentPanel->removeAllChildrenWithCleanup(true);

    createModernPanel(
        m_contentPanel,
        m_contentPanel->getContentSize() / 2.f,
        m_contentPanel->getContentSize(),
        {42, 42, 49},
        236
    );

    constexpr const char* tabTitles[] = {
        "Player",
        "Visuals",
        "Creator",
        "Misc",
        "Settings"
    };

    constexpr const char* tabDescriptions[] = {
        "Gameplay modifications and player controls.",
        "Visual and rendering tools.",
        "Level creation and editor tools.",
        "Utilities and quality-of-life features.",
        "ModUniversal configuration."
    };

    auto title = createMenuLabel(tabTitles[tab], 0.68f);
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({24.f, 258.f});
    m_contentPanel->addChild(title);

    auto subtitle = createMutedMenuLabel(tabDescriptions[tab], 0.32f);
    subtitle->setAnchorPoint({0.f, 0.5f});
    subtitle->setPosition({24.f, 235.f});
    m_contentPanel->addChild(subtitle);

    addModernDivider(
        m_contentPanel,
        {m_contentPanel->getContentSize().width / 2.f, 220.f},
        m_contentPanel->getContentSize().width - 48.f
    );

    if (tab == 0) {
        auto card = createModernPanel(
            m_contentPanel,
            {m_contentPanel->getContentSize().width / 2.f, 150.f},
            {m_contentPanel->getContentSize().width - 40.f, 116.f},
            {50, 50, 58},
            245
        );

        if (!card)
            return;

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_contentPanel->addChild(menu);

        auto noclipLabel = createMenuLabel("Noclip", 0.52f);
        noclipLabel->setAnchorPoint({0.f, 0.5f});
        noclipLabel->setPosition({32.f, 177.f});
        m_contentPanel->addChild(noclipLabel);

        auto noclipDescription = createMutedMenuLabel(
            "Pass through level geometry and hazards.",
            0.30f
        );
        noclipDescription->setAnchorPoint({0.f, 0.5f});
        noclipDescription->setPosition({32.f, 156.f});
        m_contentPanel->addChild(noclipDescription);

        auto gearSprite = CCSprite::createWithSpriteFrameName(
            "GJ_optionsBtn02_001.png"
        );

        auto noclipDefaultButton = createHackDefaultButton(
            menu,
            NoclipSettingKeys,
            {NoclipUndoX, NoclipRowY}
        );
        s_noclipDefaultButton = noclipDefaultButton;

        bool const hasNoclipChanges =
            noclipDefaultButton && noclipDefaultButton->isVisible();

        if (gearSprite) {
            gearSprite->setScale(0.72f);

            auto gearButton = CCMenuItemSpriteExtra::create(
                gearSprite,
                this,
                menu_selector(ModMenu::onNoclipSettings)
            );
            gearButton->setPosition({
                hasNoclipChanges ? NoclipGearXWithUndo : NoclipGearXWithoutUndo,
                NoclipRowY
            });
            s_noclipSettingsButton = gearButton;
            menu->addChild(gearButton);
        }
        else {
            log::warn("Could not load noclip settings gear sprite");
        }

        auto noclipToggle = createNoclipCheckbox(
            menu,
            "noclip-enabled",
            {NoclipCheckboxX, NoclipRowY}
        );
        s_noclipCheckboxButton = noclipToggle;

        if (!noclipToggle)
            log::warn("Could not create noclip checkbox");

        return;
    }

    auto emptyCard = createModernPanel(
        m_contentPanel,
        {m_contentPanel->getContentSize().width / 2.f, 143.f},
        {m_contentPanel->getContentSize().width - 40.f, 104.f},
        {50, 50, 58},
        245
    );

    if (!emptyCard)
        return;

    auto emptyTitle = createMenuLabel("No options yet", 0.48f);
    emptyTitle->setPosition({
        m_contentPanel->getContentSize().width / 2.f,
        157.f
    });
    m_contentPanel->addChild(emptyTitle);

    auto emptyDescription = createMutedMenuLabel(
        "This section is ready for future ModUniversal modules.",
        0.30f
    );
    emptyDescription->setPosition({
        m_contentPanel->getContentSize().width / 2.f,
        134.f
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
