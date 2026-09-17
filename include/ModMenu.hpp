#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class ModMenu : public geode::Popup
{
protected:
    bool init();

    void createHeader();
    void createTabBar();
    void createContentPanel();

    void onNoclip(CCObject*);
    void onNoclipSettings(CCObject*);

    void onTab(CCObject*);
    void onClose(CCObject* sender) override;

public:
    static ModMenu* create();
    static void toggle();

    static bool isNoclipEnabled();
    static void setNoclipEnabled(bool enabled);

    static bool isPhaseThroughBlocksEnabled();
    static bool isNoBlockTouchMode();
    static bool isPhaseThroughSlopesEnabled();
    static bool isPhaseThroughHazardsEnabled();
    static bool isHazardCategoryEnabled(int category);

    static void openNoclipSettings();
    static void openNoclipHazardSettings();

private:
    inline static ModMenu* s_instance = nullptr;

    CCNode* m_contentPanel = nullptr;
};
