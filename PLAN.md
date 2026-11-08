# PLAN.md — Kobo Deluxe: Modern Port (macOS / iPadOS)

**Branch:** `feature/repo_cleanup_mas` (macOS/MAS work; small reviewable commits via PR)
**Branch `ios-port`:** created (iPadOS work, forked from the macOS line)
**Target:** macOS 26 / Xcode 26 / Apple Silicon (arm64), App Store distribution
**Last updated:** 2026-11-07

## Objectives

1. **MAS-ready SDL2 build** — cleaned-up, sandboxed, App Store distributable.
   Version **1.0**.
2. **Improved audio & graphics** — within the SDL2 API first (Homebrew `sdl2`
   is sdl2-compat: SDL3 runs underneath already). Version **1.1**.
3. **iPadOS port** — multi-touch gesture input mapped into the existing game
   input layer. Version **2.0**, developed on `ios-port`.

## Versioning

- `0.5.1` = David Olofson's last upstream release. Internal `VERSION`
  (`aconfig.h`) stays `0.5.1` **until the 1.0/MAS release**.
- **1.0 starts with MAS** — the number is not used before the sandboxed
  App Store build actually works. Then:
  - **1.0** = MAS-ready, SDL2, CMake, no gameplay changes.
  - **1.1** = audio/graphics improvements.
  - **2.0** = iPadOS release.
- Title screen + CLI banner show the version plus port-credit lines
  (`KOBO_VERSION_CREDIT` in `version.h`), one line per port era:
  - `2013 ruohoruotsi (SDL2 port)` — in since the credit commit.
  - `2026 ...` line — added with the 1.0 release (MAS work).
- At 1.0: bump `aconfig.h` `VERSION` → `"1.0"` (matches CMake project +
  bundle version, already `1.0.0`/`1.0`).

## Decisions (agreed)

| Question | Decision |
|---|---|
| Build system | CMake = single source of truth. Native builds for dev/CLI; `cmake -G Xcode` + `xcodebuild archive` for App Store. Old `kobo.xcodeproj` deleted (Phase 3). |
| Scope | Both `KoboDeluxe.app` and `kobodl` CLI target. |
| Vendored SDL2 2.0.1 / SDL2_image 2.0.0 | Deleted (Phase 3, commit `f7f4462`). |
| Sandbox code changes | Same codebase, gated by `KOBO_MAS` build flag (default OFF). |
| GLSDL driver | Removed (Phase 3, commit `083f016`). Default driver was already `GFX_DRIVER_SDL2D`; `HAVE_OPENGL` was never defined, so it compiled as no-op dummies. |
| SDL2 vs SDL3 | Stay on the **SDL2 API** for 1.0/1.1. Improvements use SDL2 renderer/audio features (vsync, HiDPI, scale quality, audio device/rate). Native SDL3 API migration (~6-file surface) only if a specific feature demands it. |
| Audio | 1.0 ships with the existing 2013 audio untouched (end-to-end MAS first). The "Make It Slap" rework — live stems, game reactivity, real master bus — is the **1.1 headline** (Phase 4). Owner invests weeks in composing the new music. |
| Naming | De-spaced: bundle, resource dir, Info.plist all `KoboDeluxe` (done, Phase 2.4). In-game prose keeps the proper noun "Kobo Deluxe". CMake target + bundle ID were already `KoboDeluxe`. |
| Branch hygiene | macOS/MAS work on this branch. `Kobo Deluxe iOS/` leaves this branch (work continues on `ios-port`). |
| Per-platform code | Shared game code stays shared; platform quirks behind build flags (`__IPHONEOS__`, `KOBO_MAS`). Bigger platform features go in dedicated per-platform adapter files that plug into existing abstractions — e.g. `ios/iosinput.c` translating gestures into the key/button events `gamectl` already consumes. No `#ifdef` soup in game logic. |
| Bundle ID | `org.kobodluxe.KoboDeluxe` (no PII). CMake variable `KOBO_BUNDLE_ID`. |

## Phase 0 — Modern-toolchain fixes (done)

- [x] 0.1 PR #5 (CatBroDev) merged: `extern` on six audio tables in
      `sound/a_struct.h` (Xcode 26 clang `-fno-common`).
- [x] 0.2 `graphics/window.cpp:406`: `char *tok` → `const char *tok`.
- [x] 0.3 Full-tree `clang -fsyntax-only` sweep = 0 errors.

## Phase 1 — CMake build, running locally (done)

- [x] 1.1 `brew install sdl2_image` (2.8.12; `sdl2` 2.32.x via sdl2-compat).
- [x] 1.2 Root `CMakeLists.txt`: targets `KoboDeluxe` (MACOSX_BUNDLE) +
      `kobodl` (CLI); 69 sources → 68 after glSDL removal; Homebrew SDL2 +
      SDL2_image via pkg-config; CoreFoundation/Accelerate/Cocoa frameworks;
      C `gnu11`, C++ `gnu++14`; bundle Info.plist from
      `Kobo Deluxe/Info.plist.in` (`LSMinimumSystemVersion` 13.0);
      `MainMenu.xib` → `.nib` via ibtool; `data/` copied into bundle
      (app) / beside binary (CLI); `DEBUG=1` via `$<$<CONFIG:Debug>>`
      generator expression (works for Makefiles *and* Xcode generator).
- [x] 1.3 Build both targets; runtime fixes: CMake space-in-path quoting;
      modern ibtool single-file .nib (`copy` not `copy_directory`); per-user
      score dir `~/Library/Preferences/KoboDeluxe/scores` (auto-created,
      sandbox-safe; legacy global `/Library/...` kept as read fallback).
- [x] 1.4 Smoke tests: `.app` and `kobodl` boot → title → stage 1 → audio →
      clean quit. Owner played through the `.app` build.
- [x] 1.5 Config persistence: the legacy `"$HOME/..."` path macros in
      `kobo.cpp` were never expanded, so options were silently written to a
      literal `$HOME` dir in cwd. Now `kobo_home_path()` expands `$HOME` at
      runtime; config at `~/Library/Preferences/KoboDeluxe/KoboDeluxe
      Preferences` (auto-created, same base dir as scores, sandbox-safe).
      (Commit `de9e2d0`.)

## Phase 2 — MAS / 1.0 release (current)

- [x] 2.1 `KOBO_MAS` CMake option (default OFF) + `KoboDeluxe.entitlements`
      (`com.apple.security.app-sandbox`), passed to the Xcode project via
      `XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS` (verified in pbxproj):
      - score dir: **already sandbox-safe without a flag** ($HOME = container)
      - `save_config()` setgid/getgid block (kobo.cpp): skipped under sandbox
      - `log.txt` / `screen%d.bmp` output: → `~/Library/Application Support/
        KoboDeluxe/` (auto-created; sealed bundle Resources are read-only).
        Verified: MAS build logs there, non-MAS build keeps legacy cwd
        behavior (new shared `kobopath.{h,cpp}` module).
      - Joystick: default OFF (sandbox blocks SDL's IOKit-HID joysticks)
- [ ] 2.2 Archive flow: `archive-mas.sh <TEAM_ID>` (generated:
      `cmake -G Xcode -DKOBO_MAS=ON` → `xcodebuild archive` →
      `-exportArchive`, method `app-store`). Needs owner's Apple Team ID /
      signed-in developer account. Verify archived+sandboxed build
      launches/plays. (Completes Phase 3.3.)
- [ ] 2.3 Credits/license screen (Higuchi 1995-96, Olofson 2001-07,
      Auerswald 2005, port credits; GPL v2 + LGPL notices; public repo = GPL
      source offer). `Kobo Deluxe/en.lproj/Credits.rtf` already exists and is
      bundled — verify/extend content.
- [x] 2.4 De-space rename: bundle is now `KoboDeluxe.app` (`OUTPUT_NAME`,
      `CFBundleName`), resource dir `Kobo Deluxe/` → `KoboDeluxe/`. In-game
      display strings keep the proper noun "Kobo Deluxe". (One commit.)
- [ ] 2.5 Version: bump `aconfig.h` `VERSION` → `"1.0"`; add the 2026
      credit line in `version.h`; tag `v1.0`.
- [x] 2.6 Branch hygiene: `ios-port` branch created from this HEAD;
      `Kobo Deluxe iOS/` removed from the macOS branch (commit `ad06bd1`).
- [ ] 2.7 **Pre-release polish (pulled from Phase 4):** the 1.0 must
      *feel* right on a modern Mac. In owner QC:
      - vsync wired (`SDL_RENDERER_PRESENTVSYNC`, was a no-op; pref
        default ON + "Vertical Retrace Sync" restored to options)
      - resizable window (renderer scales the 1024×768 texture,
        nearest-neighbor)
      - fullscreen fixed: option was dead code (flag never applied); now
        `SDL_WINDOW_FULLSCREEN_DESKTOP`
      - texture/renderer/window destroyed on video restart (was a leak)

## Phase 3 — Cleanup (done)

- [x] 3.1 Delete `SDL2-2.0.1/`, `SDL2_image-2.0.0/`, `kobo.xcodeproj/`
      (~85 MB). (Commit `f7f4462`.)
- [x] 3.2 Remove glSDL driver (~2.3k lines): `glSDL.c/h` deleted,
      `GFX_DRIVER_GLSDL` branches removed, `prefs.cpp` clamp now forces
      `GFX_DRIVER_SDL2D`, 14 files include `SDL.h` directly.
      (Commit `083f016`.)
- [ ] 3.3 Re-verify full build + App Store archive after deletions.
      (Fresh `cmake -B && cmake --build` from clean tree: 0 errors,
      `kobodl -showcfg` OK. Archive step = Phase 2.2.)

## Phase 4 — Audio & graphics improvements / 1.1 (after 1.0)

Order (owner): ship 1.0 to MAS first — end-to-end — then come back here
and invest the weeks.

Graphics (rendering is already SDL2 renderer-based: software surface →
streaming texture → window):
- [x] 4.1 **Vsync** — pulled forward into 1.0 (Phase 2.7, in owner QC):
      `SDL_RENDERER_PRESENTVSYNC` per pref, pref default ON, options
      entry restored.
- [ ] 4.2 **HiDPI/Retina**: render at drawable (pixel) size, not point size —
      crisp text/sprites on Retina; verify `NSHighResolutionCapable` path.
- [ ] 4.3 Render scale quality hint (`SDL_HINT_RENDER_SCALE_QUALITY`) matched
      to the existing software scale modes; evaluate dropping the old
      software dither/scale filters if GPU scaling looks better.
- [ ] 4.4 Re-evaluate `broken_rgba8`/dither paths on modern GPUs (likely
      deletable).

### Audio — "Make It Slap" (the 1.1 headline)

Current state (code-verified 2026-11-07):
- In-game BGM = **pre-baked rendered loop** — `music.agw` loaded as a
  plain waveform (`sound.cpp:100`); `music_c.agw` is the alt variant.
  Not live-synthesized.
- Live MIDI/sequencer path effectively disabled: `ui_music`
  (`sound.cpp:605`) forces `AFX_NONE` under a comment reading "KLUDGE
  UNTIL MIDI SONGS AND FX CONTROL IS FIXED".
- 16-bit fixed-point internal processing, 44.1 kHz, 2001-era limiter;
  positional SFX is plain L/R pan + crude attenuation (`g_play`,
  `sound.cpp:364`).
- The engine already ships a 16-tap feedback delay w/ LPF (`a_delay.c`)
  and an echo FX slot (`AFX_ECHO`) — both currently unused.
- 51 `.agw` SFX waveforms in `data/sfx/`.

Core workstream (scheduled for 1.1):
- [ ] 4.5 Output pipeline: float32 internal path (SDL `AUDIO_F32` where
      the engine allows), 48 kHz option; expose device/rate/buffer size
      in options + prefs; verify no dropouts under load.
- [ ] 4.6 Master bus: look-ahead limiter (replace the 2001-era one),
      gentle saturation for glue, reverb + delay sends (enable the
      existing `a_delay`); **sidechain the music bus to the kick** —
      the techno pump is 80% of "slap" for this genre.
- [ ] 4.7 Music: kill the pre-baked loop → live-sequenced layered
      **stems** (pads → +bass → +drums → +lead/fx). 2–4 new original
      tracks in the early-90s electronica spirit of `trance1.mid`, or
      "2026 remix" arrangements of the existing material. Owner
      composes — the weeks-scale item.
- [ ] 4.8 Game-reactive layering: stem crossfades on wave transition,
      boss intro, low health, bomb (duck + fill); pause = filter sweep
      down, not a hard stop. The single biggest "premium game" tell.
- [ ] 4.9 SFX pass: A/B all 51 waveforms (verify all load; CatBroDev's
      web port is the "known-good" reference), re-render the duds with
      modern synth voices (909-style kick, wide claps, lasers); layered
      one-shots (explosion = body + crunch + tail); UI juice pass
      (score pops, combo escalation).
- [ ] 4.10 Positional upgrade: real distance attenuation curve,
      low-pass with distance, short stereo early reflections instead of
      raw pan.
- [ ] 4.11 Loudness targets (LUFS) per platform; A/B build flag
      `CLASSIC` vs `DELUXE` audio so the retro feel stays an option.
- [ ] 4.12 Ship: no clicks on open, underrun log, CPU profile (M1 +
      iPad), re-run smoke tests, owner playtest. Tag `v1.1`.

Stretch (the 10×-budget fantasy — recorded, not scheduled):
- Full re-orchestration of the entire soundtrack; music tempo/energy
  tracking wave progression in real time.
- HRTF / true binaural spatial audio on the iPad (2.0).
- Per-object panners and per-sound effect chains.
- Revisit after 1.1 lands: which stretch items earned their keep.

## Phase 5 — iPadOS / 2.0 (on `ios-port`)

Fork `ios-port` from the macOS line after 1.0 (so it inherits CMake, the
config/score fixes, and cleanup). `Kobo Deluxe iOS/` (2013 starting point)
moves there.
- [ ] 5.1 CMake iOS target (or `cmake -G Xcode` with iOS platform) building
      from the same shared sources; iOS Info.plist + provisioning.
- [ ] 5.2 **Gesture input adapter**: new `ios/iosinput.c` (iOS-only source)
      mapping multi-touch gestures (swipe/drag = ship movement, tap = fire,
      pinch = zoom?, two-finger = pause) into the same virtual key/button
      events `gamectl` consumes — keyboard/mouse code untouched.
      (Owner has early gesture-mapping experiments to bring in.)
- [ ] 5.3 Touch UI review: menu hit targets, HUD layout at 4:3 / 3:2 iPad
      sizes, landscape default (existing `DEFAULT_JOY_*` landscape constants
      suggest the 2013 port already oriented for landscape).
- [ ] 5.4 No App Sandbox concerns on iOS (containers are the norm);
      score dir under the container already works via $HOME.
- [ ] 5.5 Archive + App Store submission flow; tag `v2.0`.

## Deferred / optional (not scheduled)

- Native SDL3 API migration (~6-file surface: window/renderer split in
  gfxengine, pixel-format enums, audio callback, events, joystick renames).
  Low urgency — sdl2-compat already runs SDL3 underneath; revisit if a
  Phase 4/5 feature needs it.
- Retina/HiDPI scaling *option* (vs 4.2 doing it unconditionally).
- `web-port` branch — CatBroDev's `codex/browser-restoration` is the
  reference; hosted game live at oldgamesarchive.org.
- Joystick under MAS (would need GameController framework, not IOKit-HID).

## Verification checklist (run at each phase end)

- [ ] `cmake -B build && cmake --build build` — 0 errors, 0 new warnings of note
- [ ] `.app` launches from build dir and plays through stage 1 with audio
- [ ] `kobodl` runs from terminal
- [ ] (Phase 2+) archived MAS build launches and plays in sandbox
- [ ] (Phase 2+) options change → quit → `KoboDeluxe Preferences` file appears
      in `~/Library/Preferences/KoboDeluxe/` and is reloaded next launch
- [ ] `git status` clean; commit history tells the story
