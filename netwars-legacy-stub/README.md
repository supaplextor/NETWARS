# NETWARS – Reverse-Engineered Source Code

NetWars is a space-combat game supporting up to six players over IPX networks
(Novell NetWare, ODI drivers), single-player missions, a space-simulator mode,
and a polygon ship-shape editor.

> **This source was reverse-engineered from `NETWARS.EXE`.**  
> The original sources were lost.  The code was reconstructed from the
> decompressed binary (PKLITE 1.14, Borland C++ 3.1, large memory model).
> The copyright notice embedded in the binary reads:  
> *"Copyright (c) 1997 Caldera, Inc. All rights reserved."*

---

## Repository layout

```
NETWARS/
├── NETWARS-README.md     ← you are here
├── NETWARS.EXE           ← original compressed binary (for reference)
└── src/
    ├── startup.asm       ← Borland CRT0 start-up (TASM source)
    ├── netwars.h         ← master header (structures, constants, prototypes)
    └── netwars.cpp       ← main game module (main, menus, game loop, config)
```

---

## Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| **Borland C++ for DOS** | 3.1 (1992) | C++ compiler (`bcc.exe`) |
| **TASM** | 3.2 or 4.1 | Assembler (`tasm.exe`) |
| **TLINK** | 5.x (ships with BC++ 3.1) | Linker (`tlink.exe`) |
| **PKLITE** | 1.14 (optional) | Re-compress the EXE |
| **DOS** or **DOSBox** | any | Run/test the result |

Borland C++ 3.1 is available from Borland's historical archive and has also
been released as freeware.  All tools must be on the `PATH`.

---

## How to recompile

### 1 · Assemble the start-up module

```bat
tasm /ml /m2 src\startup.asm, obj\startup.obj
```

Flags:
* `/ml` – case-sensitive labels (required for C++ linkage)
* `/m2` – allow two assembly passes (forward references)

### 2 · Compile the main C++ module

```bat
bcc -c -ml -3 -O2 -Isrc -oobj\netwars.obj src\netwars.cpp
```

Flags:
* `-c`    – compile only (no link)
* `-ml`   – large memory model (separate code and data segments; all pointers
             default to FAR)
* `-3`    – enable 80386 instructions (the original was assembled with 80286
             level; `-2` is also acceptable)
* `-O2`   – optimise for speed (matches the original's `/O` optimisation level)
* `-Isrc` – add `src/` to the include path

### 3 · Link

```bat
tlink /m /s /l obj\startup.obj obj\netwars.obj, netwars.exe, netwars.map, bcc\lib\cl.lib bcc\lib\emu.lib bcc\lib\maths.lib
```

Flags:
* `/m`  – include public symbol map
* `/s`  – include segment map
* `/l`  – include line numbers (requires compiled with `-v`)

Library paths depend on your Borland C++ installation directory.  Typical
paths are `C:\BC31\LIB\` or `C:\BORLANDC\LIB\`.

### 4 · Optional: recompress with PKLITE

The original EXE was compressed with PKLITE 1.14:

```bat
pklite -x netwars.exe
```

The `-x` flag enables extra compression.  The result should be a single-
segment stub that decompresses the game at load time, matching the structure
of the original `NETWARS.EXE`.

---

## Memory model notes

The game was compiled with Borland C++ **large memory model** (`-ml`):

* All function calls and data references use 32-bit (segment:offset) far
  pointers by default.
* `CS ≠ DS ≠ SS` at run time.
* The data segment base at runtime is **0x2AE6** (relative to the load
  segment); this value is stored at `CS:0x023C` by the CRT0 start-up code.
* Heap and BSS start at `DS:0x39CC`.
* The far heap (used for large dynamically allocated buffers such as video
  back-buffers and shape data) is managed by `_FindBestFit` in `startup.asm`.

---

## Segment map (decompressed binary)

| Offset range | Content |
|---|---|
| `0x0000 – 0x03B6` | CRT0 start-up (`startup.asm`) |
| `0x03B7 – 0x15FFF` | Main game code (`main`, menus, game loop, AI, physics) |
| `0x16000 – 0x1FFFF` | Graphics engine (EGA/SVGA/VGA blitters, polygon renderer) |
| `0x20000 – 0x259FF` | Sound engine (PC Speaker + Sound Blaster routines) |
| `0x25A00 – 0x2A85F` | IPX networking layer |
| `0x2A860 – 0x2A86E` | Borland C++ runtime signature |
| `0x2A870 – 0x2BFFF` | Initialised data (player defaults, ship polygons, palette) |
| `0x2C000 – 0x2DFFF` | String constants |
| `0x2E000 – 0x2EA60` | COMPAQ/VESA detection and miscellaneous stubs |
| `0x2EA61 – 0x3A4BF` | Uninitialized data (BSS) – zeroed by startup |

---

## Key data structures

All structures are declared in `src/netwars.h`.

| Structure | Purpose |
|---|---|
| `PlayerState` | Per-player position, velocity, shields, weapons, IPX node |
| `MissionDef`  | Mission objectives, alien counts, completion flag, score bonus |
| `HiScore`     | Initials, score, mission number for the hall of fame |
| `NWConfig`    | All `NETWARS.INI` settings |
| `IPXNode`     | Five-word Novell IPX node address |

---

## Configuration file (`NETWARS.INI`)

The game reads (and writes) `NETWARS.INI` in the current directory.
A minimal example:

```ini
[NetWars]
SoundCard = PCSpeaker
PCSpeaker = 1
SoundBlaster = 0
ComputerShip = 1
SeperateScanner = 0
CustomShip =
DefaultName = Player 1
FastMode = 0
MotionBlur = 0
SharedFile =
NetworkMode = Broadcast
Broadcast = 0000 0000 0000 0000 0000
NODE1 = 0000 0000 0000 0000 0000
NODE2 = 0000 0000 0000 0000 0000
NODE3 = 0000 0000 0000 0000 0000
NODE4 = 0000 0000 0000 0000 0000
NODE5 = 0000 0000 0000 0000 0000
NODE6 = 0000 0000 0000 0000 0000
```

---

## Multiplayer setup (IPX)

1. Load your ODI LAN driver (e.g., `NE2000.COM`).
2. Load `LSL.COM`.
3. Load `IPXODI.COM`.
4. Run `netwars.exe` and select **Multi Player**.
5. Set `NetworkMode = Broadcast` for automatic node discovery, or
   `NetworkMode = Node2Node` and fill in the `NODE1`–`NODE6` addresses in
   `NETWARS.INI` for explicit peer-to-peer connections.

The NetWars IPX socket number is `0x4577`.

---

## Video modes

| Mode | Resolution | Colours | INT 10h function |
|------|-----------|---------|-----------------|
| EGA  | 640 × 350 | 16      | `AH=00h AL=10h` |
| SVGA | 640 × 480 | 256     | VESA `AX=4F02h BX=0101h` |
| VGA  | 320 × 200 | 256     | `AH=00h AL=13h` |

---

## Reverse-engineering notes

The decompressed binary was extracted with a Python/Unicorn x86-emulation
script that ran the PKLITE 1.14 decompression stub natively (approx.
1.5 million 8086 instructions).  The resulting 239,712-byte image was then
disassembled with `ndisasm` (NASM 2.x) to produce a 65,000-line annotated
listing from which this source was reconstructed.

Key identifiers were recovered from 344 human-readable ASCII strings embedded
in the data segment (DS base `0x2AE6`), including all menu labels, error
messages, INI key names, and the Borland C++ version stamp at offset `0x2A864`.

The CRT0 start-up code (`startup.asm`) closely matches the standard Borland
C++ 3.1 large-model `C0L.ASM` start-up as shipped with BC++ 3.1, with minor
game-specific additions (custom divide-error handler, far-heap initialisation).

---

## Known gaps

The outstanding subsystems are tracked in the repository's [canonical tasks
list](../tasks.md). Technical references from the reverse-engineering notes:

* **Polygon renderer** – the wireframe ship-shape drawing routines at offsets
  `0x16000–0x1AFFF` (EGA planar bit-manipulations and VGA mode-13h blitter).
* **Sound Blaster driver** – DSP command sequences at offsets `0x20000–0x229FF`.
* **IPX send/receive** – the low-level ECB/ESR structures at `0x25A00–0x2A85F`.
* **Shape editor** – the interactive polygon editing tool at `0x13000–0x15FFF`
  (requires a mouse driver).
* **Star-field engine** – the parallax scrolling star renderer that uses the
  sine/cosine lookup tables at DS:`0x260B8`.
