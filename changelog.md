# Changelog

## 0.1.1 - 2026-09-17

### Fixed
- Noclip setting changes are now saved immediately when changed from the custom ModUniversal menu, so closing and reopening the menu preserves the selected state.

## 0.1.0 - 2026-09-17

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
