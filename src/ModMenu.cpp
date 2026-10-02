#include "../include/ModMenu.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
#include <functional>
#include <vector>

namespace {

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

    auto sprite = createNoclipCheckboxSprite(enabled);

    if (!sprite) {
        log::error("Failed to create Noclip checkbox sprite");
        return;
    }

    button->setNormalImage(sprite);
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

            mod->setSettingValue<bool>(key, next);
            refreshNoclipCheckbox(item, next);
            saveNoclipButtonSettings();

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
    Popup* popup = nullptr;
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

        if (!entry.popup || !entry.popup->getParent())
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

        addToggleRow(menu, "Spikes", "noclip-hazard-spikes", 0, 225.f);
        addToggleRow(menu, "Ground / Edge Spikes", "noclip-hazard-ground-spikes", 1, 192.f);
        addToggleRow(menu, "Sawblades", "noclip-hazard-saws", 2, 159.f);
        addToggleRow(menu, "Pits", "noclip-hazard-pits", 3, 126.f);
        addToggleRow(menu, "Animated Hazards", "noclip-hazard-animated", 4, 93.f);
        addToggleRow(menu, "Other Hazards", "noclip-hazard-other", 5, 60.f);

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
        int tag,
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

        if (toggle)
            toggle->setTag(tag);
        else
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
        if (!Popup::init(440.f, 285.f))
            return false;

        auto title = createMenuLabel("Noclip Settings", 0.62f);
        title->setPosition({m_size.width / 2.f, m_size.height - 30.f});
        m_mainLayer->addChild(title);

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(menu);

        addToggleRow(menu, "Phase Through Blocks", "noclip-phase-blocks", 0, 220.f);
        addToggleRow(menu, "Phase Through Slopes", "noclip-phase-slopes", 1, 188.f);
        addToggleRow(menu, "Phase Through Hazards", "noclip-phase-hazards", 2, 156.f);

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
        int tag,
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

        if (toggle)
            toggle->setTag(tag);
        else
            log::warn("Could not create noclip checkbox for {}", settingKey);

        return toggle;
    }

    ButtonSprite* createBlockModeButton() {
        auto mode = Mod::get()->getSettingValue<std::string>("noclip-block-mode");
        auto text = mode == "no-touch"
            ? "No Block Touch"
            : "Safe Block Touch";

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
                    ? "No Block Touch"
                    : "Safe Block Touch"
            );
        }
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
    if (!Popup::init(460.f, 235.f))
        return false;

    createHeader();
    createTabBar();
    createContentPanel();

    return true;
}

void ModMenu::createHeader() {
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

        auto btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            menu_selector(ModMenu::onTab)
        );

        btn->setTag(i);
        btn->setPosition({
            x + spr->getScaledContentSize().width / 2.f,
            y
        });

        if (i == 0)
            spr->setColor({120, 255, 120});

        menu->addChild(btn);
        x += spr->getScaledContentSize().width + gap;
    }
}

void ModMenu::createContentPanel() {
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
    int tab = 0;

    if (sender) {
        auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
        tab = btn->getTag();

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

        if (gearSprite) {
            gearSprite->setScale(0.72f);

            auto gearButton = CCMenuItemSpriteExtra::create(
                gearSprite,
                this,
                menu_selector(ModMenu::onNoclipSettings)
            );
            gearButton->setPosition({255.f, 62.f});
            menu->addChild(gearButton);
        }
        else {
            log::warn("Could not load noclip settings gear sprite");
        }

        auto noclipToggle = createNoclipCheckbox(
            menu,
            "noclip-enabled",
            {305.f, 62.f}
        );

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
}

void ModMenu::openNoclipSettings() {
    if (auto popup = NoclipSettingsPopup::create())
        popup->showAndRegister();
}

void ModMenu::openNoclipHazardSettings() {
    if (auto popup = NoclipHazardPopup::create())
        popup->showAndRegister();
}
