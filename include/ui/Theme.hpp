#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// Colors and radii from the 0.2.0 Figma file (Desktop 16:9 / Compact 9:16).
namespace mu::theme {

constexpr ccColor3B Shell{14, 17, 15};       // #0e110f
constexpr ccColor3B Header{20, 23, 22};      // #141716
constexpr ccColor3B Panel{19, 22, 20};       // #131614
constexpr ccColor3B Card{24, 27, 26};        // #181b1a
constexpr ccColor3B Button{31, 36, 33};      // #1f2421
constexpr ccColor3B TabActive{31, 64, 46};   // #1f402e
constexpr ccColor3B Text{229, 235, 229};     // #e5ebe5
constexpr ccColor3B Muted{155, 158, 168};
constexpr ccColor3B StatusOn{38, 89, 59};    // #26593b
constexpr ccColor3B StatusIdle{46, 46, 46};  // #2e2e2e
constexpr ccColor3B Accent{75, 190, 138};
constexpr ccColor3B AccentText{18, 28, 24};
constexpr ccColor3B Divider{68, 72, 82};

constexpr float ShellRadius = 18.f;
constexpr float HeaderRadius = 12.f;
constexpr float CompactHeaderRadius = 10.f;
constexpr float PanelRadius = 12.f;
constexpr float CardRadius = 10.f;
constexpr float TabRadius = 8.f;
constexpr float CompactTabRadius = 7.f;
constexpr float ButtonRadius = 7.f;
constexpr float PillRadius = 14.f;

constexpr float DesktopWidth = 1200.f;
constexpr float DesktopHeight = 720.f;
constexpr float CompactWidth = 430.f;
constexpr float CompactHeight = 760.f;

constexpr float CardWidth = 430.f;
constexpr float CardHeight = 138.f;
constexpr float CardGapX = 24.f;
constexpr float CardGapY = 18.f;

constexpr float SidebarWidth = 188.f;
constexpr float TabWidth = 154.f;
constexpr float TabHeight = 42.f;
constexpr float TabGap = 13.f;

} // namespace mu::theme
