# Changelog

## 0.1.3

### Added
- Added a “Set to Default” button along with an individual button in each mod.

### Changed
- Changed Noclip default states.
- Changed some mod names to avoid confusion.
- Temporarily disabled the “Configure Hazards” option in Noclip options.

### Fixed
- Fixed a bug involving closing menus that would crash the game.

## 0.1.2

### Fixed
- Fixed Noclip not working properly.
- Fixed Noclip settings not saving when closing the ModUniversal menu.
- Fixed problems with Escape and F3 closing the wrong menu level.
- Fixed the mod menu being accessible while playing a level.
- Fixed the mod menu being inaccessible while a level is paused.

### Changed
- Changed the default Noclip state from ON to OFF.

## 0.1.1

### Fixed
- Noclip setting changes are now saved immediately when changed from the custom ModUniversal menu, so closing and reopening the menu preserves the selected state.

## 0.1.0

### Added
- Customizable Noclip settings for block, slope, and hazard phasing.
- Safe Block Touch and No Block Touch collision modes.
- Separate hazard categories for spikes, ground / edge spikes, sawblades, pits, animated hazards, and other hazards.
- Persistent Noclip settings through Geode's setting system.
- Dedicated Noclip configuration popup.

### Changed
- Reworked the Player tab Noclip control into a checkbox with a gear button for configuration.
- Separated Noclip gameplay logic into its own source file.

### Fixed
- Regular level objects are no longer incorrectly treated as hazards by Noclip.
- Phase Through Blocks now controls block phasing independently from hazard protection.

## 0.0.1

### Added
- Created the mod template.
