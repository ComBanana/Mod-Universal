#include "../../include/ui/NoclipPopups.hpp"
#include "../../include/ui/Overlay.hpp"
#include "../../include/ui/Theme.hpp"
#include "../../include/ui/Widgets.hpp"
#include "../../include/ModMenu.hpp"

using namespace geode::prelude;
using namespace mu;

namespace {

class ThemedPopup : public Popup {
protected:
    bool setupShell(CCSize size, char const* title, char const* subtitle) {
        if (!Popup::init(size.width, size.height))
            return false;

        if (m_bgSprite)
            m_bgSprite->setVisible(false);

        if (m_closeBtn) {
            m_closeBtn->setAnchorPoint({0.5f, 0.5f});
            m_closeBtn->setPosition({
                m_size.width - 22.f,
                m_size.height - 22.f
            });
            m_closeBtn->setScale(0.62f);
        }

        ui::panel(
            m_mainLayer,
            m_size / 2.f,
            m_size,
            theme::Shell,
            theme::ShellRadius
        )->setZOrder(-100);

        ui::panel(
            m_mainLayer,
            {m_size.width / 2.f, m_size.height - 36.f},
            {m_size.width - 32.f, 52.f},
            theme::Header,
            theme::HeaderRadius
        );

        auto heading = ui::label(title, m_size.width < 440.f ? 0.56f : 0.64f);
        if (heading) {
            heading->setAnchorPoint({0.f, 0.5f});
            heading->setPosition({28.f, m_size.height - 28.f});
            m_mainLayer->addChild(heading, 8);
        }

        if (subtitle) {
            auto sub = ui::mutedLabel(
                subtitle,
                m_size.width < 440.f ? 0.27f : 0.31f
            );
            if (sub) {
                sub->setAnchorPoint({0.f, 0.5f});
                sub->setPosition({28.f, m_size.height - 46.f});
                m_mainLayer->addChild(sub, 8);
            }
        }

        return true;
    }

    void addToggleRow(
        CCMenu* menu,
        char const* labelText,
        char const* settingKey,
        float y
    ) {
        auto const rowSize = CCSize{m_size.width - 48.f, 42.f};
        ui::panel(
            m_mainLayer,
            {m_size.width / 2.f, y},
            rowSize,
            theme::Card,
            theme::CardRadius
        );

        auto caption = ui::label(
            labelText,
            m_size.width < 440.f ? 0.42f : 0.48f
        );
        if (caption) {
            caption->setAnchorPoint({0.f, 0.5f});
            caption->setPosition({36.f, y});
            m_mainLayer->addChild(caption, 6);
        }

        bool const enabled = Mod::get()->getSettingValue<bool>(settingKey);
        ui::pillButton(
            menu,
            enabled ? "ON" : "OFF",
            {m_size.width - 70.f, y},
            {72.f, 26.f},
            [settingKey](CCMenuItemSpriteExtra* item) {
                bool const next = !Mod::get()->getSettingValue<bool>(settingKey);
                if (std::string(settingKey) == "noclip-enabled")
                    ModMenu::setNoclipEnabled(next);
                else
                    Mod::get()->setSettingValue<bool>(settingKey, next);

                overlay::saveModData();
                ui::setPillAppearance(item, next ? "ON" : "OFF", next);
                ModMenu::refreshCurrentTab();
            },
            enabled
        );
    }

    void onClosePopup(CCObject*) {
        overlay::unregisterPopup(this);
        this->onClose(nullptr);
    }

    void keyBackClicked() override {
        onClosePopup(nullptr);
    }

    void showAndRegister() {
        this->show();
        overlay::registerPopup(this, [this]() {
            this->onClosePopup(nullptr);
        });
    }
};

class NoclipHazardPopup : public ThemedPopup {
protected:
    bool init() {
        auto const screen = CCDirector::sharedDirector()->getWinSize();
        auto const size = overlay::popupSize(
            560.f,
            screen.width < screen.height ? 620.f : 430.f
        );

        if (!setupShell(
            size,
            "Spike Phasing",
            "Choose which hazard categories are ignored by Noclip."
        )) {
            return false;
        }

        auto menu = ui::menuOn(m_mainLayer);
        float const top = m_size.height - 96.f;
        float const gap = std::min(48.f, std::max(40.f, (top - 70.f) / 5.f));

        addToggleRow(menu, "Spikes", "noclip-hazard-spikes", top);
        addToggleRow(menu, "Ground / Edge Spikes", "noclip-hazard-ground-spikes", top - gap);
        addToggleRow(menu, "Sawblades", "noclip-hazard-saws", top - gap * 2.f);
        addToggleRow(menu, "Pits", "noclip-hazard-pits", top - gap * 3.f);
        addToggleRow(menu, "Animated Hazards", "noclip-hazard-animated", top - gap * 4.f);
        addToggleRow(menu, "Other Hazards", "noclip-hazard-other", top - gap * 5.f);

        ui::textButton(
            menu,
            "Back",
            {m_size.width / 2.f, 28.f},
            {126.f, 34.f},
            [this]() { this->onClosePopup(nullptr); }
        );

        return true;
    }

public:
    static NoclipHazardPopup* create() {
        auto ret = new NoclipHazardPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    void present() {
        showAndRegister();
    }
};

class NoclipSettingsPopup : public ThemedPopup {
protected:
    CCLayerColor* m_blockModeBackground = nullptr;
    CCLabelBMFont* m_blockModeLabel = nullptr;

    bool init() {
        auto const screen = CCDirector::sharedDirector()->getWinSize();
        auto const size = overlay::popupSize(
            560.f,
            screen.width < screen.height ? 620.f : 450.f
        );

        if (!setupShell(
            size,
            "Noclip",
            "Control how Noclip interacts with the level."
        )) {
            return false;
        }

        auto menu = ui::menuOn(m_mainLayer);
        float const top = m_size.height - 102.f;

        addToggleRow(menu, "Block Phasing", "noclip-phase-blocks", top);
        addToggleRow(menu, "Slope Phasing", "noclip-phase-slopes", top - 50.f);
        addToggleRow(menu, "Spike Phasing", "noclip-phase-hazards", top - 100.f);

        ui::panel(
            m_mainLayer,
            {m_size.width / 2.f, top - 150.f},
            {m_size.width - 48.f, 42.f},
            theme::Card,
            theme::CardRadius
        );

        auto collisionLabel = ui::label(
            "Block Collision",
            m_size.width < 440.f ? 0.42f : 0.48f
        );
        if (collisionLabel) {
            collisionLabel->setAnchorPoint({0.f, 0.5f});
            collisionLabel->setPosition({36.f, top - 150.f});
            m_mainLayer->addChild(collisionLabel, 6);
        }

        bool const noTouch =
            Mod::get()->getSettingValue<std::string>("noclip-block-mode") == "no-touch";

        m_blockModeBackground = ui::roundedRect(
            noTouch ? theme::StatusOn : theme::Button,
            {132.f, 26.f},
            13.f
        );

        if (m_blockModeBackground) {
            m_blockModeLabel = ui::label(
                noTouch ? "No Hitbox" : "Standard",
                0.40f,
                theme::Text
            );

            if (m_blockModeLabel) {
                m_blockModeLabel->setPosition({66.f, 13.f});
                m_blockModeBackground->addChild(m_blockModeLabel);
            }

            auto modeButton = CCMenuItemExt::createSpriteExtra(
                m_blockModeBackground,
                [this](CCMenuItemSpriteExtra*) {
                    this->onBlockMode();
                }
            );

            if (modeButton) {
                ui::disablePressAnim(modeButton);
                modeButton->setPosition({m_size.width - 96.f, top - 150.f});
                menu->addChild(modeButton);
            }
        }

        auto hazard = ui::textButton(
            menu,
            "Configure Hazards",
            {m_size.width / 2.f, top - 210.f},
            {190.f, 34.f},
            [this]() { this->onHazardSettings(); },
            theme::Button,
            theme::Muted
        );

        if (hazard)
            hazard->setEnabled(false);

        auto info = ui::mutedLabel(
            "Hazard categories will be configurable later.",
            0.29f
        );
        if (info) {
            info->setAnchorPoint({0.5f, 0.5f});
            info->setPosition({m_size.width / 2.f, top - 242.f});
            m_mainLayer->addChild(info);
        }

        ui::textButton(
            menu,
            "Back",
            {m_size.width / 2.f, 28.f},
            {126.f, 34.f},
            [this]() { this->onClosePopup(nullptr); }
        );

        return true;
    }

    void onBlockMode() {
        auto current = Mod::get()->getSettingValue<std::string>("noclip-block-mode");
        std::string const next = current == "no-touch" ? "safe-touch" : "no-touch";
        Mod::get()->setSettingValue<std::string>("noclip-block-mode", next);
        overlay::saveModData();

        bool const noTouch = next == "no-touch";
        ui::setRoundedRectColor(
            m_blockModeBackground,
            noTouch ? theme::StatusOn : theme::Button,
            13.f
        );

        if (m_blockModeLabel)
            m_blockModeLabel->setString(noTouch ? "No Hitbox" : "Standard");

        ModMenu::refreshCurrentTab();
    }

    void onHazardSettings() {
        if (auto popup = NoclipHazardPopup::create())
            popup->present();
    }

public:
    static NoclipSettingsPopup* create() {
        auto ret = new NoclipSettingsPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    void present() {
        showAndRegister();
    }
};

} // namespace

namespace mu::ui {

void openNoclipSettings() {
    if (auto popup = NoclipSettingsPopup::create())
        popup->present();
}

void openNoclipHazardSettings() {
    if (auto popup = NoclipHazardPopup::create())
        popup->present();
}

} // namespace mu::ui
