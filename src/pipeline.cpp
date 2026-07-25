// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
// EDIT THIS FILE to preview your own effect, then rebuild (`pio run`) and
// rerun. This is the viewer's analog of an Arduino sketch's setup(): register
// your strip(s), initialize LedPipelines, build a pipeline, and the render loop
// below streams every frame to the browser at http://127.0.0.1:8420.
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────

#include "Arduino.h" // viewer host stub (String, millis/micros/delay, Serial)
#include "LedPipelines.h"  // the LedPipelines library (pulled via lib_deps)
#include "viewer/PipelineQueue.h" // publishes the pipeline's serialized JSON to the browser
#include "viewer/Runner.h" // viewer runtime: startServer()
#include "viewer/ViewerOutput.h" // the viewer's LedOutput backend

using namespace ledpipelines;
using namespace ledpipelines::effects;

#define LED_COUNT 100

// The viewer's render backend. Declare your strip(s) on it, register it with
// setOutput(), and every rendered frame is published to the browser. No backing
// pixel array / pin / chipset - the viewer doesn't drive hardware.
viewer::ViewerOutput output;

LedPipelineStage *pipeline;

// setup(): declare strips on the output and register it FIRST, then
// initialize() (it reads the topology from the registered output), then build
// the pipeline.
void buildPipeline() {
  output.addStrip(LED_COUNT);
  ledpipelines::setOutput(&output);

  ledpipelines::initialize();
  ledpipelines::setMaxRefreshRate(60);

  // A 10-pixel white segment sweeping across the strip and looping - exercises
  // the real-clock timing path.

  // auto ball = std::shared_ptr<LedPipelineStage>(
  //     HSVGradient::Builder(0, 9)
  //         .runtimeMs(8000)
  //         .startGradient(FHSVA(0, 1, 1), FHSVA(120, 1, 1))
  //         .endGradient(FHSVA(360, 1, 1), FHSVA(480, 1, 1))
  //         .wrap(Loop::Builder())
  //         .block()
  //         .build());
  //
  // pipeline =
  //     SeriesLedPipeline::Builder()
  //         .addStage(Shared::Builder(ball)
  //                       .wrap(Moving::Builder(2000)
  //                                 .startPosition(0)
  //                                 .endPosition(80)
  //                                 .smoothingFunction(
  //                                     SmoothingFunction::INVERSE_QUADRATIC))
  //                       .timebox(2000))
  //         .addStage(
  //             Shared::Builder(ball)
  //                 .wrap(Moving::Builder(2000)
  //                           .startPosition(80)
  //                           .endPosition(0)
  //                           .smoothingFunction(SmoothingFunction::QUADRATIC))
  //                 .timebox(2000))
  //         .timebox(4000)
  //         .loop()
  //         .build();

  auto particle = SolidSegment::Builder(RGBA::Orange, 1);

  auto factory = [=]() -> LedPipelineStage * {
    auto particle = SolidSegment::Builder(RGBA::Orange, 1);

    auto in = particle.wrap(RandomFadeIn::Builder(2000))
                  .minRuntimeMs(1000)
                  .samplingFunction(SamplingFunction::CENTERED)
                  .terminateOnComplete(true);

    auto wait = particle.wrap(RandomTimeBox::Builder(2000)
                                  .minRuntimeMs(1000)
                                  .samplingFunction(SamplingFunction::CENTERED)
                                  .terminateOnComplete(true));

    auto out = particle.wrap(RandomFadeOut::Builder(2000))
                   .minRuntimeMs(1000)
                   .samplingFunction(SamplingFunction::CENTERED)
                   .terminateOnComplete(true);

    return SeriesLedPipeline::Builder()
        .addStage(in)
        .addStage(wait)
        .addStage(out)
        .wrap(RandomShift::Builder(static_cast<float>(TemporaryLedData::size))
                  .minOffset(0)
                  .samplingFunction(SamplingFunction::UNIFORM)
                  .useWholePixels(true))
        .build();
  };

  auto spawner =
      TimedSpawner::Builder(factory, 1000).maxChildren(50).keepOldOnSpawn(true);

  pipeline = spawner.build();

  pipeline->reset();
}

int main() {
  viewer::startServer(8420);
  buildPipeline();

  // run() self-rate-limits to setMaxRefreshRate; each rendered frame's
  // output.show() publishes pixels to the browser. The small real sleep keeps
  // the CPU idle between frames.
  while (true) {
    pipeline->run();
    // Publish the pipeline's structure + live state each iteration, so the tree view reflects the same moment the
    // pixels do. toJson(true) walks the whole stage tree; PipelineQueue coalesces, so if the browser can't keep up it
    // simply drops intermediate snapshots rather than backing up the loop.
    viewer::PipelineQueue::instance().publish(pipeline->toJson(true));
    delay(1);
  }
}
