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

    void onTab(CCObject *);
    void onClose(CCObject *sender) override;

public:
    static ModMenu *create();
    static void toggle();

    static bool isNoclipEnabled();
    static void setNoclipEnabled(bool enabled);

private:
    inline static ModMenu *s_instance = nullptr;

    CCNode *m_contentPanel = nullptr;
};