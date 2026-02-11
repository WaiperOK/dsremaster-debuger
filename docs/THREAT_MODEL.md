# Threat Model

## Assets
- Runtime process memory and camera control values.
- Operator command stream and saved camera bookmarks.

## Threats
- Invalid memory writes leading to process instability.
- Using stale addresses after scene transitions/patch changes.
- Unauthorized use in non-controlled environments.

## Mitigations
- `IsPageAccessible` and safe read/write wrappers before memory operations.
- Manual rescan workflow (`scan`/`autoscan`) when offsets change.
- Explicit operator commands and bounded parameter validation.
- Recommendation: run only in isolated lab environment and supported game versions.
