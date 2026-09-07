# Kaper 64
A Commodore 64 port of the Privateer game (Kaptajn Kaper i Kattegat)

# Original source code
The original source code in QBASIC can be found here: https://github.com/kb-dk/KaptajnKaper

## Build Source Of Truth
Assembler builds use `kaper.asm` and the included `.inc` files directly.

The project uses Kick Assembler. From PowerShell, build the program with:

```powershell
java -jar C:\apps\KickAssembler\KickAss.jar .\kaper.asm -o .\bin\kaper.prg
```

To create the disk image and run the game in VICE, use:

```powershell
.\run-vice-with-disk.cmd
```

## Modules
- `kaper.asm` is the main entry point, holds the global player state, the name-entry
  screen, and imports the program modules.
- `constants.inc` contains shared memory addresses, VIC configuration, gameplay
  constants, the shared zero-page pointers, and the inline-argument print macros
  (`pstr`, `pstrnl`, `pnum`, `astr`, `wstr`, `bstr`, `bnum`, `fnum`).
- `audio.inc` contains the SID player and the tune/sound-effect data.
- `start.inc` contains the title screen shown before the intro.
- `intro.inc` contains the intro screen, the RLE unpacker for the intro bitmap, text-screen
  setup, VIC bank switching, the gameplay sound effects, and shared display helpers.
- `intro.dat` is the hires intro bitmap (load address `$5c00`: 1000 bytes of screen colour
  data, 24 pad bytes, then the 8000-byte bitmap at `$6000`). Kick Assembler RLE-packs it at
  build time (9024 -> 3157 bytes) into `intro_rle_data`, so it is neither loaded from disk
  nor stored uncompressed. It is converted from `intro.art` (OCP Art Studio hires) in the
  `Kaper64 BASIC\intro` folder.
- `shooting.inc` contains the cannon-battle minigame and shooting screen data.
- `harbour_sailing.inc` contains the harbour-approach minigame.
- `harbour.inc` contains harbour trading and resource management.
- `map.inc` contains map movement, random events, boarding combat, the end-game and
  victory screens, the text/bitmap rendering helpers, and the map screen renderer.
- `map.dat` contains the raw high-resolution bitmap and screen data for the map.


## Memory Layout
- The intro uses a 320x200 hires bitmap stored RLE-packed at `$b340` (in the RAM under
  the BASIC ROM) and unpacked to screen `$5c00` / bitmap `$6000` before each showing.
- `draw_map` reuses `$6000-$7f3f` for the map bitmap, which is why the intro is unpacked
  again rather than kept around when it is shown after a game over.
- The map uses a high-resolution bitmap with screen/color data in VIC bank 1.
- Program code must stay below `$4400` (VIC bank 1 screen RAM); the name-entry screen
  prints the remaining headroom as a build-time diagnostic.
- Intro and event screens are prepared while the VIC display is hidden to avoid
	showing partially drawn screen contents during transitions.
