// Definitions for the native FastLED/Arduino stubs: the FastLED and Serial globals, plus REAL wall-clock timing.
// Compiled into the viewer build so the library's globals resolve at link time. See FastLED.h / Arduino.h.

#include <chrono>
#include <thread>

#include "FastLED.h"
#include "Arduino.h"

FakeController FastLED;
FakeSerial Serial;

// Real monotonic clock, unlike the library's test stub (which returns a fake counter). The library's refresh-rate
// gate and every time-based effect read micros(), so this must advance at true wall-clock speed for a live preview.
static const std::chrono::steady_clock::time_point g_start = std::chrono::steady_clock::now();

unsigned long micros() {
	auto elapsed = std::chrono::steady_clock::now() - g_start;
	return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
}

unsigned long millis() { return micros() / 1000UL; }

void delay(unsigned long ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
