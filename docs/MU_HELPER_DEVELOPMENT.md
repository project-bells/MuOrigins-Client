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

Status: **protocol and server core implemented; client UI and live integration tests pending**.

- The inspected unused extension opcode is `0xFA`. Subcodes `00` through `05`
  cover configuration, start, stop, status, offline request, and synchronization.
- Both sides use protocol version 2, exact packet-size checks, bounded fields,
  and a 250 ms server command rate limit. The client never submits damage,
  rewards, inventory results, or character state.
- `Source/MuServer/GameServer/HelperManager.*` runs from the existing 100 ms
  monster timer and keeps per-object runtime state outside `OBJECTSTRUCT`.
- The server selects the nearest valid viewport monster and reuses the existing
  attack, skill-delay, mana/BP, potion, effect, inventory, and packet APIs.
- Implemented core actions: start/stop/status, primary/secondary skill cadence,
  basic attacks, HP/MP potions, and the four supported self buffs.
- `src/Net/HelperProtocol.*` provides the matching Win32 client transport and
  validated status state; disconnect/character reset clears that state.
- Win32 Release built successfully with zero warnings on 2026-10-19.

The first in-game settings panel is implemented in `src/UI/HelperWindow.*`.
Press `H` while in the world to open it. It exposes server status, attack,
hunting range, HP/MP thresholds, primary/secondary skill IDs, secondary-skill
cadence, auto buff, Apply, Start, and Stop. Its hit area blocks world movement,
and the hotkey is disabled while chat input is active.

Pending: named skill selection, movement/return behavior (Phase 4), Linux
Docker compilation, and live combat/potion/buff verification. Offline requests
deliberately return `NOT_SUPPORTED` until Phase 5 instead of disconnecting or
pretending that offline control is active.

## Phase 4 — Advanced automation

Status: **Auto Loot core implemented; navigation, party, and persistence pending**.

Protocol version 2 adds independent Zen, jewel, and Excellent-item filters plus
a pickup range of 1-5 tiles. The GameServer only chooses a nearby viewport item;
the actual pickup is passed to the original `CGItemGetRecv`, preserving item
ownership, quest rules, inventory capacity, Zen limits, and normal client
inventory packets. The Helper never creates or awards an item directly.

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
