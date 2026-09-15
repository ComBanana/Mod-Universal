#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class ModMenu : public geode::Popup {
protected:
    bool init();

    void createHeader();
    void createTabBar();
    void createContentPanel();
    void onTab(CCObject*);
    void onClose(CCObject* sender) override;

public:
    static ModMenu* create();
    static void toggle();

private:
    inline static ModMenu* s_instance = nullptr;
};