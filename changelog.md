# Mod Universal — Changelog

All notable changes to Mod Universal are documented here, with the newest release at the top.

## v0.1.3 — Noclip & Menu Improvements

### Added
- Added a universal **Set to Default** button.
- Added an individual **Undo** button for modified settings.

### Changed
- Changed the default Noclip states.
- Renamed several Noclip options to make their purpose clearer.
- Temporarily disabled **Configure Hazards** while the hazard configuration system is being reworked.

### Fixed
- Fixed a menu-closing bug that could cause the game to crash.

---

## v0.1.2 — Noclip & Menu Stability

### Changed
- Changed the default Noclip state from ON to OFF.

### Fixed
- Fixed Noclip not working properly.
- Fixed Noclip settings not saving when the ModUniversal menu was closed.
- Fixed Escape and F3 closing the wrong menu level.
- Fixed the mod menu being accessible during active gameplay.
- Fixed the mod menu being inaccessible while a level was paused.

---

## v0.1.1 — Settings Persistence

### Fixed
- Fixed custom Noclip setting changes not being preserved after closing and reopening the ModUniversal menu.

---

## v0.1.0 — Noclip System

### Added
- Added customizable block, slope, and hazard phasing.
- Added **Standard** and **No Hitbox** block collision modes.
- Added separate hazard categories for spikes, ground / edge spikes, sawblades, pits, animated hazards, and other hazards.
- Added persistent Noclip settings through Geode's setting system.
- Added a dedicated Noclip configuration popup.

### Changed
- Reworked the Player tab Noclip control into a checkbox with a settings button.
- Separated Noclip gameplay logic into its own source file.

### Fixed
- Fixed regular level objects being incorrectly treated as hazards by Noclip.
- Fixed block phasing being coupled to hazard protection.

---

## v0.0.1 — Initial Release

### Added
- Created the ModUniversal mod template.
