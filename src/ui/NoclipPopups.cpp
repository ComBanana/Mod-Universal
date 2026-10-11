#include "../../include/ui/NoclipPopups.hpp"
#include "../../include/ui/Overlay.hpp"
#include "../../include/ui/Theme.hpp"
#include "../../include/ui/Widgets.hpp"
#include "../../include/ModMenu.hpp"

#include <Geode/ui/Layout.hpp>
#include <Geode/ui/ScrollLayer.hpp>

#include <algorithm>
#include <string>

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
        if (m_closeBtn)
            m_closeBtn->setVisible(false);

        ui::panel(
            m_mainLayer,
            m_size / 2.f,
            m_size,
            theme::Shell,
            theme::ShellRadius
        )->setZOrder(-100);

        // Compact header and matching close control.
        ui::panel(
            m_mainLayer,
            {m_size.width / 2.f, m_size.height - 32.f},
            {m_size.width - 28.f, 48.f},
            theme::Header,
            theme::HeaderRadius
        );

        auto heading = ui::label(title, m_size.width < 400.f ? 0.56f : 0.64f);
        if (heading) {
            heading->setAnchorPoint({0.f, 0.5f});
            heading->setPosition({22.f, m_size.height - 22.f});
            m_mainLayer->addChild(heading, 8);
        }

        if (subtitle) {
            auto sub = ui::mutedLabel(
                subtitle,
                m_size.width < 400.f ? 0.27f : 0.31f
            );
            if (sub) {
                sub->setAnchorPoint({0.f, 0.5f});
                sub->setPosition({22.f, m_size.height - 41.f});
                m_mainLayer->addChild(sub, 8);
            }
        }

        auto headerMenu = ui::menuOn(m_mainLayer, 30);
        if (headerMenu) {
            ui::textButton(
                headerMenu,
                "X",
                {m_size.width - 29.f, m_size.height - 31.f},
                {26.f, 26.f},
                [this]() { this->onClosePopup(nullptr); }
            );
        }

        return true;
    }

    CCNode* createToggleRow(
        CCSize rowSize,
        char const* labelText,
        char const* settingKey
    ) {
        auto row = CCNode::create();
        if (!row)
            return nullptr;

        row->setContentSize(rowSize);
        row->ignoreAnchorPointForPosition(false);
        row->setAnchorPoint({0.5f, 0.5f});

        auto background = ui::roundedRect(
            theme::Card,
            rowSize,
            8.f
        );
        if (background) {
            background->setPosition(rowSize / 2.f);
            row->addChild(background, 0);
        }

        auto caption = ui::label(
            labelText,
            rowSize.width < 360.f ? 0.42f : 0.48f
        );
        if (caption) {
            caption->setAnchorPoint({0.f, 0.5f});
            caption->setPosition({16.f, rowSize.height / 2.f});
            row->addChild(caption, 6);
        }

        auto menu = ui::menuOn(row, 12);
        bool const enabled = Mod::get()->getSettingValue<bool>(settingKey);
        ui::pillButton(
            menu,
            enabled ? "ON" : "OFF",
            {rowSize.width - 48.f, rowSize.height / 2.f},
            {62.f, 22.f},
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

        return row;
    }

    CCNode* createCategoryRow(CCSize rowSize, char const* labelText) {
        auto row = CCNode::create();
        if (!row)
            return nullptr;

        row->setContentSize(rowSize);
        row->ignoreAnchorPointForPosition(false);
        row->setAnchorPoint({0.5f, 0.5f});

        auto caption = ui::label(labelText, 0.42f, theme::Muted);
        if (caption) {
            caption->setAnchorPoint({0.f, 0.5f});
            caption->setPosition({14.f, rowSize.height / 2.f});
            row->addChild(caption, 2);
        }

        return row;
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

class NoclipSettingsPopup : public ThemedPopup {
protected:
    CCLayerColor* m_blockModeBackground = nullptr;
    CCLabelBMFont* m_blockModeLabel = nullptr;

    CCNode* createBlockModeRow(CCSize rowSize) {
        auto row = CCNode::create();
        if (!row)
            return nullptr;

        row->setContentSize(rowSize);
        row->ignoreAnchorPointForPosition(false);
        row->setAnchorPoint({0.5f, 0.5f});

        auto background = ui::roundedRect(theme::Card, rowSize, 8.f);
        if (background) {
            background->setPosition(rowSize / 2.f);
            row->addChild(background, 0);
        }

        auto caption = ui::label(
            "Block Collision",
            rowSize.width < 360.f ? 0.42f : 0.48f
        );
        if (caption) {
            caption->setAnchorPoint({0.f, 0.5f});
            caption->setPosition({16.f, rowSize.height / 2.f});
            row->addChild(caption, 6);
        }

        bool const noTouch =
            Mod::get()->getSettingValue<std::string>("noclip-block-mode") == "no-touch";

        m_blockModeBackground = ui::roundedRect(
            noTouch ? theme::StatusOn : theme::Button,
            {108.f, 22.f},
            11.f
        );

        if (m_blockModeBackground) {
            m_blockModeLabel = ui::label(
                noTouch ? "No Hitbox" : "Standard",
                0.40f,
                theme::Text
            );

            if (m_blockModeLabel) {
                m_blockModeLabel->setPosition({54.f, 11.f});
                m_blockModeBackground->addChild(m_blockModeLabel);
            }

            auto menu = ui::menuOn(row, 12);
            auto modeButton = CCMenuItemExt::createSpriteExtra(
                m_blockModeBackground,
                [this](CCMenuItemSpriteExtra*) {
                    this->onBlockMode();
                }
            );

            if (modeButton) {
                ui::disablePressAnim(modeButton);
                modeButton->setPosition({
                    rowSize.width - 66.f,
                    rowSize.height / 2.f
                });
                menu->addChild(modeButton);
            }
        }

        return row;
    }

    bool init() {
        bool const compact = overlay::isCompact();
        auto const size = overlay::popupSize(
            compact ? 360.f : 460.f,
            compact ? 460.f : 350.f
        );

        if (!setupShell(
            size,
            "Noclip",
            "Control how Noclip interacts with the level."
        )) {
            return false;
        }

        // All controls live in one scrollable list. Hazard-specific toggles
        // remain in this list under their own category instead of a second popup.
        CCSize const scrollSize = {
            m_size.width - 32.f,
            m_size.height - 84.f
        };

        auto scroll = ScrollLayer::create(scrollSize, true, true);
        if (!scroll)
            return false;

        scroll->setPosition({16.f, 14.f});
        m_mainLayer->addChild(scroll, 5);

        auto content = scroll->m_contentLayer;
        content->setLayout(
            ColumnLayout::create()
                ->setAxisReverse(true)
                ->setAutoGrowAxis(scrollSize.height)
                ->setGap(6.f)
                ->setAxisAlignment(AxisAlignment::Start)
                ->setCrossAxisAlignment(AxisAlignment::Center)
        );

        CCSize const rowSize = {
            scrollSize.width - 8.f,
            32.f
        };

        auto addToggle = [&](char const* labelText, char const* settingKey) {
            if (auto row = createToggleRow(rowSize, labelText, settingKey))
                content->addChild(row);
        };

        addToggle("Block Phasing", "noclip-phase-blocks");
        addToggle("Slope Phasing", "noclip-phase-slopes");
        addToggle("Spike Phasing", "noclip-phase-hazards");

        if (auto row = createBlockModeRow(rowSize))
            content->addChild(row);

        if (auto category = createCategoryRow(
            {rowSize.width, 22.f},
            "HAZARD CATEGORIES"
        )) {
            content->addChild(category);
        }

        addToggle("Spikes", "noclip-hazard-spikes");
        addToggle("Ground / Edge Spikes", "noclip-hazard-ground-spikes");
        addToggle("Sawblades", "noclip-hazard-saws");
        addToggle("Pits", "noclip-hazard-pits");
        addToggle("Animated Hazards", "noclip-hazard-animated");
        addToggle("Other Hazards", "noclip-hazard-other");

        content->updateLayout();
        scroll->scrollToTop();
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
            11.f
        );

        if (m_blockModeLabel)
            m_blockModeLabel->setString(noTouch ? "No Hitbox" : "Standard");

        ModMenu::refreshCurrentTab();
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

// Kept for compatibility with existing callers; hazard options now live in
// the main scrollable Noclip settings panel.
void openNoclipHazardSettings() {
    openNoclipSettings();
}

} // namespace mu::ui
