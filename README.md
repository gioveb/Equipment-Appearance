# Equipment Appearance 0.2.1

A DAOLib plugin for Dragon Age: Origins: per-character helmet visibility, HEELS, Full Body robes and optional weapon scaling.

## Downloads

[Download version 0.2.1](https://github.com/gioveb/Equipment-Appearance/releases/tag/v0.2.1)

Players: use **Equipment_Appearance_0.2.1_Main.zip**. This is a manual-install ZIP, not a DAZIP. Read [installation instructions](README_EN.txt): GameFiles goes into the game installation; DocumentsFiles contains support resources for the active Documents/BioWare/Dragon Age folder. DAOLib is required. Back up your settings and do not run the older standalone heels_probe/robe_fullbody plugins alongside this plugin.

## Source and build

The source corresponding to the 0.2.1 release is in `src`. The complete source ZIP on the release includes unpacked dependency sources as well. No compiler, game executable, DAOLib loader DLL or custom outfit meshes are bundled.

1. Download this repository at tag `v0.2.1` and extract it, or clone it.
2. Install the official [LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw/releases) Windows toolchain. The checked build used `llvm-mingw-20261006-ucrt-x86_64` and targets **i686 (32-bit)**.
3. Open PowerShell in the source folder and run:

```powershell
./build.ps1 -Toolchain "C:/path/to/llvm-mingw/bin"
```

The script extracts `vendor_sources.zip` into `vendor` if missing, then builds and runs the offline checks. The archive contains the exact dependency sources used for the release (DAOLib, DAOAddressLibrary, MinHook and spdlog), with their licences. nlohmann/json is in `src/third_party`.

Output: `build/bin_ship/daolib/plugins/equipment_appearance/equipment_appearance.dll`, with the INI and scale-table files beside it. The full source release can be built with the same command; its dependencies are already unpacked.

## Version 0.2.1

Fixes helmet visibility changes during dialogue/cutscene transitions. The author confirmed Captured! with helmets shown and hidden and preference persistence after save/load. Includes scale table 1.3.3 (99 entries), Bregan's Bow 0.85, Bulwark of the True King 1.0, and updated Enchanter's Staff Refined mesh references.

HEELS, Full Body, local-origin weapon scaling and saved helmet choices remain included. Ear hiding is not implemented. GOG/Steam executable layouts were checked; EA gameplay with the unified plugin is not verified. DAOLib 0.11.0 interface/address compatibility was checked statically, not certified through a full gameplay test.

See [README_EN.txt](README_EN.txt) and [CHANGELOG_EN.txt](CHANGELOG_EN.txt) for settings, installation, limitations and history.

## Licence and credits

Plugin source headers carry GPL-3.0-or-later notices; see [LICENSE](LICENSE). Third-party components retain their own licences in `LICENSES` and the dependency archive. DAOLib and DAOAddressLibrary: Whocares66-Dev. Weapon geometry comparison reference: Dalls Rescaled Weapons. Table estimates are not recovered author settings. No Dalls mesh assets are redistributed. Light Caps Scalp Fix is a separate mod.
