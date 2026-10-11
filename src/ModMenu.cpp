#include "../include/ModMenu.hpp"
#include "../include/ui/Catalog.hpp"
#include "../include/ui/NoclipPopups.hpp"
#include "../include/ui/Overlay.hpp"
#include "../include/ui/Theme.hpp"
#include "../include/ui/Widgets.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/modify/PlayLayer.hpp>
<<<<<<< Updated upstream

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

void updateNoclipRowLayout() {
    bool const hasNoclipChanges = !areSettingsAtDefault(NoclipSettingKeys);

    if (auto button = s_noclipDefaultButton.lock())
        button->setVisible(hasNoclipChanges);

    if (auto button = s_noclipSettingsButton.lock()) {
        button->setPosition({
            hasNoclipChanges ? 235.f : 275.f,
            62.f
        });
    }

    if (auto button = s_noclipCheckboxButton.lock()) {
        button->setPosition({330.f, 62.f});
    }
}

void updateNoclipDefaultButton() {
    updateNoclipRowLayout();
}

CCLabelBMFont* createMenuLabel(char const* text, float scale = 0.45f) {
    auto label = CCLabelBMFont::create(text, "goldFont.fnt");
    label->setScale(scale);
    return label;
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
        if (!Popup::init(430.f, 300.f))
            return false;

        auto title = createMenuLabel("Hazard Phasing", 0.6f);
        title->setPosition({m_size.width / 2.f, m_size.height - 30.f});
        m_mainLayer->addChild(title);

        auto subtitle = createMenuLabel("Choose which hazard categories noclip ignores.", 0.32f);
        subtitle->setPosition({m_size.width / 2.f, m_size.height - 52.f});
        subtitle->setOpacity(190);
        m_mainLayer->addChild(subtitle);

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        addToggleRow(menu, "Spikes", "noclip-hazard-spikes", 225.f);
        addToggleRow(menu, "Ground / Edge Spikes", "noclip-hazard-ground-spikes", 192.f);
        addToggleRow(menu, "Sawblades", "noclip-hazard-saws", 159.f);
        addToggleRow(menu, "Pits", "noclip-hazard-pits", 126.f);
        addToggleRow(menu, "Animated Hazards", "noclip-hazard-animated", 93.f);
        addToggleRow(menu, "Other Hazards", "noclip-hazard-other", 60.f);

        auto closeSprite = ButtonSprite::create(
            "Back",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.7f
        );

        auto closeButton = CCMenuItemSpriteExtra::create(
            closeSprite,
            this,
            menu_selector(NoclipHazardPopup::onClosePopup)
        );

        closeButton->setPosition({m_size.width / 2.f, 27.f});
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
        label->setPosition({65.f, y});
        m_mainLayer->addChild(label);

        auto toggle = createNoclipCheckbox(
            menu,
            settingKey,
            {345.f, y}
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
=======
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Scrollbar.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace mu;

namespace {

constexpr int TabLabelTag = 7002;
constexpr int StatusLabelTag = 7102;

CCNode* createFeatureCard(catalog::FeatureDef const& feature, CCSize size) {
    auto card = CCNode::create();
    if (!card)
        return nullptr;

    card->setContentSize(size);
    card->ignoreAnchorPointForPosition(false);
    card->setAnchorPoint({0.5f, 0.5f});

    auto background = ui::roundedRect(theme::Card, size, theme::CardRadius);
    if (background) {
        background->setPosition(size / 2.f);
        card->addChild(background, 0);
    }

    float const pad = std::min(22.f, size.width * 0.05f);
    auto title = ui::label(feature.title, size.height >= 120.f ? 0.70f : 0.58f);
    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({pad, size.height - 29.f});
        title->limitLabelWidth(size.width - pad * 2.f - 88.f, title->getScale(), 0.42f);
        card->addChild(title, 2);
    }

    auto desc = ui::label(feature.description, 0.48f, theme::Text);
    if (desc) {
        desc->setAnchorPoint({0.f, 1.f});
        desc->setPosition({pad, size.height - 46.f});
        desc->limitLabelWidth(size.width - pad * 2.f, 0.48f, 0.32f);
        card->addChild(desc, 2);
    }

    auto menu = ui::menuOn(card, 12);
    if (!menu)
        return card;

    char const* const status = catalog::statusLabel(feature);
    bool const active = catalog::isStatusActive(feature);
    float const pillWidth = std::strcmp(status, "ON") == 0 ? 72.f : 62.f;
    CCPoint const pillPos = {
        size.width - pad - pillWidth / 2.f,
        size.height - 29.f
    };

    if (feature.status == catalog::FeatureStatus::Toggle) {
        ui::pillButton(
            menu,
            status,
            pillPos,
            {pillWidth, 26.f},
            [id = std::string(feature.id)](CCMenuItemSpriteExtra*) {
                for (auto const& item : catalog::features()) {
                    if (item.id != id)
                        continue;

                    catalog::applyToggle(item, !catalog::isToggleOn(item));
                    ModMenu::refreshCurrentTab();
                    return;
                }
            },
            active
        );
    }
    else {
        auto pill = ui::roundedRect(
            active ? theme::StatusOn : theme::StatusIdle,
            {pillWidth, 26.f},
            13.f
        );
        if (pill) {
            pill->setPosition(pillPos);
            auto caption = ui::label(status, 0.46f);
            if (caption) {
                caption->setTag(StatusLabelTag);
                caption->setPosition(pill->getContentSize() / 2.f);
                pill->addChild(caption);
            }
            card->addChild(pill, 4);
        }
    }

    if (catalog::isDirty(feature)) {
        ui::textButton(
            menu,
            "RESET",
            {
                size.width - pad - pillWidth - 40.f,
                size.height - 29.f
            },
            {58.f, 22.f},
            [id = std::string(feature.id)]() {
                for (auto const& item : catalog::features()) {
                    if (item.id != id)
                        continue;
                    catalog::resetFeature(item);
                    ModMenu::refreshCurrentTab();
                    return;
                }
>>>>>>> Stashed changes
            }
        );
    }

<<<<<<< Updated upstream
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
        if (!Popup::init(440.f, 285.f))
            return false;

        auto title = createMenuLabel("Noclip Settings", 0.62f);
        title->setPosition({m_size.width / 2.f, m_size.height - 30.f});
        m_mainLayer->addChild(title);

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        addToggleRow(menu, "Block Phasing", "noclip-phase-blocks", 220.f);
        addToggleRow(menu, "Slope Phasing", "noclip-phase-slopes", 188.f);
        addToggleRow(menu, "Spike Phasing", "noclip-phase-hazards", 156.f);

        auto modeLabel = createMenuLabel("Block Collision Mode", 0.42f);
        modeLabel->setAnchorPoint({0.f, 0.5f});
        modeLabel->setPosition({55.f, 119.f});
        m_mainLayer->addChild(modeLabel);

        m_blockModeButton = createBlockModeButton();
        auto modeButton = CCMenuItemSpriteExtra::create(
            m_blockModeButton,
            this,
            menu_selector(NoclipSettingsPopup::onBlockMode)
        );
        modeButton->setPosition({300.f, 119.f});
        menu->addChild(modeButton);

        auto hazardButtonSprite = ButtonSprite::create(
            "Configure Hazards",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.62f
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
        hazardButton->setPosition({m_size.width / 2.f, 78.f});
        menu->addChild(hazardButton);

        auto info = createMenuLabel(
            "Configure which hazard types noclip ignores.",
            0.3f
        );
        info->setPosition({m_size.width / 2.f, 51.f});
        info->setOpacity(180);
        m_mainLayer->addChild(info);

        auto closeSprite = ButtonSprite::create(
            "Back",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.7f
        );

        auto closeButton = CCMenuItemSpriteExtra::create(
            closeSprite,
            this,
            menu_selector(NoclipSettingsPopup::onClosePopup)
        );
        closeButton->setPosition({m_size.width / 2.f, 25.f});
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
            {370.f, y}
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
            0.54f
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
=======
    if (feature.hasSettings && feature.openSettings && feature.settingsEnabled) {
        auto hit = CCLayerColor::create(
            {0, 0, 0, 0},
            size.width - pad * 2.f - 100.f,
            64.f
        );
        if (hit) {
            hit->ignoreAnchorPointForPosition(false);
            hit->setAnchorPoint({0.5f, 0.5f});

            auto button = CCMenuItemExt::createSpriteExtra(
                hit,
                [open = feature.openSettings](CCMenuItemSpriteExtra*) {
                    if (open)
                        open();
                }
            );

            if (button) {
                ui::disablePressAnim(button);
                button->setPosition({
                    pad + (size.width - pad * 2.f - 100.f) / 2.f,
                    size.height - 40.f
                });
                menu->addChild(button);
            }
        }
    }

    return card;
}
>>>>>>> Stashed changes

} // namespace

bool ModMenu::init() {
    overlay::ensureSettingsFile();

<<<<<<< Updated upstream
    if (!Popup::init(460.f, 235.f))
        return false;

    createHeader();
    createTabBar();
    createContentPanel();

    auto resetSprite = ButtonSprite::create(
        "Set to Default",
        "goldFont.fnt",
        "GJ_button_01.png",
        0.5f
    );

    if (resetSprite) {
        auto resetButton = CCMenuItemSpriteExtra::create(
            resetSprite,
            this,
            menu_selector(ModMenu::onSetAllToDefault)
        );
        resetButton->setPosition({m_size.width - 70.f, 15.f});
        m_mainLayer->addChild(resetButton);
    }

=======
    float const aspect = overlay::windowAspect();
    auto const popupSize = overlay::popupSize(
        aspect < 0.95f ? theme::CompactWidth : aspect < 1.45f ? 960.f : theme::DesktopWidth,
        aspect < 0.95f ? theme::CompactHeight : aspect < 1.45f ? 700.f : theme::DesktopHeight
    );

    if (!Popup::init(popupSize.width, popupSize.height))
        return false;

    if (m_bgSprite)
        m_bgSprite->setVisible(false);

    if (m_closeBtn)
        m_closeBtn->setVisible(false);

    auto background = ui::roundedRect(theme::Shell, m_size, theme::ShellRadius);
    if (background) {
        background->setPosition(m_size / 2.f);
        m_mainLayer->addChild(background, -100);
    }

    createHeader();
    createTabBar();
    createContentPanel();
>>>>>>> Stashed changes
    return true;
}

void ModMenu::createHeader() {
<<<<<<< Updated upstream
    auto logo = CCSprite::create("PopupTitle.png"_spr);

    if (!logo) {
        log::error("Failed to load PopupTitle.png");
        return;
    }

    logo->setAnchorPoint({0.5f, 1.f});

    float desiredWidth = m_size.width * 0.65f;
    float scale = desiredWidth / logo->getContentSize().width;
    logo->setScale(scale);

    float overlap = logo->getScaledContentSize().height * 0.5f;

    logo->setPosition({
        m_size.width / 2.f,
        m_size.height + overlap
    });

    m_mainLayer->addChild(logo, 100);
}

void ModMenu::createTabBar() {
    auto strip = extension::CCScale9Sprite::create("square02_small.png");

    strip->setContentSize({
        m_size.width - 24.f,
        42.f
    });

    strip->setColor({55, 55, 55});
    strip->setOpacity(210);
    strip->setPosition({
        m_size.width / 2.f,
        m_size.height - 48.f
    });

    m_mainLayer->addChild(strip);

    auto menu = CCMenu::create();
    menu->setPosition({0, 0});
    m_mainLayer->addChild(menu);

    constexpr const char* tabs[] = {
        "Player",
        "Visuals",
        "Creator",
        "Misc",
        "Settings"
    };

    float x = 32.f;
    float y = strip->getPositionY();
    constexpr float gap = 10.f;

    for (int i = 0; i < 5; i++) {
        auto spr = ButtonSprite::create(
            tabs[i],
            "goldFont.fnt",
            "GJ_button_04.png",
            0.55f
        );

        spr->setScale(.72f);
=======
    bool const compact = overlay::isCompact();
    float const scale = overlay::desktopScale(m_size);

    float const headerWidth = compact
        ? m_size.width - 32.f * scale
        : m_size.width - 48.f * scale;
    float const headerHeight = (compact ? 66.f : 72.f) * scale;
    float const headerY = m_size.height - (compact ? 16.f : 20.f) * scale - headerHeight / 2.f;

    ui::panel(
        m_mainLayer,
        {m_size.width / 2.f, headerY},
        {headerWidth, headerHeight},
        theme::Header,
        compact ? theme::CompactHeaderRadius : theme::HeaderRadius
    );

    auto title = ui::label(
        "MOD UNIVERSAL",
        compact ? 0.82f * scale : 1.20f * scale
    );
    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({
            (compact ? 30.f : 48.f) * scale,
            compact ? headerY : m_size.height - 47.f * scale
        });
        title->limitLabelWidth(
            headerWidth * (compact ? 0.48f : 0.32f),
            title->getScale(),
            0.42f * scale
        );
        m_mainLayer->addChild(title, 10);
    }

    if (!compact) {
        auto version = ui::label("v0.2.0", 0.52f * scale);
        if (version) {
            version->setAnchorPoint({0.f, 0.5f});
            version->setPosition({
                48.f * scale,
                m_size.height - 70.f * scale
            });
            m_mainLayer->addChild(version, 10);
        }
    }

    auto headerMenu = ui::menuOn(m_mainLayer, 30);
    if (!headerMenu)
        return;

    float const closeSize = compact ? 28.f * scale : 34.f * scale;
    float const resetWidth = compact ? 96.f * scale : 126.f * scale;
    float const resetHeight = compact ? 28.f * scale : 34.f * scale;
    float const right = m_size.width / 2.f + headerWidth / 2.f - 16.f * scale;

    ui::textButton(
        headerMenu,
        "X",
        {right - closeSize / 2.f, headerY},
        {closeSize, closeSize},
        [this]() { this->onClose(nullptr); }
    );

    ui::textButton(
        headerMenu,
        "Set to Default",
        {right - closeSize - 8.f * scale - resetWidth / 2.f, headerY},
        {resetWidth, resetHeight},
        [this]() { this->onSetAllToDefault(nullptr); }
    );
}

void ModMenu::createTabBar() {
    m_tabMenu = ui::menuOn(m_mainLayer, 25);
    if (!m_tabMenu)
        return;

    auto const& tabs = catalog::tabs();
    bool const compact = overlay::isCompact();
    float const scale = overlay::desktopScale(m_size);

    if (compact) {
        float const navWidth = m_size.width - 32.f * scale;
        float const navHeight = 62.f * scale;
        float const navY = m_size.height - 123.f * scale;

        ui::panel(
            m_mainLayer,
            {m_size.width / 2.f, navY},
            {navWidth, navHeight},
            theme::Panel,
            theme::CompactHeaderRadius
        );

        int const count = static_cast<int>(tabs.size());
        float const gap = 7.f * scale;
        float const itemWidth = std::min(
            70.f * scale,
            (navWidth - 18.f * scale - gap * (count - 1)) / std::max(1, count)
        );
        float const itemHeight = 34.f * scale;
        float const total = count * itemWidth + (count - 1) * gap;
        float const startX = m_size.width / 2.f - total / 2.f + itemWidth / 2.f;

        for (int i = 0; i < count; ++i) {
            auto item = ui::roundedRect(
                i == m_currentTab ? theme::TabActive : theme::Card,
                {itemWidth, itemHeight},
                theme::CompactTabRadius * scale
            );
            if (!item)
                continue;

            auto caption = ui::label(tabs[i].label, 0.42f * scale);
            if (caption) {
                caption->setTag(TabLabelTag);
                caption->setPosition(item->getContentSize() / 2.f);
                caption->limitLabelWidth(itemWidth - 8.f, 0.42f * scale, 0.20f);
                item->addChild(caption);
            }

            auto button = CCMenuItemSpriteExtra::create(
                item,
                this,
                menu_selector(ModMenu::onTab)
            );
            if (!button)
                continue;

            ui::disablePressAnim(button);
            button->setTag(i);
            button->setPosition({startX + i * (itemWidth + gap), navY});
            m_tabMenu->addChild(button);
        }

        return;
    }

    float const sidebarLeft = 24.f * scale;
    float const sidebarWidth = theme::SidebarWidth * scale;
    float const sidebarHeight = 570.f * scale;
    float const sidebarBottom = 40.f * scale;

    ui::panel(
        m_mainLayer,
        {sidebarLeft + sidebarWidth / 2.f, sidebarBottom + sidebarHeight / 2.f},
        {sidebarWidth, sidebarHeight},
        theme::Panel
    );

    auto modules = ui::label("MODULES", 0.50f * scale);
    if (modules) {
        modules->setAnchorPoint({0.f, 0.5f});
        modules->setPosition({
            45.f * scale,
            m_size.height - 137.5f * scale
        });
        m_mainLayer->addChild(modules, 5);
    }

    auto scrollHeight = sidebarHeight - 52.f * scale;
    auto scroll = ScrollLayer::create(
        {sidebarWidth - 16.f * scale, scrollHeight},
        true,
        true
    );
    scroll->setPosition({
        sidebarLeft + 8.f * scale,
        sidebarBottom + 12.f * scale
    });
    m_mainLayer->addChild(scroll, 8);

    auto content = scroll->m_contentLayer;
    content->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAutoGrowAxis(scrollHeight)
            ->setGap(theme::TabGap * scale)
            ->setAxisAlignment(AxisAlignment::Start)
            ->setCrossAxisAlignment(AxisAlignment::Center)
    );

    float const buttonWidth = theme::TabWidth * scale;
    float const buttonHeight = theme::TabHeight * scale;
    auto tabHitMenu = ui::menuOn(content, 20);
    m_tabMenu = tabHitMenu;

    for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
        auto item = ui::roundedRect(
            i == m_currentTab ? theme::TabActive : theme::Card,
            {buttonWidth, buttonHeight},
            theme::TabRadius
        );
        if (!item)
            continue;

        item->setContentSize({buttonWidth, buttonHeight});

        auto caption = ui::label(tabs[i].label, 0.62f * scale);
        if (caption) {
            caption->setTag(TabLabelTag);
            caption->setAnchorPoint({0.f, 0.5f});
            caption->setPosition({15.f * scale, buttonHeight / 2.f});
            item->addChild(caption);
        }
>>>>>>> Stashed changes

        auto btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            menu_selector(ModMenu::onTab)
        );
<<<<<<< Updated upstream

        btn->setTag(i);
        btn->setPosition({
            x + spr->getScaledContentSize().width / 2.f,
            y
        });

        if (i == 0)
            spr->setColor({120, 255, 120});

        menu->addChild(btn);
        x += spr->getScaledContentSize().width + gap;
=======
        if (!button)
            continue;

        ui::disablePressAnim(button);
        button->setTag(i);
        button->setContentSize({buttonWidth, buttonHeight});
        content->addChild(button);
>>>>>>> Stashed changes
    }

    content->updateLayout();
    scroll->scrollToTop();
}

void ModMenu::createContentPanel() {
<<<<<<< Updated upstream
    m_contentPanel = CCNode::create();

    m_contentPanel->setContentSize({
        m_size.width - 40.f,
        125.f
    });

    m_contentPanel->setPosition({
        20.f,
        15.f
    });

    m_mainLayer->addChild(m_contentPanel);
    onTab(nullptr);
=======
    bool const compact = overlay::isCompact();
    float const scale = overlay::desktopScale(m_size);

    m_contentPanel = CCNode::create();
    if (!m_contentPanel)
        return;

    if (compact) {
        float const width = m_size.width - 32.f * scale;
        float const height = 574.f * scale;
        m_contentPanel->setContentSize({width, height});
        m_contentPanel->setPosition({16.f * scale, 16.f * scale});
    }
    else {
        float const panelWidth = 924.f * scale;
        float const panelHeight = 570.f * scale;
        m_contentPanel->setContentSize({panelWidth, panelHeight});
        m_contentPanel->setPosition({220.f * scale, 40.f * scale});
    }

    m_mainLayer->addChild(m_contentPanel);
    fillContent();
}

void ModMenu::fillContent() {
    if (!m_contentPanel)
        return;

    m_contentPanel->removeAllChildrenWithCleanup(true);
    m_contentScroll = nullptr;

    auto const size = m_contentPanel->getContentSize();
    bool const compact = overlay::isCompact();
    float const scale = overlay::desktopScale(m_size);

    ui::panel(
        m_contentPanel,
        size / 2.f,
        size,
        theme::Panel,
        compact ? theme::CompactHeaderRadius : theme::PanelRadius
    );

    auto const& tab = catalog::tabs()[static_cast<size_t>(m_currentTab)];

    auto heading = ui::label(tab.heading, compact ? 0.78f * scale : 0.92f * scale);
    if (heading) {
        heading->setAnchorPoint({0.f, 0.5f});
        heading->setPosition({
            compact ? 16.f * scale : 26.f * scale,
            size.height - (compact ? 24.f : 28.f) * scale
        });
        m_contentPanel->addChild(heading, 8);
    }

    auto subtitle = ui::label(tab.subtitle, compact ? 0.42f * scale : 0.48f * scale);
    if (subtitle) {
        subtitle->setAnchorPoint({0.f, 0.5f});
        subtitle->setPosition({
            compact ? 16.f * scale : 26.f * scale,
            size.height - (compact ? 44.f : 52.f) * scale
        });
        subtitle->limitLabelWidth(size.width - 40.f * scale, subtitle->getScale(), 0.28f);
        m_contentPanel->addChild(subtitle, 8);
    }

    float const headerReserve = compact ? 62.f * scale : 86.f * scale;
    auto const scrollSize = CCSize{
        size.width,
        std::max(80.f, size.height - headerReserve)
    };

    m_contentScroll = ScrollLayer::create(scrollSize, true, true);
    m_contentScroll->setPosition({0.f, 0.f});
    m_contentPanel->addChild(m_contentScroll, 5);

    auto features = catalog::featuresFor(catalog::tabAt(m_currentTab));
    int const columns = (!compact && size.width >= 560.f) ? 2 : 1;
    float const pad = compact ? 16.f * scale : 26.f * scale;
    float const gapX = compact ? 12.f * scale : theme::CardGapX * scale;
    float const gapY = compact ? 12.f * scale : theme::CardGapY * scale;
    float const usable = std::max(120.f, scrollSize.width - pad * 2.f);
    float const cardWidth = (usable - gapX * (columns - 1)) / static_cast<float>(columns);
    float const cardHeight = theme::CardHeight * (compact ? std::min(1.f, scale * 1.05f) : scale);

    auto content = m_contentScroll->m_contentLayer;
    content->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAutoGrowAxis(scrollSize.height)
            ->setGap(gapY)
            ->setAxisAlignment(AxisAlignment::Start)
            ->setCrossAxisAlignment(AxisAlignment::Center)
    );

    for (int i = 0; i < static_cast<int>(features.size()); i += columns) {
        int const count = std::min(
            columns,
            static_cast<int>(features.size()) - i
        );
        auto row = CCNode::create();
        float const rowWidth = count * cardWidth + (count - 1) * gapX;
        row->setContentSize({rowWidth, cardHeight});
        row->ignoreAnchorPointForPosition(false);
        row->setAnchorPoint({0.5f, 0.5f});
        row->setLayout(
            RowLayout::create()
                ->setGap(gapX)
                ->setAutoScale(false)
                ->setAxisAlignment(AxisAlignment::Start)
                ->setCrossAxisAlignment(AxisAlignment::Center)
        );

        for (int column = 0; column < count; ++column) {
            if (auto card = createFeatureCard(*features[i + column], {cardWidth, cardHeight}))
                row->addChild(card);
        }

        row->updateLayout();
        content->addChild(row);
    }

    content->updateLayout();
    m_contentScroll->scrollToTop();
}

void ModMenu::highlightTabs() {
    if (!m_tabMenu)
        return;

    for (auto* child : CCArrayExt<CCNode*>(m_tabMenu->getChildren())) {
        auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
        if (!button)
            continue;

        auto background = typeinfo_cast<CCLayerColor*>(button->getNormalImage());
        if (!background)
            continue;

        bool const selected = button->getTag() == m_currentTab;
        ui::setRoundedRectColor(
            background,
            selected ? theme::TabActive : theme::Card
        );

        if (auto caption = typeinfo_cast<CCLabelBMFont*>(
            background->getChildByTag(TabLabelTag)
        )) {
            caption->setColor(theme::Text);
        }
    }
>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
    // F3 acts like a back action: close the deepest open submenu first.
    if (closeTopNestedPopup())
=======
    if (overlay::closeTop())
>>>>>>> Stashed changes
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
    overlay::closeAll();
    overlay::saveModData();
    s_instance = nullptr;
    Popup::onClose(sender);
}

void ModMenu::onTab(CCObject* sender) {
    if (sender) {
        auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
<<<<<<< Updated upstream
        tab = btn->getTag();
        m_currentTab = tab;

        log::info("Clicked tab {}", tab);

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
                    : ccColor3B{255, 255, 255}
            );
        }
    }

    if (!m_contentPanel)
        return;

    m_contentPanel->removeAllChildrenWithCleanup(true);

    if (tab == 0) {
        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_contentPanel->addChild(menu);

        auto noclipLabel = createMenuLabel("Noclip", 0.5f);
        noclipLabel->setAnchorPoint({0.f, 0.5f});
        noclipLabel->setPosition({105.f, 62.f});
        m_contentPanel->addChild(noclipLabel);

        auto gearSprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png");

        auto noclipDefaultButton = createHackDefaultButton(
            menu,
            NoclipSettingKeys,
            {280.f, 62.f}
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
                hasNoclipChanges ? 235.f : 275.f,
                62.f
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
            {330.f, 62.f}
        );
        s_noclipCheckboxButton = noclipToggle;

        if (!noclipToggle)
            log::warn("Could not create noclip checkbox");

        return;
    }

    auto button = ButtonSprite::create(
        fmt::format("TEST {}", tab + 1).c_str(),
        "goldFont.fnt",
        "GJ_button_01.png",
        0.7f
    );

    auto testButton = CCMenuItemSpriteExtra::create(
        button,
        this,
        nullptr
    );

    auto menu = CCMenu::create();
    menu->setPosition(m_contentPanel->getContentSize() / 2.f);
    menu->addChild(testButton);
    m_contentPanel->addChild(menu);
}

void ModMenu::onNoclipSettings(CCObject*) {
    openNoclipSettings();
=======
        m_currentTab = std::clamp(btn->getTag(), 0, catalog::tabCount() - 1);
        highlightTabs();
    }

    fillContent();
>>>>>>> Stashed changes
}

void ModMenu::refreshCurrentTab() {
    if (s_instance)
        s_instance->fillContent();
}

void ModMenu::onSetAllToDefault(CCObject*) {
    createQuickPopup(
        "Set to Default",
        "This will change all previously changed <cr>settings</c> "
        "back to their <cy>default values</c>. This cannot be undone.",
        "Cancel",
        "Reset",
        [](auto, bool btn2) {
            if (!btn2)
                return;

            catalog::resetAllFeatures();
            ModMenu::refreshCurrentTab();
        }
    );
}

void ModMenu::openNoclipSettings() {
    mu::ui::openNoclipSettings();
}

void ModMenu::openNoclipHazardSettings() {
    mu::ui::openNoclipHazardSettings();
}
