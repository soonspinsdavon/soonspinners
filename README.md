# SoonSpinner

A variable-rate "spin" effect plugin for Soonspins — same core idea as
Soundtoys' Vari-Fi (turntable/tape-style speed changes that couple pitch
and time), built from scratch with JUCE so it runs as VST3 / AU on both
macOS and Windows, plus a Standalone app for quick testing.

This is the MVP core: manual speed control with motor-style glide, two
momentary "Spin Down" / "Spin Up" buttons (also exposed as
DAW/MIDI-automatable parameters), and a Wow/Flutter macro for tape-style
wobble. GUI has a lowercase-styled title and a generated parallax cloud
background in the soonspins orange that continuously drifts left to
right (like soonspins.com), with three layers scrolling/parallaxing at
different speeds for depth. No real branding artwork/logo, no
AAX/Pro Tools support yet, and the exact soonspins.com font isn't
confirmed — those are deliberately deferred (see Roadmap and Open
questions below).

## What's in here

```
SoonSpinner/
  CMakeLists.txt          # JUCE fetched automatically via CMake FetchContent
  Source/
    VariSpinEngine.h/.cpp # the actual DSP: variable-rate circular buffer read + wow/flutter LFOs
    PluginProcessor.h/.cpp# parameters, host glue
    PluginEditor.h/.cpp   # themed GUI: parallax background, lowercase text, placeholder font
    Assets/
      background_clouds_orange.png  # procedurally generated halftone cloud art, embedded as BinaryData
```

## One-time setup

You'll need, per platform you want to build on:

**macOS**
- Xcode (from the App Store) + Xcode Command Line Tools (`xcode-select --install`)
- CMake 3.22+ (`brew install cmake`)
- Git (`brew install git` if not already present)

**Windows**
- Visual Studio 2022 Community, with the "Desktop development with C++" workload
- CMake 3.22+ (bundled with VS 2022, or install separately from cmake.org)
- Git for Windows

You don't need JUCE installed separately — the CMake script pulls it
automatically the first time you configure the project (it's a few
hundred MB, so the first configure will take a few minutes).

## Building

From the `SoonSpinner` folder:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Note the `-DCMAKE_BUILD_TYPE=Release` at configure time — macOS uses a
single-config generator (Makefiles), which only picks up the build type
from that flag, not from `--build --config`. Without it you'll still get
a working plugin, just an unoptimized one landing in a differently-named
output folder. (Windows/Visual Studio is multi-config, so `--config
Release` on the build step is what matters there.)

First build will be slow (cloning + compiling JUCE itself). After that,
incremental builds are fast.

**Fastest way to test changes:** build and run just the Standalone
target — it launches as a normal app with your audio interface, no DAW
required:

```bash
cmake --build build --config Release --target SoonSpinner_Standalone
```

Once you're happy with a change, build the VST3 (and AU on Mac) and
copy them into your plugin folders, or just rely on
`COPY_PLUGIN_AFTER_BUILD TRUE` in the CMakeLists, which does that for
you automatically on each build.

## How the effect works

`VariSpinEngine` writes incoming audio into a circular buffer at a
fixed rate, and reads it back out with a separate "read head" whose
speed can range from 0.25x to 4x. When the read head moves slower than
incoming audio is being written, you hear it slow down and drop in
pitch; faster, and it speeds up and rises in pitch — pitch and time
are coupled, exactly like a physical turntable or tape deck. The
`Speed` knob sets a manual base rate; `Glide` controls how long the
read head takes to arrive at a new target (motor inertia); `Spin Down`
/ `Spin Up` are momentary overrides that push the rate to the Down/Up
Amount knobs while held, and glide back to the base speed on release.

See the comment block at the top of `VariSpinEngine.h` for the known
MVP limitation (a rare glitch under sustained extreme pitch shifts) and
the plan to fix it with a crossfaded dual-read-head.

The Wow/Flutter knob is a single macro (0-100%) that scales a fixed
~0.8Hz "wow" LFO and a fixed ~6Hz "flutter" LFO together, both
multiplying the read rate on top of whatever the Speed/Spin controls
are already doing. Separate rate/depth controls for each are a natural
follow-up once you've heard how this sits.

## Sending test builds to people

CI (`.github/workflows/build.yml`) builds both macOS (VST3 + AU) and
Windows (VST3) on every push, using GitHub's own Mac and Windows
runners — you don't need to own a PC for this. One-time setup:

```bash
cd SoonSpinner
git init
git add .
git commit -m "SoonSpinner MVP"
```

Then create an empty repo on GitHub (github.com/new — don't initialize
it with a README), and push:

```bash
git remote add origin <the repo URL GitHub gives you>
git branch -M main
git push -u origin main
```

Every push now triggers a build automatically (check the **Actions**
tab on GitHub). To hand testers an actual downloadable build, cut a
tagged release:

```bash
git tag v0.1.0-beta1
git push origin v0.1.0-beta1
```

That triggers CI, and once it's green, a **Release** appears on your
repo's GitHub page with `SoonSpinner-macOS.zip` and
`SoonSpinner-Windows.zip` attached — send testers that Release URL.

**What testers do with the zip:**
- macOS: unzip, drop `SoonSpinner.vst3` into `~/Library/Audio/Plug-Ins/VST3/` and `SoonSpinner.component` into `~/Library/Audio/Plug-Ins/Components/`, then rescan plugins in their DAW.
- Windows: unzip, drop `SoonSpinner.vst3` into `C:\Program Files\Common Files\VST3\`, then rescan.

**Unsigned-build warnings to warn testers about**, since we haven't
done code signing/notarization yet (that's a paid Apple/Windows
developer account, on the roadmap below):
- **macOS** will likely refuse to load it the first time with an
  "unidentified developer" warning. Testers can fix this by opening
  Terminal and running:
  `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/SoonSpinner.vst3`
  (and the same for the `.component` path), or via System Settings →
  Privacy & Security → there's usually an "Allow Anyway" button after
  the first blocked attempt.
- **Windows** SmartScreen may flag the downloaded zip itself; "More
  info" → "Run anyway" clears it. The VST3 DLL itself typically loads
  fine once unzipped since DAWs load it directly rather than the OS
  gatekeeping it like an installer.

## Open questions / things to confirm

- **Exact soonspins.com font.** The GUI currently falls back to the
  system monospace font as a placeholder — I couldn't extract the real
  webfont from a plain fetch of the site. Send the font name (or a
  screenshot of the site's text) and I'll embed the actual typeface as
  BinaryData, same way the background art is embedded.
- **Background art colours.** `Source/Assets/background_clouds_orange.png`
  is a from-scratch halftone/dither cloud texture matching the style of
  the reference image you sent, recoloured to an orange-on-dark palette.
  It's not sampled from the actual soonspins.com logo (couldn't pull
  exact pixel colours from a fetch) — flag it if the orange needs to be
  closer to a specific hex.

## Roadmap (next passes, in rough order)

1. **Fix the read-head collision edge case** with a crossfaded second
   read head instead of the current hard "nudge."
2. **Modulation sources** — LFO-driven and envelope-driven spin, not
   just manual/momentary, mirroring Vari-Fi's Source selector.
3. **Real theming** — swap the placeholder font/palette for confirmed
   Soonspins brand assets (logo, exact colours, custom knob art).
4. **AAX/Pro Tools support** — separate track requiring an Avid
   developer agreement and PACE code signing; not needed for VST3/AU.
5. **Code signing & notarization** — Developer ID + notarization on
   macOS, EV certificate on Windows, so installs don't trigger security
   warnings.
6. **Licensing + store** — a license-key/activation scheme and a
   checkout flow (e.g. Gumroad/Paddle/FastSpring) for selling through
   your own Soonspins store, plus an auto-update check in the plugin.

Ping me for any of these whenever you're ready to tackle the next one.
