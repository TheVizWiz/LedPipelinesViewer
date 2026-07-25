# LedPipelines Viewer

A desktop previewer for the [LedPipelines](https://registry.platformio.org/libraries/thevizwiz/LedPipelines)
library. Write a pipeline in C++ exactly as you would in an Arduino sketch, build, and watch it animate live in your
browser as a wrapped grid of pixels — no hardware needed.

It works by compiling LedPipelines natively and rendering through `viewer::ViewerOutput` — an implementation of the
library's `ledpipelines::LedOutput` backend interface. The library is unchanged; the only difference from hardware is
that instead of latching physical LEDs, the backend streams each rendered frame to the browser (over Server-Sent
Events), and the clock is real wall-clock time so timed effects animate at their true speed. What you see is the exact
RGB the physical LEDs would receive.

(The viewer used to stub out `FastLED.h`; since LedPipelines 0.2.0 decoupled from FastLED behind the `LedOutput`
interface, the viewer is just a backend implementation and needs no FastLED at all.)

This is a standalone tool: it depends on LedPipelines, not the other way around. If you only run effects on hardware,
you never need this.

## Architecture (two processes)

The viewer is split into a data backend and a UI, which run as separate processes:

- **C++ backend** (`http://127.0.0.1:8420`) — compiles LedPipelines, runs the render loop, and serves two Server-Sent
  Events streams and nothing else: `/stream` (rendered pixels) and `/pipeline` (the pipeline's serialized structure +
  live state). It serves no HTML.
- **Vite/React frontend** (`http://127.0.0.1:5173`) — a TypeScript React app under `web/` that connects to those
  streams and renders them. In dev it proxies `/stream` and `/pipeline` to the backend, so there is no CORS to
  configure. It has two tabs:
  - **pixels** — the live pixel grid with bloom/size controls.
  - **tree** — the pipeline drawn as a live tree of nested boxes (wrappers wrap their inner; pipelines and spawners
    hold their children), colored by running state (orange = RUNNING, dimmed = DONE) with a **details** toggle for
    per-stage config/state fields. Powered by the library's `toJson()` serialization (enabled via the
    `-D LEDPIPELINES_SERIALIZATION` build flag).

## Requirements

- [PlatformIO Core](https://platformio.org/install/cli) (`pio`) — builds and runs the C++ backend.
- A C++ toolchain (Apple clang / gcc). No CMake required.
- [Node.js](https://nodejs.org/) 18+ and `npm` — builds and serves the Vite/React UI.

### Windows

The viewer builds with the GCC/Clang flag set (`-std=c++17`, `-pthread`), so on Windows use a **GCC toolchain
(MinGW-w64)**, not MSVC. Install it (e.g. via [MSYS2](https://www.msys2.org/) `pacman -S mingw-w64-x86_64-gcc`, or
[w64devkit](https://github.com/skeeto/w64devkit)) and make sure its `g++` is on `PATH` when you run `pio`. The build
links `ws2_32`/`wsock32` automatically on Windows (see `scripts/build_setup.py`) so cpp-httplib's networking resolves;
no manual flags needed. MSVC (`cl.exe`) is not supported — it rejects the GCC-style flags.

## Quickstart

```sh
git clone <this repo>
cd ledpipelineviewer

# One-time: install the UI's dependencies.
cd web && npm install && cd ..

# Edit src/pipeline.cpp to build your effect (see below), then:
pio run -t serve              # builds/runs the C++ backend AND launches the Vite UI
```

`pio run -t serve` builds the C++ backend, starts the Vite dev server (which opens your browser at
<http://127.0.0.1:5173>), then runs the backend on `:8420`. The UI connects to the backend automatically. Stop with
`Ctrl-C` — that stops the backend and tears down Vite with it. To preview a different effect, edit `src/pipeline.cpp`
and run `pio run -t serve` again. (`src/pipeline.cpp` is always recompiled, so the running binary always matches your
current source.)

If `web/node_modules` is missing, `serve` stops with a reminder to run `npm install` in `web/` first — it does not
install dependencies for you.

### Running the pieces separately

```sh
# Terminal 1 — the C++ backend (SSE streams on :8420):
pio run && ./.pio/build/viewer/program

# Terminal 2 — the UI (opens the browser on :5173, proxies to the backend):
cd web && npm run dev
```

The Vite dev server gives you hot-reload on the UI. The backend must be running for the streams to have data.

## Writing a pipeline

`src/pipeline.cpp` is the one file you edit — the viewer's analog of an Arduino `setup()`. Declare your strip(s) on a
`ViewerOutput`, register it with `setOutput()`, then `initialize()`:

```cpp
viewer::ViewerOutput output;   // the viewer's LedOutput backend (no pin/chipset — it doesn't drive hardware)
LedPipelineStage *pipeline;

void buildPipeline() {
    output.addStrip(100);                          // declare strip(s) FIRST
    ledpipelines::setOutput(&output);              // register the backend
    ledpipelines::initialize();                    // then initialize (reads strip count/sizes from the output)
    ledpipelines::setMaxRefreshRate(60);

    pipeline = SolidSegment::Builder(RGBA::White, 10)
                   .wrap(Moving::Builder(4000).startPosition(0).endPosition(90))
                   .wrap(Loop::Builder())
                   .build();
    pipeline->reset();
}
```

Register multiple strips by calling `output.addStrip(count)` once per strip; the browser draws one row block per strip.
Order matters: call `addStrip(...)` and `setOutput(...)` before `ledpipelines::initialize()`.

## Testing against a newer LedPipelines

The library is a normal PlatformIO dependency (`lib_deps = thevizwiz/LedPipelines` in `platformio.ini`). To pick up a
newer published version, bump the version there (or run `pio pkg update`) and rebuild.

## How it fits together

**Backend (C++):**

- `src/pipeline.cpp` — your effect + the render loop. Each iteration renders a frame and publishes both the pixels
  (via `ViewerOutput`) and the pipeline's `toJson(true)` (via `PipelineQueue`).
- `src/viewer/ViewerOutput.h` — the `ledpipelines::LedOutput` backend: each frame's pixels are packed and published to
  the browser on `show()`, instead of latching hardware.
- `src/viewer/FrameQueue.*` / `src/viewer/PipelineQueue.*` — coalescing single-slot mailboxes between the render loop
  and the server (one for pixels, one for the pipeline JSON), so the render loop never blocks on the browser and slow
  clients just drop intermediate snapshots.
- `src/viewer/Server.*` — an embedded HTTP server (vendored [`cpp-httplib`](https://github.com/yhirose/cpp-httplib))
  serving the two SSE streams: `/stream` (pixels) and `/pipeline` (pipeline structure + state). No HTML.
- `stubs/Arduino.h`, `stubs/stubs_impl.cpp` — host substitutes for the Arduino surface the library expects (`String`,
  `Serial`, and timing): `micros()`/`millis()` are real `std::chrono` wall-clock time. No FastLED stub is needed.
- `scripts/build_setup.py` — pre-build setup (compiles the out-of-tree stubs, links winsock on Windows, and always
  forces `pipeline.cpp` to recompile so the binary matches your current source).

**Frontend (`web/`, Vite + React + TypeScript):**

- `src/App.tsx` — the tab shell (pixels / tree).
- `src/PixelView.tsx` — the live pixel grid with bloom/size controls (`/stream`).
- `src/TreeView.tsx` — the live pipeline tree with running-state coloring and the details toggle (`/pipeline`).
- `src/useSSE.ts` — a small hook wrapping `EventSource` for either stream.
- `vite.config.ts` — dev server config, including the proxy of `/stream` and `/pipeline` to the backend on `:8420`.

**Stream formats:**

- `/stream`: `{"seq":N,"strips":[["#rrggbb",...],...]}`
- `/pipeline`: `{"seq":N,"pipeline":<stage JSON>}`, where the stage JSON is what `LedPipelineStage::toJson(true)`
  produces (nested structure + per-stage config and `state`).
