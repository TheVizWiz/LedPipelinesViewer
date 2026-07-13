// Definitions for the native Arduino stub: the Serial global plus REAL wall-clock timing. Compiled into the viewer
// build so the library's Arduino-side globals resolve at link time. See Arduino.h. (The viewer no longer stubs
// FastLED - LedPipelines renders through viewer::ViewerOutput, an ledpipelines::LedOutput implementation.)

#include <chrono>
#include <thread>

#include "Arduino.h"

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
