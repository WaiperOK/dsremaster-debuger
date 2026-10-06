# Changelog

## Unreleased
- Fixed `IsPageAccessible`: it only looked at the first page of a range and accepted guard pages. It now walks every region of the range and rejects `PAGE_GUARD` and `PAGE_NOACCESS`.
- Safe read/write helpers and the pattern scan now use structured exception handling. A plain `catch (...)` does not catch access violations in MSVC, so a page that disappeared mid-copy could crash the game.
- Pattern scanner walks the module region by region instead of calling `VirtualQuery` for every byte, and the byte matching moved to the header-only `PatternMatch.h` so it can be tested without the game. The last position of a region is no longer skipped.
- `FindCameraAddresses` returns an empty list when no candidate passes validation instead of the unfiltered candidates.
- `HexStringToBytes` rejects non hex input instead of turning it into zero bytes.
- Addresses in log messages are printed as hex (`0x...`); they used to be a `0x` prefix followed by a decimal number.
- `IsValidCameraAddress` rejects NaN/infinity and only treats an all zero triple as empty, so a camera at 0 on one axis is accepted.
- Scan progress is reported every 10% (the old modulo check almost never fired).
- `DllMain` closes the thread handle and disables thread notifications.
- Added `tools/test_pure_logic.cpp` for the platform independent helpers.

## v0.2.0 - 2026-02-11
- Added advanced debug menu commands: `status`, `speed`, `fov`, `timescale`, `savepos`, `gotopos`, `bookmarks`.
- Added bookmark management for camera coordinates.
- Improved input handling by draining `std::cin` after numeric commands.
- Removed stale local-agent scaffold and obsolete console stub.
- Added architecture docs, threat model, roadmap, security policy, and CI workflow.
