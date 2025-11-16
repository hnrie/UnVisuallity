# UnVisuallity

## Overview
UnVisuallity is a Roblox exploit framework focused on providing a stable execution environment for custom Luau scripts. The project sets up a Windows DLL that hooks RobloxPlayer, exposes a curated library surface to in-game scripts, and renders debugging overlays via ImGui. Internally the project is split into execution, rendering, teleport handling, communication, and Roblox reverse-engineering helpers to keep each concern isolated.

## Features
* Deterministic Luau execution via the `execution` subsystem.
* Task scheduler hooks that queue arbitrary code onto Roblox's worker threads.
* Teleport persistence so the environment can survive game transitions.
* Custom ImGui-based UI for script editors and key validation flows.
* Drawing, HTTP, filesystem, websocket, and console libraries exposed to scripts.

## Prerequisites
* Windows 10/11 with the Desktop Development with C++ workload installed (Visual Studio 2022 recommended).
* CMake 3.24+ and Ninja (preferred) or MSBuild generators.
* vcpkg for dependency management (the repository includes a `vcpkg.json`).
* RobloxPlayer installed locally for address rebasing.

## Building
1. Install dependencies through vcpkg:
   ```powershell
   vcpkg install --triplet x64-windows
   ```
2. Generate the project files:
   ```powershell
   cmake -B build -S . -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="<path-to-vcpkg>\scripts\buildsystems\vcpkg.cmake"
   ```
3. Build the DLL:
   ```powershell
   cmake --build build --config Release
   ```
4. The compiled DLL will be located in `build/` (or `cmake-build-release/` when using CLion/MSVC). Inject this module into RobloxPlayerBeta.exe using your preferred injector.

## Usage
1. Launch Roblox and join a game.
2. Inject the built UnVisuallity DLL once the process is stable.
3. The entrypoint (`entry.cpp`) waits for Roblox to finish loading, captures the `ScriptContext`, and initializes:
   * `environment`: registers all Lua-facing libraries.
   * `taskscheduler`: hooks Roblox's scheduler and starts executing queued scripts.
   * `renderer`: sets up DirectX11 swap-chain hooks and ImGui for UI rendering.
   * `communication`: begins the remote command channel if configured.
4. Drop Luau scripts into the configured workspace/autoexec directories, or send code through the communication channel to run it in-game.

## Repository Layout
* `entry.cpp` – DLL entrypoint and process bootstrapper.
* `src/core` – Core runtime pieces (environment, execution, rendering, communication, teleport handling).
* `src/rbx` – Reverse-engineered Roblox structures, offsets, and helper functions.
* `src/misc` – Data used by drawing APIs (fonts, serialized image resources, etc.).
* `dependencies` – Third-party headers such as the curl wrapper and HTTP status helpers.

## Development Tips
* Docstrings follow a Doxygen-compatible style so IDEs can surface rich hints.
* Public globals such as `g_execution` or `g_taskscheduler` provide singletons for each subsystem—reuse them rather than creating additional instances.
* When updating offsets in `src/rbx/engine/game.h`, ensure you also rebase the function pointers defined at the bottom of the file.

## Contributing
1. Fork the repository and create a feature branch.
2. Make your changes, ensuring you add/update documentation and keep formatting consistent.
3. Run the build to ensure the DLL compiles.
4. Submit a pull request summarizing the work.