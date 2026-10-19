# MU Origins 0.97K — Six-Phase Development Record

This document records verified source locations, implementation status, build
evidence, and pending manual tests for the hybrid MU Helper project.

## Phase 1 — Repository audit and baseline

Status: **audit complete; Windows baseline build verified; Linux build pending**.

### Windows client

- Entry point: `src/WinMain.cpp::WinMain`
- Window creation: `src/Core/Window.cpp::CWindow::Create`
- Window procedure: `src/Core/WndProc.cpp::WndProc`
- Input dispatch: `src/Input/Input_WndProc.cpp`
- Frame/render loop: `src/Game/Game_MainLoop.cpp`
- Network transport: `src/Net/Network.cpp`
- Protocol definitions: `src/Net/Protocol/`
- User configuration: `src/Config/UserSettings.cpp`
- Build systems: `mu97k.sln`, `mu97k.vcxproj`, and `CMakeLists.txt`

The legacy `FindWindowA` single-instance shutdown is already disabled in
`WinMain.cpp`. A successful Win32 Release baseline build was produced with the
Visual Studio v143 toolchain on 2026-10-19.

### Linux server

- Build system: `Source/MuServer/CMakeLists.txt` (C++17)
- GameServer initialization/timers: `Source/MuServer/GameServer/GameMain.cpp`
- User periodic processing: `Source/MuServer/GameServer/User.cpp`
- Object/monster processing: `Source/MuServer/GameServer/ObjectManager.cpp`
- Client protocol dispatch: `Source/MuServer/GameServer/Protocol.cpp`
- Combat validation: `Source/MuServer/GameServer/Attack.cpp`
- Skill validation/execution: `Source/MuServer/GameServer/SkillManager.cpp`
- Item/inventory operations: `Source/MuServer/GameServer/ItemManager.cpp`
- Character load/save: `Source/MuServer/GameServer/DSProtocol.cpp`
- Linux socket dispatch: `Source/MuServer/GameServer/SocketManagerLinux.cpp`
- Database schema: `MuServer/MySQL/DB/`
- Container build: root `Dockerfile` and `docker-compose.yml`

WSL enumeration is denied in the current Windows environment, so the Linux
baseline has not been claimed as built. It must be verified in Docker or on the
Ubuntu host before server Helper changes are deployed.

## Phase 2 — Four clients and global F12

Status: **source implemented and Win32 Release build verified; live GUI matrix pending**.

- `src/Core/MultiClientPolicy.*` reserves one of four atomic, per-installation
  process slots. Windows releases a slot even after a crash.
- `tools/MUWindowManager/` builds the standalone `MUWindowManager.exe`.
- F12 toggles all managed clients; tray commands remain available if F12 is
  occupied by another application.
- A client is accepted only after validating its window class, process ID,
  full `main.exe` path, and adjacent `Data` directory.
- Stale HWND reuse is rejected by repeating PID/path validation.
- Newly launched clients are hidden immediately while global hidden state is
  active. Exiting the manager restores windows it hid.
- `[MultiClient] MaxInstances=1..4` may be added to `Config.ini`; the default
  and hard maximum are both 4.

Pending manual tests: one through four live clients, fifth-client rejection,
F12 hide/restore, client exit while hidden, new client while hidden, manager
exit restore, and hotkey-conflict fallback.

## Phase 3 — Core MU Helper

Status: **pending**.

Implement versioned settings/start/stop/status packets, a server-authoritative
Helper scheduler, combat/skill/potion/buff validation through existing APIs,
and the first in-game client settings panel.

## Phase 4 — Advanced automation

Status: **pending**.

Implement owner-validated item pickup and filters, hunting-origin return,
navigation/stuck recovery, party automation, and per-character persistence.

## Phase 5 — Offline hunting

Status: **pending**.

Implement explicit offline-session state, safe disconnect/reconnect transfer,
duplicate-session prevention, periodic persistence, policy stops, and recovery
auditing without relying on the Windows client.

## Phase 6 — Optimization and testing

Status: **pending**.

Add hidden/background rendering throttles, bounded server scheduling, telemetry,
and load-test 1/4/10/50/100 Helper sessions where the host permits. Record CPU,
memory, tick latency, and network stability without claiming unrun tests.
