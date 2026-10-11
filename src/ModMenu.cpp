#include "../include/ModMenu.hpp"
#include "../include/ui/Catalog.hpp"
#include "../include/ui/NoclipPopups.hpp"
#include "../include/ui/Overlay.hpp"
#include "../include/ui/Theme.hpp"
#include "../include/ui/Widgets.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/modify/PlayLayer.hpp>
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

    float const scale = size.height / theme::CardHeight;
    auto background = ui::roundedRect(theme::Card, size, theme::CardRadius * scale);
    if (background) {
        background->setPosition(size / 2.f);
        card->addChild(background, 0);
    }

    float const pad = 18.f * scale;

    auto title = ui::label(feature.title, 0.54f * scale);
    if (title) {
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({pad, size.height - 24.f * scale});
        title->limitLabelWidth(
            size.width - pad * 2.f - 88.f * scale,
            title->getScale(),
            0.38f * scale
        );
        card->addChild(title, 2);

        // Mark this feature when one or more of its settings differ from the
        // default, instead of showing a separate RESET button on the card.
        if (catalog::isDirty(feature)) {
            auto mark = ui::label("*", 0.46f * scale, theme::Muted);
            if (mark) {
                mark->setAnchorPoint({0.f, 0.5f});
                mark->setPosition({
                    pad + title->getContentSize().width * title->getScaleX() + 4.f * scale,
                    size.height - 24.f * scale
                });
                card->addChild(mark, 3);
            }
        }
    }

    auto desc = ui::label(feature.description, 0.38f, theme::Text);
    if (desc) {
        desc->setAnchorPoint({0.f, 1.f});
        desc->setPosition({pad, size.height - 46.f * scale});
        desc->limitLabelWidth(
            std::min(290.f * scale, size.width - pad * 2.f),
            0.38f,
            0.25f
        );
        card->addChild(desc, 2);
    }

    auto menu = ui::menuOn(card, 12);
    if (!menu)
        return card;

    char const* const status = catalog::statusLabel(feature);
    bool const active = catalog::isStatusActive(feature);
    float const pillWidth = (std::strcmp(status, "ON") == 0 ? 72.f : 62.f) * scale;
    float const pillHeight = 26.f * scale;
    CCPoint const pillPos = {
        size.width - pad - pillWidth / 2.f,
        size.height - 29.f * scale
    };

    if (feature.status == catalog::FeatureStatus::Toggle) {
        ui::pillButton(
            menu,
            status,
            pillPos,
            {pillWidth, pillHeight},
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
            {pillWidth, pillHeight},
            pillHeight / 2.f
        );
        if (pill) {
            pill->setPosition(pillPos);
            auto caption = ui::label(status, 0.34f);
            if (caption) {
                caption->setTag(StatusLabelTag);
                caption->setPosition(pill->getContentSize() / 2.f);
                pill->addChild(caption);
            }
            card->addChild(pill, 4);
        }
    }

    if (feature.hasSettings && feature.openSettings && feature.settingsEnabled) {
        ui::textButton(
            menu,
            "Configure",
            {
                size.width - 18.f * scale - 46.f * scale,
                18.f * scale
            },
            {92.f * scale, 28.f * scale},
            [open = feature.openSettings]() {
                if (open)
                    open();
            },
            theme::Button,
            theme::Text
        );
    }

    return card;
}

} // namespace

bool ModMenu::init() {
    overlay::ensureSettingsFile();

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

    auto background = ui::roundedRect(
        theme::Shell,
        m_size,
        theme::ShellRadius * overlay::desktopScale(m_size)
    );
    if (background) {
        background->setPosition(m_size / 2.f);
        m_mainLayer->addChild(background, -100);
    }

    createHeader();
    createTabBar();
    createContentPanel();
    return true;
}

void ModMenu::createHeader() {
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
        (compact ? theme::CompactHeaderRadius : theme::HeaderRadius) * scale
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
        auto version = ui::label("v0.2.0", 0.72f * scale);
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

    float const resetWidth = (compact ? 96.f : 126.f) * scale;
    float const resetHeight = (compact ? 28.f : 34.f) * scale;
    float const right = m_size.width / 2.f + headerWidth / 2.f -
        (compact ? 16.f : 21.f) * scale;

    float const closeSize = (compact ? 28.f : 34.f) * scale;
    float const closeCenterX = right - closeSize / 2.f;

    ui::textButton(
        headerMenu,
        "X",
        {closeCenterX, headerY},
        {closeSize, closeSize},
        [this]() { this->onClose(nullptr); }
    );

    ui::textButton(
        headerMenu,
        "Set to Default",
        {
            closeCenterX - closeSize / 2.f - 8.f * scale - resetWidth / 2.f,
            headerY
        },
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
            theme::CompactHeaderRadius * scale
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
        {
            sidebarLeft + sidebarWidth / 2.f,
            sidebarBottom + sidebarHeight / 2.f
        },
        {sidebarWidth, sidebarHeight},
        theme::Panel,
        theme::PanelRadius * scale
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

    // Desktop navigation is fixed to the Figma 16:9 coordinates. The sidebar
    // has only five tabs, so it does not need a nested scroll container.
    float const buttonWidth = theme::TabWidth * scale;
    float const buttonHeight = theme::TabHeight * scale;
    float const buttonCenterX = 118.f * scale;

    for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
        auto item = ui::roundedRect(
            i == m_currentTab ? theme::TabActive : theme::Card,
            {buttonWidth, buttonHeight},
            theme::TabRadius * scale
        );
        if (!item)
            continue;

        auto caption = ui::label(tabs[i].label, 0.62f * scale);
        if (caption) {
            caption->setTag(TabLabelTag);
            caption->setAnchorPoint({0.f, 0.5f});
            caption->setPosition({15.f * scale, buttonHeight / 2.f});
            caption->limitLabelWidth(
                buttonWidth - 24.f * scale,
                0.62f * scale,
                0.34f * scale
            );
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
        button->setContentSize({buttonWidth, buttonHeight});
        button->setPosition({
            buttonCenterX,
            m_size.height - (179.f + i * 55.f) * scale
        });
        m_tabMenu->addChild(button);
    }
}

void ModMenu::createContentPanel() {
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
        (compact ? theme::CompactHeaderRadius : theme::PanelRadius) * scale
    );

    auto const& tab = catalog::tabs()[static_cast<size_t>(m_currentTab)];

    auto heading = ui::label(tab.heading, compact ? 0.78f * scale : 1.18f * scale);
    if (heading) {
        heading->setAnchorPoint({0.f, 0.5f});
        heading->setPosition({
            compact ? 16.f * scale : 26.f * scale,
            size.height - (compact ? 24.f : 28.f) * scale
        });
        m_contentPanel->addChild(heading, 8);
    }

    auto subtitle = ui::label(tab.subtitle, compact ? 0.42f * scale : 0.58f * scale);
    if (subtitle) {
        subtitle->setAnchorPoint({0.f, 0.5f});
        subtitle->setPosition({
            compact ? 16.f * scale : 26.f * scale,
            size.height - (compact ? 44.f : 52.f) * scale
        });
        subtitle->limitLabelWidth(size.width - 40.f * scale, subtitle->getScale(), 0.28f);
        m_contentPanel->addChild(subtitle, 8);
    }

    auto features = catalog::featuresFor(catalog::tabAt(m_currentTab));

    if (!compact) {
        // Match the desktop frame directly: 430x138 cards with 24px column
        // spacing and the 196px / 352px top offsets from the Figma file.
        float const cardWidth = theme::CardWidth * scale;
        float const cardHeight = theme::CardHeight * scale;
        float const firstRowY = 415.f * scale;
        float const secondRowY = 259.f * scale;
        float const leftCenterX = 241.f * scale;
        float const rightCenterX = 695.f * scale;

        for (int i = 0; i < static_cast<int>(features.size()); ++i) {
            int const row = i / 2;
            int const column = i % 2;
            float const centerX = column == 0 ? leftCenterX : rightCenterX;
            float const centerY = row == 0
                ? firstRowY
                : secondRowY - (row - 1) * 156.f * scale;

            auto card = createFeatureCard(
                *features[i],
                {cardWidth, cardHeight}
            );
            if (!card)
                continue;

            card->setPosition({centerX, centerY});
            m_contentPanel->addChild(card, 4);
        }

        return;
    }

    float const headerReserve = 62.f * scale;
    auto const scrollSize = CCSize{
        size.width,
        std::max(80.f, size.height - headerReserve)
    };

    m_contentScroll = ScrollLayer::create(scrollSize, true, true);
    m_contentScroll->setPosition({0.f, 0.f});
    m_contentPanel->addChild(m_contentScroll, 5);

    int const columns = 1;
    float const pad = 16.f * scale;
    float const gapX = 12.f * scale;
    float const gapY = 12.f * scale;
    float const usable = std::max(120.f, scrollSize.width - pad * 2.f);
    float const cardWidth = usable;
    float const cardHeight = theme::CardHeight * std::min(1.f, scale * 1.05f);

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

    float const scale = overlay::desktopScale(m_size);
    float const tabRadius = theme::TabRadius * scale;

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
            selected ? theme::TabActive : theme::Card,
            tabRadius
        );

        if (auto caption = typeinfo_cast<CCLabelBMFont*>(
            background->getChildByTag(TabLabelTag)
        )) {
            caption->setColor(theme::Text);
        }
    }
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
    if (overlay::closeTop())
        return;

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

void ModMenu::keyBackClicked() {
    this->onClose(nullptr);
}

void ModMenu::onTab(CCObject* sender) {
    if (sender) {
        auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
        m_currentTab = std::clamp(btn->getTag(), 0, catalog::tabCount() - 1);
        highlightTabs();
    }

    fillContent();
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
