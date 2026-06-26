# OpenDeFeedback

**Realtime live-audio cleanup plugin** — adaptive de-feedback, conservative
de-noise and gentle de-reverb for live sound, built on
[iPlug2](https://github.com/iPlug2/iPlug2) with an IGraphics / NanoVG GUI.

> **Status: DSP-only MVP.** All processing is hand-tuned, realtime-safe DSP.
> The machine-learning path is scaffolded (`src/ml/`) but inert — it ships a
> neutral stub and does not affect the audio yet.

OpenDeFeedback is an independent, original project. It does **not** copy any
other product's DSP or UI design. It is MIT-licensed and built to be a clean,
hackable base.

---

## Features (MVP 1)

* **Adaptive feedback suppression** — FFT-free detector + bank of up to 8
  dynamic tracking notch filters.
* **Conservative noise reduction** — 3-band downward expander with a tracked
  noise floor (not a hard gate).
* **Gentle de-reverb** — late-tail ducker that preserves transients.
* **Artifact guard** — caps total attenuation to avoid tonal collapse / pumping.
* **Zero latency** — fully causal, reports 0 samples; no lookahead.
* **Modes** — Full Clean, Feedback Only, Noise Only, Room Only, Live Vocal,
  Speech, Instrument.
* **Dark live-console GUI** — flat vector controls, input/output meters and a
  live detected-frequency / reduction readout.

## Parameters

| Parameter        | Range     | Default | Notes                                   |
|------------------|-----------|---------|-----------------------------------------|
| Strength         | 0.0–1.0   | 1.0     | Global dry/wet blend                    |
| Feedback Amount  | 0.0–1.0   | 1.0     | Detector sensitivity + max notch depth  |
| Noise Amount     | 0.0–1.0   | 0.35    | Band-expander strength                  |
| Room Amount      | 0.0–1.0   | 0.25    | Tail-suppression strength               |
| Artifact Guard   | 0.0–1.0   | 0.75    | 1 = very gentle, 0 = allow heavy cuts   |
| Mode             | enum      | Full Clean |                                      |
| Mute             | bool      | false   |                                         |
| Bypass           | bool      | false   |                                         |

---

## Repository layout

```
OpenDeFeedback/
  CMakeLists.txt           top-level build
  external/iPlug2/         iPlug2 (git submodule)
  src/
    common/                POD params + realtime-safe helpers
    dsp/                   the realtime DSP engine and stages
    ml/                    placeholder tiny-model interfaces (inert)
    ui/                    theme wrapper + flat vector draw primitives
    themes/                Default.json (design-time theme source ONLY)
    generated/             DefaultTheme.h (generated, committed)
    plugin/                iPlug2 host glue + NanoVG GUI + config.h
  tools/                   generate_theme_header.py (build-time codegen)
  resources/               shared font/image assets
  training/                future PyTorch pipeline (stubs)
  docs/                    architecture.md, dsp_design.md
```

### Theme (static, generated — no runtime JSON)

UI colors, spacing, radii and font sizes come from a generated static header,
`src/generated/DefaultTheme.h` (namespace `odf::Theme`). The plugin never reads
or parses JSON at runtime. `src/themes/Default.json` is a **design-time source
only**; regenerate the header after editing it with:

```bash
python tools/generate_theme_header.py            # writes src/generated/DefaultTheme.h
# or, via CMake:
cmake --build build --target OpenDeFeedback-theme
```

The UI uses `InterVariable.ttf` when available and falls back gracefully to a
bundled face otherwise.

See [docs/architecture.md](docs/architecture.md) and
[docs/dsp_design.md](docs/dsp_design.md) for the design.

---

## Building (Windows first)

### 1. Get the source + iPlug2 submodule

```bash
git clone <this-repo> OpenDeFeedback
cd OpenDeFeedback
git submodule update --init --recursive
```

The iPlug2 submodule lives at `external/iPlug2`. The default IGraphics backend
(NanoVG) needs **no** extra dependencies.

### 2. (For VST3) fetch the VST3 SDK — one time

The VST3 SDK is not bundled. From the iPlug2 dependencies dir:

```bash
# Windows (PowerShell)
cd external/iPlug2/Dependencies/IPlug
./download-vst3-sdk.ps1

# macOS / Linux
cd external/iPlug2/Dependencies/IPlug
./download-vst3-sdk.sh
```

If the SDK is absent, CMake simply skips the VST3 target; the **standalone app**
target still builds (handy for compiling/auditioning the DSP).

### 3. Configure & build with CMake

```bash
# from the repo root
cmake -B build -G "Visual Studio 17 2022" -A x64

# VST3
cmake --build build --config Release --target OpenDeFeedback-vst3

# Standalone app (no SDK required)
cmake --build build --config Release --target OpenDeFeedback-app
```

Ninja also works: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`.

The IGraphics renderer/backend can be selected at configure time, e.g.
`-DIGRAPHICS_BACKEND=NANOVG -DIGRAPHICS_RENDERER=GL3` (defaults are sensible per
platform). See `external/iPlug2/Scripts/cmake/IGraphics.cmake`.

### Targets

| Target                   | Output                          |
|--------------------------|---------------------------------|
| `OpenDeFeedback-vst3`    | VST3 plugin (needs VST3 SDK)    |
| `OpenDeFeedback-app`     | Standalone application          |

CLAP / LV2 / AU / AAX are intentionally **not** built in the MVP. CLAP can be
added later by fetching its SDK and adding `CLAP` to `FORMATS` in
`src/plugin/CMakeLists.txt`.

---

## Current limitations

* DSP is intentionally **conservative** — it favours "do no harm" over maximum
  removal. The feedback detector's frequency resolution is coarse (band-limited,
  no FFT) and may not catch very fast onsets.
* Noise reduction and de-reverb are subtle by design; this is not an
  offline-grade restoration suite.
* The ML model is **not implemented** — `TinyModelStub` returns neutral values.
* Tested primarily on Windows; macOS/Linux are kept in mind (portable C++17 +
  iPlug2) but not yet validated here.
* Stereo (≤2 ch) is supported; higher channel counts are clamped.

---

## Roadmap

**MVP 1 (this release)** — iPlug2 VST3 + standalone, NanoVG GUI,
Strength/Mode/Mute/Bypass, DSP feedback detector, adaptive notch bank,
conservative noise reducer, basic de-reverb tail suppressor, output meters.

**MVP 2** — better feedback detector (sub-band frequency refinement), visual
detected-frequency list, preset system, benchmark/test harness.

**MVP 3** — tiny embedded ML feedback/noise/room classifier, embedded quantised
weights, PyTorch training + export pipeline (see [training/](training/README.md)).

---

## License

MIT — see [LICENSE](LICENSE). Bundled third-party components (iPlug2/WDL/NanoVG,
the Roboto font) retain their own permissive licenses.

This project is independent and not affiliated with or derived from any
commercial de-feedback / de-noise product.
