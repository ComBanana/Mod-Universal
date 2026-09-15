#include "../include/ModMenu.hpp"

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

    // Background strip
    auto strip = extension::CCScale9Sprite::create("square02_small.png");

    strip->setContentSize({
        m_size.width - 24.f,
        42.f
    });

    strip->setColor({55, 55, 55});      // dark gray
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

    for (auto const& tab : tabs) {

        auto spr = ButtonSprite::create(
            tab,
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

        btn->setPosition({
            x + spr->getScaledContentSize().width / 2.f,
            y
        });

        menu->addChild(btn);

        x += spr->getScaledContentSize().width + gap;
    }
}

void ModMenu::createContentPanel() {

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
}