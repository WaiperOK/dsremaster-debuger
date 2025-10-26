# Repository Guidelines

## Project Structure & Module Organization
- `DarkSoulsDebugMenu.sln` loads the single Visual Studio solution; `DarkSoulsDebugMenu.vcxproj` defines the Win32 DLL target.
- Runtime entry points live in `dllmain.cpp` and `DarkSoulsDebugMenu.cpp`, while gameplay tooling sits in `DebugCamera.cpp` and helpers in `MemoryManager.cpp`.
- Shared headers (`DebugCamera.h`, `MemoryManager.h`, `pch.h`) sit beside their sources to keep include paths simple.
- No dedicated test directory exists yet; ad-hoc test scaffolding can live under `tools/` if you add one to avoid polluting the shipping DLL.

## Build, Test, and Development Commands
- `msbuild DarkSoulsDebugMenu.sln /p:Configuration=Debug /p:Platform=x64` — builds the DLL from a Developer Command Prompt for VS 2022.
- `msbuild DarkSoulsDebugMenu.sln /t:Rebuild /p:Configuration=Release /p:Platform=x64` — produces a clean release build for shipping.
- `devenv DarkSoulsDebugMenu.sln /Build "Debug|x64"` — alternative GUI-friendly build for local debugging.
- Attach the built DLL to the Dark Souls process via your preferred injector and monitor the console window to validate command handling.

## Coding Style & Naming Conventions
- Follow the existing 4-space indentation and brace-on-same-line style used across `main.cpp` and `DebugCamera.cpp`.
- Prefer PascalCase for functions (`ReadConsoleInput`), CamelCase for local variables, and the `C` prefix for classes (`CDebugCamera`) to match current headers.
- Keep headers self-contained, rely on `pch.h` for heavy system includes, and avoid non-ASCII comments to prevent mojibake in default Windows encodings.

## Testing Guidelines
- Manual validation is expected: inject the Debug build and exercise console commands such as `showpos` or `setpos` to verify camera writes.
- When adding memory operations, include guard helpers similar to `IsValidCameraAddress` and log to `std::cout` for quick sanity checks.
- If you introduce automated tests, place them behind `#ifdef _DEBUG` blocks and ensure they do not ship in Release builds.

## Commit & Pull Request Guidelines
- Mirror the existing `scope: short summary` convention (`camera: tighten FOV clamping`). Write subjects in imperative mood and keep them under 72 characters.
- Include concise body bullets for context, note any offsets or signatures you changed, and link Dark Souls patch versions when relevant.
- Pull requests should describe the feature toggle or memory address touched, list manual validation steps, and attach screenshots or logs when UI or console output changes.

## Agent-Specific Instructions
- Always document discovered memory addresses and magic constants inside `MemoryManager.cpp` comments to keep reverse-engineering reproducible.
- Record any required Windows handle permissions or injector settings in your PR so maintainers can replicate the environment quickly.
