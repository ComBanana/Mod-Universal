#include "../../include/ui/Overlay.hpp"
#include "../../include/ui/Theme.hpp"

#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace mu::overlay {
namespace {

struct NestedPopupEntry {
    WeakRef<Popup> popup;
    std::function<void()> close;
};

std::vector<NestedPopupEntry> s_nestedPopups;

CCSize aspectReferenceSize() {
    auto const director = CCDirector::sharedDirector();
    if (!director)
        return {1.f, 1.f};

    if (auto const view = director->getOpenGLView()) {
        auto const& frame = view->getFrameSize();
        if (frame.width > 1.f && frame.height > 1.f)
            return frame;
    }

    auto const winSize = director->getWinSize();
    if (winSize.width > 1.f && winSize.height > 1.f)
        return winSize;

    return {1.f, 1.f};
}

} // namespace

void registerPopup(Popup* popup, std::function<void()> close) {
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

    s_nestedPopups.push_back({popup, std::move(close)});
}

void unregisterPopup(Popup* popup) {
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

bool closeTop() {
    while (!s_nestedPopups.empty()) {
        auto entry = std::move(s_nestedPopups.back());
        s_nestedPopups.pop_back();

        auto popup = entry.popup.lock();
        if (!popup || !popup->getParent())
            continue;

        if (entry.close)
            entry.close();

        return true;
    }

    return false;
}

void closeAll() {
    while (closeTop()) {
    }
}

bool saveModData() {
    if (auto result = Mod::get()->saveData(); !result) {
        log::error(
            "Failed to save Mod Universal settings: {}",
            result.unwrapErr()
        );
        return false;
    }

    return true;
}

CCSize popupSize(float preferredWidth, float preferredHeight) {
    auto const screen = CCDirector::sharedDirector()->getWinSize();
    float const availableWidth = std::max(220.f, screen.width - 24.f);
    float const availableHeight = std::max(180.f, screen.height - 24.f);
    float const scale = std::min(
        1.f,
        std::min(
            availableWidth / preferredWidth,
            availableHeight / preferredHeight
        )
    );

    return {
        preferredWidth * scale,
        preferredHeight * scale
    };
}

float windowAspect() {
    auto const size = aspectReferenceSize();
    return size.width / std::max(1.f, size.height);
}

bool isCompact() {
    return windowAspect() < 0.95f;
}

float desktopScale(CCSize size) {
    if (isCompact()) {
        return std::min(
            1.f,
            std::min(
                size.width / theme::CompactWidth,
                size.height / theme::CompactHeight
            )
        );
    }

    return std::min(1.f, size.width / theme::DesktopWidth);
}

void ensureSettingsFile() {
    auto const path = Mod::get()->getSaveDir() / "settings.json";
    std::error_code ec;

    if (std::filesystem::exists(path, ec) || ec)
        return;

    saveModData();
}

} // namespace mu::overlay
