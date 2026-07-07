// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// EDIT THIS FILE to preview your own effect, then rebuild (`pio run`) and
// rerun. This is the viewer's analog of an Arduino sketch's setup(): register
// your strip(s), initialize LedPipelines, build a pipeline, and the render loop
// below streams every frame to the browser at http://127.0.0.1:8420.
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────

#include "Arduino.h"       // viewer host stub
#include "FastLED.h"       // viewer host stub
#include "LedPipelines.h"  // the LedPipelines library (pulled via lib_deps)
#include "viewer/Runner.h" // viewer runtime: startServer()

using namespace ledpipelines;
using namespace ledpipelines::effects;

// The backing pixel buffer. Size it to your strip; the array is what addLeds()
// windows into.
CRGB leds[100];

LedPipelineStage *pipeline;

// setup(): register strips FIRST, then initialize() (it reads
// FastLED.count()/size()), then build the pipeline.
void buildPipeline() {
  FastLED.addLeds<WS2812B, 5, GRB>(leds, 100);

  ledpipelines::initialize();
  ledpipelines::setMaxRefreshRate(60);

  // A 10-pixel white segment sweeping across the strip and looping - exercises
  // the real-clock timing path.

  auto rgb = SeriesLedPipeline::Builder()
  .addStage(
    SolidSegment::Builder(CRGB::Red, 10).wrap(TimeBox::Builder(1000))
    )
  .addStage(
  SolidSegment::Builder(CRGB::Lime, 10).wrap(TimeBox::Builder(1000))
  )
  .addStage(
  SolidSegment::Builder(CRGB::Blue, 10).wrap(TimeBox::Builder(1000))
  )
  .wrap(Loop::Builder());

  pipeline =  rgb
                 .wrap(Repeat::Builder(100))
                 .wrap(Moving::Builder(21000).startPosition(-20).endPosition(80))
                 .wrap(Loop::Builder())
                 .build();

  pipeline->reset();
}

int main() {
  viewer::startServer(8420);
  buildPipeline();

  // run() self-rate-limits to setMaxRefreshRate; each rendered frame's
  // FastLED.show() pushes pixels to the browser. The small real sleep keeps the
  // CPU idle between frames.
  while (true) {
    pipeline->run();
    delay(1);
  }
}
