# Arcanum Restored CE

> [!IMPORTANT]
> This is a beta, technology preview, or whatever other label you have in mind for a pre-release software.

## Fork Notice

Arcanum Restored CE is a public fork of [Arcanum Community Edition](https://github.com/alexbatalov/arcanum-ce)
by Alex Batalov. The original Community Edition work remains credited upstream; this fork builds on that foundation
with a restoration-focused roadmap for engine hardening, modding support, content expansion, and long-term playability.

This repository does not include original Arcanum game assets. You must own a copy of
*Arcanum: Of Steamworks and Magick Obscura* to play.

## Status

The base Community Edition engine is playable on modern systems, but Restored CE is an active restoration fork.
Changes land in small, reviewable lanes with build evidence, compatibility scans, and clear rollback notes.
Default single-player behavior stays protected unless a change is explicitly documented and gated.

## Project Goals

Arcanum Restored CE is intended to become a stronger, more flexible foundation for long-term Arcanum work while
preserving the feel, rules, and atmosphere of the original game. The focus is not to replace Arcanum with something
unrecognizable, but to make the engine sturdier, easier to diagnose, and more welcoming to ambitious restoration and
expansion projects.

The current direction centers on:

- Engine hardening: safer startup diagnostics, better scan modes, cleaner configuration paths, stronger file-loading
  reports, and more predictable platform behavior.
- Practical bug fixes driven by real play, preview builds, compatibility scans, and reproducible saves.
- Module expansion support for restored areas, new maps, improved tooling, and an in-game travel layer that can support
  module-expansion routes without overwriting stock game data.
- Character expansion work for larger rosters, restored presentation options, portraits, art routes, and gameplay data
  without destabilizing the base game.
- Follower restoration support, including cleaner dialogue/audio routing and future follower voice-over packages where
  permissions and asset boundaries allow it.
- Multiplayer and co-op foundations that strengthen the existing Arcanum multiplayer surface over time while keeping
  single-player compatibility protected.
- Preview build and companion-tool workflows that make testing easier for players, modders, and restoration contributors.

The long-term goal is a polished Restored CE fork that can serve players, modders, and restoration projects at the same
time: stable enough for normal play, transparent enough for debugging, and flexible enough for larger Arcanum worlds.

## Installation

You must own the game to play. Purchase your copy on [GOG](https://www.gog.com/game/arcanum_of_steamworks_and_magick_obscura) or [Steam](https://store.steampowered.com/app/500810).

<details>
    <summary>Minimum installation</summary>

    ```
    .
    ├── arcanum1.dat
    ├── arcanum2.dat
    ├── arcanum3.dat
    ├── arcanum4.dat
    ├── modules
    │   ├── Arcanum
    │   │   ├── movies
    │   │   │   ├── 00069.bik
    │   │   │   ├── 01138.bik
    │   │   │   ├── 02112.bik
    │   │   │   ├── 50000.bik
    │   │   │   ├── 51169.bik
    │   │   │   ├── 91568.bik
    │   │   │   ├── A0021.bik
    │   │   │   ├── G0021.bik
    │   │   │   └── movies.mes
    │   │   └── sound
    │   │       └── music
    │   │           ├── Arcanum.mp3
    │   │           ├── Caladon.mp3
    │   │           ├── Caladon_Catacombs.mp3
    │   │           ├── Cities.mp3
    │   │           ├── Combat 1.mp3
    │   │           ├── Combat 2.mp3
    │   │           ├── Combat 3.mp3
    │   │           ├── Combat 4.mp3
    │   │           ├── Combat 5.mp3
    │   │           ├── Combat 6.mp3
    │   │           ├── CombatMusic.mp3
    │   │           ├── Dungeons.mp3
    │   │           ├── DwarvenMusic.mp3
    │   │           ├── Interlude.mp3
    │   │           ├── Isle_of_Despair.mp3
    │   │           ├── Kerghan.mp3
    │   │           ├── Mines.mp3
    │   │           ├── Qintara.mp3
    │   │           ├── Tarant.mp3
    │   │           ├── Tarant_Sewers.mp3
    │   │           ├── Towns.mp3
    │   │           ├── Tulla.mp3
    │   │           ├── Vendegoth.mp3
    │   │           ├── Villages.mp3
    │   │           ├── Void.mp3
    │   │           └── Wilderness.mp3
    │   ├── Arcanum.PATCH0
    │   ├── Arcanum.dat
    │   └── Vormantown.dat
    └── tig.dat
    ```
</details>

### Windows

Download and copy `arcanum-ce.exe` to your `Arcanum` folder. It serves as a drop-in replacement for `arcanum.exe`.

### Linux

- Use the Windows installation as a base - it contains the data assets needed to play. Copy the `Arcanum` folder somewhere, for example `/home/john/Desktop/Arcanum`.

- Alternatively, you can extract the required files from the GOG installer:

```console
$ sudo apt install innoextract
$ innoextract ~/Downloads/setup_arcanum.exe -I app
$ mv app Arcanum
```

- Download and copy `arcanum-ce` to this folder.

- Run `./arcanum-ce`.

### macOS

> [!NOTE]
> macOS 10.13 (High Sierra) or higher is required. Runs natively on Intel-based Macs and Apple Silicon.

- Use the Windows installation as a base - it contains the data assets needed to play. Copy the `Arcanum` folder somewhere, for example `/Applications/Arcanum`.

- Alternatively, if you have Homebrew installed, you can extract the required files from the GOG installer:

```console
$ brew install innoextract
$ innoextract ~/Downloads/setup_arcanum.exe -I app
$ mv app /Applications/Arcanum
```

- Download and copy `Arcanum Community Edition.app` to this folder.

- Run `Arcanum Community Edition.app`.

### Android & iOS

These ports are not currently intended for players. Touch controls are not yet implemented, and window management is subpar. No further instructions will be provided until these issues are resolved (but you can easily figure it out, it's not rocket science).

## Building from source

Check [`ci-build.yml`](.github/workflows/ci-build.yml) for details on how the project is compiled.

### Companion tools

Desktop builds also include `arcanum-ce-module-check`, a read-only scanner for loose module/data folders.
See [`docs/ARCANUM_CE_MODULE_CHECK.md`](docs/ARCANUM_CE_MODULE_CHECK.md).

## Configuration

Several configuration options are available as command-line switches (admittedly not very user-friendly):

- `-4637`: Enable cheat level 3

- `-window`: Run in windowed mode (default is fullscreen)

- `-geometry=1280x720`: Set window size (default is 800x600)

Community Edition also reads optional startup settings from `arcanum-ce.cfg` in the game directory. It accepts the same option names as `HighRes/config.ini`, plus lower snake_case spellings such as `scroll_fps`.

```ini
profile=restored-default
module=Arcanum
width=1280
height=720
windowed=1
show_fps=0
scroll_fps=35
scroll_dist=10
startup_report=1
logos=1
intro=1
```

When `HighRes/config.ini` from the Unofficial Arcanum Patch is present in the game directory, Community Edition imports it first. Values in `arcanum-ce.cfg` override the legacy import. Command-line switches such as `-window`, `-geometry=1280x720`, `-scrollfps:35`, and `-scrolldist:10` still override matching file settings for that launch.

Restored CE profile files can describe the expected module, DATs, and loose override roots for a runtime. When startup reporting is enabled, the engine writes `restored-ce-startup-report.txt` with the selected module, config sources, profile state, and path checks. See [`docs/ARCANUM_RESTORED_CE_PROFILES.md`](docs/ARCANUM_RESTORED_CE_PROFILES.md).

## Contributing

Play the game and file bugs if any (there are likely many). Attach a save game for investigation. Suggestions for quality of life improvements are also welcome. The major objective for 25H2 is to clarify remaining functions.

## License

The source code in this repository is available under the [Sustainable Use License](LICENSE.md).
