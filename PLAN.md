# PLAN.md — Kobo Deluxe: Modern macOS Port

**Branch:** `macos-port-SDL2` (all work lands here, small reviewable commits)
**Target:** macOS 26 / Xcode 26 / Apple Silicon (arm64), App Store distribution
**Last updated:** 2026-11-06

## Context

- Kobo Deluxe 0.5.1 (2003, David Olofson / Akira Higuchi) + the 2013 SDL2 macOS
  port. Game code was already ported to SDL2 APIs in 2013.
- Old build (Xcode project compiling vendored SDL2 2.0.1 + SDL2_image 2.0.0 from
  source, prebuilt x86_64 webp.framework) is unbuildable on arm64 + Xcode 26.
- Verified: all 69 game source files compile 0-error against Homebrew SDL2 2.32.10
  + macOS 26.2 SDK, except the two Phase 0 fixes below.
- Upstream web port (CatBroDev, PR #5 + `codex/browser-restoration` branch)
  independently proved the code runs as-is on modern toolchains.

## Decisions (agreed)

| Question | Decision |
|---|---|
| Build system | CMake = single source of truth. Native builds for dev/CLI; `cmake -G Xcode` + `xcodebuild archive` for App Store. Old `kobo.xcodeproj` deleted in Phase 3. |
| Scope | Both `KoboDeluxe.app` and `kobodl` CLI target. |
| Vendored SDL2 2.0.1 / SDL2_image 2.0.0 | Delete from repo (Phase 3). |
| Sandbox code changes | Same codebase, gated by `KOBO_MAS` build flag (default OFF). |
| GLSDL driver | Remove in Phase 3 (default driver is already GFX_DRIVER_SDL2D). |
| SDL2 vs SDL3 | Stay on SDL2 API. Homebrew `sdl2` = sdl2-compat (SDL3 under the hood). Native SDL3 port = deferred optional. |
| Branch hygiene | Work directly on `macos-port-SDL2`. |
| Bundle ID | `org.kobodluxe.KoboDeluxe` (no PII; neutral `org` prefix, echoes the game's `kobodl` binary name). CMake variable `KOBO_BUNDLE_ID`. |

## Phase 0 — Modern-toolchain fixes

- [ ] 0.1 **Owner action:** approve + merge PR #5 (CatBroDev), then `git pull`.
      Six audio tables in `KoboDeluxe-0.5.1/sound/a_struct.h` (lines 34-39) get
      `extern`. Required: Xcode 26 clang defaults to `-fno-common`, so the
      duplicates with definitions in `a_struct.c:31-36` fail at link time.
- [ ] 0.2 `KoboDeluxe-0.5.1/graphics/window.cpp:406`: `char *tok` → `const char *tok`
      (strchr returns `const char*` in C++; the only compile error in the tree).
- [ ] 0.3 Verify: full-tree `clang -fsyntax-only` sweep = 0 errors (baseline already
      established per-file; repeat post-fix as a scriptable check).

## Phase 1 — CMake build, running locally

- [x] 1.1 `brew install sdl2_image` (2.8.12; pulls libpng/libtiff/webp/etc. `sdl2`
      now 2.32.74 via sdl2-compat, plus `sdl3` 3.4.20.)
- [x] 1.2 Write root `CMakeLists.txt` (macOS-only on this branch; clean structure a
      future `web-port` branch can borrow):
      - Targets: `KoboDeluxe` (MACOSX_BUNDLE) + `kobodl` (CLI tool)
      - Sources (69): top-level `cfgform cfgparse dashboard enemies enemy filemap form
        game gamectl gamestate kobo manage map myship options pfile prefs radar random
        scenes score screen sound states` (.cpp) + `logger.c`; `graphics/{cs.c,
        display.cpp, filters.c, gfxengine.cpp, glSDL.c, region.c, sofont.cpp, sprite.c,
        toolkit.cpp, vidmodes.c, window.cpp}`; `eel/{e_builtin e_getargs e_lexer e_path
        e_register e_script e_symtab e_util eel}.c`; `sound/{a_agw a_bus a_channel
        a_commands a_control a_delay a_events a_filters a_globals a_limiter a_midi
        a_midicon a_midifile a_midisock a_patch a_pitch a_plugin a_sequencer a_struct
        a_voice a_wave a_wca audio audiodev sfifo}.c`
      - Includes: `KoboDeluxe-0.5.1`, `.../graphics`, `.../sound`, `.../eel`,
        `.../data/sfx`; define `USE_RESOURCE_WORKING_DIR=1` (app) / `0` (CLI)
      - Link: Homebrew SDL2 + SDL2_image (pkg-config, `/opt/homebrew` prefix + rpath);
        frameworks CoreFoundation, Accelerate, Cocoa
      - Standards: C `gnu11`, C++ `gnu++14`
      - App bundle: Info.plist (from `Kobo Deluxe/Kobo Deluxe-Info.plist`;
        `LSMinimumSystemVersion` 13.0, add `NSHighResolutionCapable`),
        `en.lproj/MainMenu.xib`, `Icon.icns`; copy `KoboDeluxe-0.5.1/data/` →
        `Contents/Resources/data/` (cwd=Resources, so `./data/gfx` + `./data/sfx`
        resolve); CLI: copy `data/` beside binary (old "Copy data resources" behavior)
- [x] 1.3 Build both targets; fix whatever runtime niggles appear.
      (Niggles found & fixed: unquoted space-in-path in CMake resources list;
      modern ibtool emits single-file .nib → `copy` not `copy_directory`;
      player-profile score save — see score-dir fix above.)
- [x] 1.4 Smoke test `.app`: boot → loading screen → title → stage 1 gameplay →
      audio → Esc×5 quit. Smoke test `kobodl` from terminal (same, cwd = build dir).
      (Both verified: 51 waveforms load, title reaches, clean quit. Owner played
      through the .app build.)

## Phase 2 — App Store (MAS) ready

- [ ] 2.1 Entitlements file: `com.apple.security.app-sandbox` (mandatory for MAS).
      New CMake option `KOBO_MAS` (default OFF) gates:
      - ~~`KOBO_SCORE_DIR`~~ **done in Phase 1:** per-user score dir
        (`~/Library/Preferences/KoboDeluxe/scores`) registered first and
        auto-created at startup; sandbox-safe without a flag ($HOME = container
        under MAS). Global shared scoreboard not possible under MAS — document
        the limitation.
      - `save_config()` setgid/getgid block (kobo.cpp:1365-1387): skipped under
        sandbox (would fail).
      - `log.txt` / `screen%d.bmp` output: → Application Support / Temp (sealed
        bundle Resources are read-only).
      - Joystick: default OFF (sandbox blocks SDL's IOKit-HID joysticks;
        keyboard/mouse unaffected).
- [ ] 2.2 Archive flow: `cmake -B build-xcode -G Xcode` →
      `xcodebuild -scheme KoboDeluxe -configuration Release archive` →
      `-exportArchive` with MAS provisioning profile. Verify archived+sandboxed build.
- [ ] 2.3 Credits/license screen (Higuchi 1995-96, Olofson 2001-07, Auerswald 2005,
      port credits; GPL v2 + LGPL notices; public repo = GPL source offer).
      Version bump 0.5.1 → 1.0. Confirm final bundle ID (from shortlist above).

## Phase 3 — Cleanup + modernization

- [x] 3.1 Deletion commit: `SDL2-2.0.1/`, `SDL2_image-2.0.0/` (~85 MB; webp.framework
      is x86_64-only, can never link on arm64) + `kobo.xcodeproj/` (references deleted
      trees; `cmake -G Xcode` takes over its role). `Kobo Deluxe iOS/` stays untouched.
      (Commit `f7f4462`.)
- [x] 3.2 Remove GLSDL driver: `graphics/glSDL.c`, `glSDL.h`, `GFX_DRIVER_GLSDL`
      branches in `gfxengine.cpp`, clamp bug at `prefs.cpp:178`
      (`if(videodriver != GFX_DRIVER_SDL2D) videodriver = GFX_DRIVER_GLSDL;` →
      forces `GFX_DRIVER_SDL2D`). `HAVE_OPENGL` was never defined in this build, so
      glSDL compiled as no-op dummies; no OpenGL/GLU frameworks were linked.
      14 headers/sources that included `glSDL.h` now include `SDL.h` directly.
      (Commit `083f016`.) Re-run Phase 1 smoke test.
- [ ] 3.3 Re-verify full build + App Store archive after deletions.
      (Fresh `cmake -B && cmake --build` from clean tree: 0 errors, `kobodl
      -showcfg` OK. Archive step pending Phase 2 provisioning.)
- [x] 1.5 (added) Config persistence: the `"$HOME/..."` path macros in `kobo.cpp`
      were never expanded, so options were silently written to a literal `$HOME`
      dir in cwd. Now `kobo_home_path()` expands `$HOME` at runtime; config lives at
      `~/Library/Preferences/KoboDeluxe/KoboDeluxe Preferences` (auto-created),
      same base dir as the score dir — sandbox-safe. (Commit `de9e2d0`.)

## Deferred / optional (not scheduled)

- Native SDL3 API migration (~6-file surface: window/renderer split in gfxengine,
  pixel-format enums, audio callback, events, joystick renames). Low urgency —
  sdl2-compat already runs SDL3 underneath.
- Retina/HiDPI scaling option; desktop-fullscreen default instead of exclusive.
- `web-port` branch — CatBroDev's `codex/browser-restoration` branch is the reference;
  hosted game already live at oldgamesarchive.org.
- Joystick under MAS (would need GameController-framework path, not raw IOKit-HID).

## Verification checklist (run at each phase end)

- [ ] `cmake -B build && cmake --build build` — 0 errors, 0 new warnings of note
- [ ] `.app` launches from build dir and plays through stage 1 with audio
- [ ] `kobodl` runs from terminal
- [ ] (Phase 2+) archived MAS build launches and plays in sandbox
- [ ] `git status` clean; commit history tells the story
