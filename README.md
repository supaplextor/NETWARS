# NETWARS – modernized build notes

This repository contains the original reverse-engineered DOS-era NetWars assets, plus a modernized Linux build path designed for contemporary desktops, 4K displays, and Debian Linux Trixie.

## What changed

- Added a modern SDL2 launcher for contemporary operating systems.
- Added a 4K-capable render loop with a starfield background and HUD shell.
- Added CMake build support for Debian Linux Trixie and other modern Unix-like systems.
- Kept the original DOS reverse-engineering files intact for historical reference.

## Supported environments

- Debian Linux Trixie
- Ubuntu 24.04+ / Debian 12+
- macOS 13+
- Windows 10/11 with SDL2 available

## Prerequisites

On Debian Trixie:

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config libsdl2-dev
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/netwars-modern
```

The launcher opens a modern window with a 4K logical canvas (3840x2160) and scales cleanly on high-DPI screens.

## Notes

This modern build is intentionally a playable baseline and modernization layer rather than a direct port of the original Borland C++ DOS code. The DOS-era files remain in `NETWARS/` and are preserved as reverse-engineering artifacts.

The original Reverse-Engineered Source Code project is still available in the `NETWARS/` directory and is not replaced by this modern runtime.
