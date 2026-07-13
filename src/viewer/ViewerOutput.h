#pragma once

#include <cstdint>
#include <vector>

#include "LedPipelines.h"  // ledpipelines::LedOutput, RGBA

#include "viewer/FrameQueue.h"

namespace viewer {
	// The viewer's LedPipelines backend. Instead of driving hardware, it collects each rendered frame's pixels and
	// publishes them to the browser over the FrameQueue. This is the whole reason the viewer no longer stubs FastLED:
	// the library renders through ledpipelines::LedOutput, so the viewer is just an implementation of that interface.
	//
	// Strips are declared with addStrip(count) - there is no data pin, chipset, or color order, because none of that
	// means anything on the host. Register the output with ledpipelines::setOutput() before ledpipelines::initialize()
	// (which reads the topology from it).
	class ViewerOutput : public ledpipelines::LedOutput {
	public:
		// Declare a strip of `count` pixels. Call once per strip, before initialize(). Returns the strip's index.
		int addStrip(int count) {
			strips.push_back(std::vector<uint32_t>(count, 0));
			return (int)strips.size() - 1;
		}

		int stripCount() const override {
			return (int)strips.size();
		}

		int stripSize(int strip) const override {
			return (int)strips[strip].size();
		}

		// Reset the pending frame to all-off before the library re-renders it. Buffer-only (matches LedOutput's
		// contract); show() is what publishes.
		void clear() override {
			for (auto &strip : strips) {
				for (auto &pixel : strip) pixel = 0;
			}
		}

		// `color` is the final, opacity-baked RGB from populate(). Pack into 0xRRGGBB for the browser (alpha is not
		// sent - it has already been baked into the RGB by the library). RGBA is in the global namespace (see Color.h).
		void setPixel(int strip, int indexInStrip, RGBA color) override {
			strips[strip][indexInStrip] = ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | (uint32_t)color.b;
		}

		// Publish the finished frame to the browser. A non-blocking mailbox write, so the render loop never waits on
		// the network (slow/absent clients just drop frames).
		void show() override {
			Frame frame;
			frame.strips = strips;  // copy the current per-strip pixels into a frame snapshot
			FrameQueue::instance().publish(std::move(frame));
		}

	private:
		// Per-strip pixel buffers (packed 0xRRGGBB), grown by addStrip(). The Frame published to the browser is a
		// snapshot of these.
		std::vector<std::vector<uint32_t> > strips;
	};
} // namespace viewer
