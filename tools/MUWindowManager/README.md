# MUWindowManager

Standalone Win32 manager for up to four MU Origins client windows.

## Controls

- `F12`: toggle Hide All / Restore All.
- Double-click the tray icon: toggle Hide All / Restore All.
- Right-click the tray icon: Restore All, Hide All, or Exit.

Only top-level windows owned by `main.exe`, using the MU client window class,
and launched beside a `Data` directory are managed. The process ID and full
executable path are revalidated before every hide or restore operation.

If F12 is already registered by another program, tray controls remain
available. Exiting the manager restores every window it hid.

## Visual Studio build

```powershell
msbuild MUWindowManager.vcxproj /p:Configuration=Release /p:Platform=Win32
```

The Visual Studio project places `MUWindowManager.exe` in `bin/Client`.

## CMake build

```powershell
cmake -S tools/MUWindowManager -B build/MUWindowManager -A Win32
cmake --build build/MUWindowManager --config Release
```
