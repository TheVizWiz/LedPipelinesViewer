#pragma once

// Host-side (native) stub of FastLED for the LedPipelines web previewer. It provides the surface the library uses - the
// CRGB/CHSV color types (see CRGB.h) and a FastLED controller with the strips registered via addLeds<>() - but instead
// of driving hardware, show() snapshots the rendered pixels and publishes them to the browser via the FrameQueue.
//
// CRGB/CHSV and the full FastLED color API live in CRGB.h (a faithful port of FastLED's color types + all named colors
// + hsv2rgb). CRGB's memory layout there matches what every LedPipelines .cpp was compiled against, so it stays
// ABI-compatible. The CRGB multiply operators (operator*, operator*=) are defined BY THE LIBRARY (LedPipelineUtils) and
// must NOT be declared in the stub, or the link fails with duplicate symbols.

#include <cstdint>
#include <vector>

#include "Arduino.h"
#include "CRGB.h"

#include "viewer/FrameQueue.h"

// Chipset and color-order identifiers. On real FastLED these select hardware timing / channel order; on the host they
// are ignored, but they must be nameable so user code that mirrors a real sketch - FastLED.addLeds<WS2812B, PIN, GRB>
// (...) - compiles. (The library's own test stub omits these because its tests call addLeds() with no template args.)
enum EChipset {
	WS2812B,
	WS2812,
	WS2811,
	NEOPIXEL,
	SK6812,
	APA102,
	WS2801,
	LPD8806,
};

enum EOrder {
	RGB = 0x012,
	RBG = 0x021,
	GRB = 0x102,
	GBR = 0x120,
	BRG = 0x201,
	BGR = 0x210,
};

// A single logical strip: a window into a caller-provided CRGB buffer.
struct FakeStrip {
	CRGB *leds;
	int len;

	int size() const { return len; }
	CRGB &operator[](int i) { return leds[i]; }
};

// The global FastLED controller stand-in: holds the strips registered via addLeds<>().
struct FakeController {
	std::vector<FakeStrip> strips;

	// Template signature mirrors FastLED.addLeds<CHIPSET, PIN, ORDER>(leds, count) and the offset overload. Real
	// FastLED templates these on NON-TYPE params (an EChipset value, a uint8_t pin, an EOrder value), so we use
	// `auto...` non-type template parameters - a plain `typename...` would reject the numeric pin argument. All are
	// ignored on the host.
	template <auto... Args>
	FakeStrip &addLeds(CRGB *leds, int count) {
		strips.push_back(FakeStrip{leds, count});
		return strips.back();
	}

	template <auto... Args>
	FakeStrip &addLeds(CRGB *leds, int offset, int count) {
		strips.push_back(FakeStrip{leds + offset, count});
		return strips.back();
	}

	int count() const { return (int)strips.size(); }
	FakeStrip &operator[](int i) { return strips[i]; }

	void clear() {}

	// The render hook. After the library's populateFastLed(), each strip's buffer holds the final opacity-baked RGB
	// the physical LED would show. Snapshot every strip (preserving per-strip grouping) into a Frame and publish it to
	// the browser. Publishing is a non-blocking mailbox write, so the render loop never waits on the network.
	void show() {
		viewer::Frame frame;
		frame.strips.reserve(strips.size());
		for (auto &strip : strips) {
			std::vector<uint32_t> pixels;
			pixels.reserve(strip.len);
			for (int j = 0; j < strip.len; j++) {
				CRGB c = strip.leds[j];
				pixels.push_back(((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.b);
			}
			frame.strips.push_back(std::move(pixels));
		}
		viewer::FrameQueue::instance().publish(std::move(frame));
	}
};

extern FakeController FastLED;
