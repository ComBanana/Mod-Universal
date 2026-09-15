#include "../include/ModMenu.hpp"


static bool s_noclipEnabled = false;

bool ModMenu::isNoclipEnabled() {
    return s_noclipEnabled;
}

void ModMenu::setNoclipEnabled(bool enabled) {
    s_noclipEnabled = enabled;

    log::info(
        "Noclip {}",
        enabled ? "enabled" : "disabled"
    );
}

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
    float scale =
        desiredWidth /
        logo->getContentSize().width;

    logo->setScale(scale);

    float overlap = logo->getScaledContentSize().height * 0.5f;

    logo->setPosition({
        m_size.width / 2.f,
        m_size.height + overlap
    });

    m_mainLayer->addChild(logo, 100);
}

void ModMenu::createTabBar() {
    auto strip = extension::CCScale9Sprite::create(
        "square02_small.png"
    );

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

        // Player starts selected.
        if (i == 0) {
            spr->setColor({120, 255, 120});
        }

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

    // Start on Player / Test 1.
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
    if (s_instance) {
        s_instance->removeFromParentAndCleanup(true);
        s_instance = nullptr;
        return;
    }

    s_instance = create();

    if (s_instance)
        s_instance->show();
}

void ModMenu::onClose(CCObject* sender) {
    log::info("Popup closed!");

    s_instance = nullptr;

    Popup::onClose(sender);
}

void ModMenu::onTab(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);

    int tab = btn->getTag();

    log::info("Clicked tab {}", tab);

    // Light up the selected tab and restore all others.
    auto menu = static_cast<CCMenu*>(btn->getParent());

    for (auto* child : CCArrayExt<CCNode*>(menu->getChildren())) {
        auto otherButton =
            typeinfo_cast<CCMenuItemSpriteExtra*>(child);

        if (!otherButton)
            continue;

        auto sprite = otherButton->getNormalImage();

        if (!sprite)
            continue;

        sprite->setColor(
            otherButton == btn
                ? ccColor3B{120, 255, 120}
                : ccColor3B{255, 255, 255}
        );
    }

    if (!m_contentPanel)
        return;

    m_contentPanel->removeAllChildrenWithCleanup(true);

    if (tab == 0) {
        auto menu = CCMenu::create();
        menu->setPosition(
            m_contentPanel->getContentSize() / 2.f
        );

        auto buttonSprite = ButtonSprite::create(
            ModMenu::isNoclipEnabled()
                ? "Noclip: ON"
                : "Noclip: OFF",
            "goldFont.fnt",
            "GJ_button_01.png",
            0.7f
        );

        auto noclipButton = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(ModMenu::onNoclip)
        );

        menu->addChild(noclipButton);
        m_contentPanel->addChild(menu);

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
    menu->setPosition(
        m_contentPanel->getContentSize() / 2.f
    );

    menu->addChild(testButton);
    m_contentPanel->addChild(menu);
}

