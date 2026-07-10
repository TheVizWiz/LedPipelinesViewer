# LedPipelines Viewer

A desktop previewer for the [LedPipelines](https://registry.platformio.org/libraries/thevizwiz/LedPipelines)
library. Write a pipeline in C++ exactly as you would in an Arduino sketch, build, and watch it animate live in your
browser as a wrapped grid of pixels — no hardware needed.

It works by compiling LedPipelines natively against host-side `FastLED.h` / `Arduino.h` stubs. The library is
unchanged; the only difference from hardware is that `FastLED.show()` streams each rendered frame to the browser
(over Server-Sent Events) instead of latching physical LEDs, and the clock is real wall-clock time so timed effects
animate at their true speed. What you see is the exact RGB the physical LEDs would receive.

This is a standalone tool: it depends on LedPipelines, not the other way around. If you only run effects on hardware,
you never need this.

## Requirements

- [PlatformIO Core](https://platformio.org/install/cli) (`pio`)
- A C++ toolchain (Apple clang / gcc). No CMake required.

### Windows

The viewer builds with the GCC/Clang flag set (`-std=c++17`, `-pthread`), so on Windows use a **GCC toolchain
(MinGW-w64)**, not MSVC. Install it (e.g. via [MSYS2](https://www.msys2.org/) `pacman -S mingw-w64-x86_64-gcc`, or
[w64devkit](https://github.com/skeeto/w64devkit)) and make sure its `g++` is on `PATH` when you run `pio`. The build
links `ws2_32`/`wsock32` automatically on Windows (see `scripts/windows_flags.py`) so cpp-httplib's networking resolves;
no manual flags needed. MSVC (`cl.exe`) is not supported — it rejects the GCC-style flags.

## Quickstart

```sh
git clone <this repo>
cd ledpipelineviewer

# Edit src/pipeline.cpp to build your effect (see below), then:
pio run -t serve              # builds the viewer + LedPipelines, then runs it
```

Open <http://127.0.0.1:8420>. You'll see the pixels update in real time. Stop with `Ctrl-C`. To preview a different
effect, edit `src/pipeline.cpp` and run `pio run -t serve` again — it rebuilds only what changed, then relaunches.

`pio run -t serve` is a one-shot build-and-run. If you'd rather do the two steps separately:

```sh
pio run                       # compiles only
./.pio/build/viewer/program   # runs the last build; prints: serving on http://127.0.0.1:8420
```

## Writing a pipeline

`src/pipeline.cpp` is the one file you edit — the viewer's analog of an Arduino `setup()`:

```cpp
CRGB leds[100];
LedPipelineStage *pipeline;

void buildPipeline() {
    FastLED.addLeds<WS2812B, 5, GRB>(leds, 100);   // register strip(s) FIRST
    ledpipelines::initialize();                    // then initialize (reads strip count/sizes)
    ledpipelines::setMaxRefreshRate(60);

    pipeline = SolidSegment::Builder(CRGB::White, 10)
                   .wrap(Moving::Builder(4000).startPosition(0).endPosition(90))
                   .wrap(Loop::Builder())
                   .build();
    pipeline->reset();
}
```

Register multiple strips with the offset overload (`addLeds<...>(leds, offset, count)`); the browser draws one row
block per strip. Order matters: call `addLeds(...)` before `ledpipelines::initialize()`.

## Testing against a newer LedPipelines

The library is a normal PlatformIO dependency (`lib_deps = thevizwiz/LedPipelines` in `platformio.ini`). To pick up a
newer published version, bump the version there (or run `pio pkg update`) and rebuild.

## How it fits together

- `stubs/FastLED.h`, `stubs/Arduino.h`, `stubs/stubs_impl.cpp` — host substitutes for the hardware. `FastLED.show()`
  publishes a frame; `micros()`/`millis()` are real `std::chrono` time.
- `src/viewer/FrameQueue.*` — a coalescing single-slot mailbox between the render loop and the server, so the render
  loop never blocks on the browser and slow clients just drop frames.
- `src/viewer/Server.*` — an embedded HTTP server (vendored [`cpp-httplib`](https://github.com/yhirose/cpp-httplib))
  serving the page at `/` and the SSE pixel stream at `/stream`.
- `web/index.html` — the browser renderer (embedded into the binary via `scripts/embed_web.sh` → `IndexHtml.h`; run
  that script after editing the page).
- `src/pipeline.cpp` — your effect + the render loop.

The stream frame format is `{"seq":N,"strips":[["#rrggbb",...],...]}`.
