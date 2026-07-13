#pragma once

// Host-side (native) stub of the Arduino core for the LedPipelines web previewer. Provides the slice the library
// touches - the String type, a no-op Serial, min()/max(), and the timing calls - plus a few no-op GPIO symbols so a
// user's entry file can be pasted straight from a real Arduino sketch.
//
// CRITICAL: unlike the library's test stub, millis()/micros() here return REAL wall-clock time (see stubs_impl.cpp).
// The library's run() rate-limiter and every time-based effect (Moving, TimeBox, Loop, FadeIn) are driven off
// micros(); a fake counter would freeze or jump the animation. delay() must also really sleep.

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

// The ESP32 Arduino core exposes min()/max() as function templates in the global namespace (not macros, so qualified
// std::min in the library still works). Mirror that so bare min()/max() calls in the library resolve on the host.
// Returns by value (via common_type) so mixed-type comparisons don't dangle a reference to a promoted temporary.
template <typename T, typename U>
typename std::common_type<T, U>::type min(T a, U b) { return a < b ? a : b; }
template <typename T, typename U>
typename std::common_type<T, U>::type max(T a, U b) { return a > b ? a : b; }

// Arduino's String maps cleanly onto std::string for our purposes. We only use construction from numbers/C-strings and
// operator+ concatenation (e.g. in user pipeline sketches' Serial logging).
class String : public std::string {
public:
	String() = default;
	String(const char *s) : std::string(s) {}
	String(const std::string &s) : std::string(s) {}
	String(char c) : std::string(1, c) {}

	// One templated constructor for every arithmetic type instead of a fixed int/long/float/... overload set. The
	// discrete overloads were ambiguous for types with no exact match - e.g. building a String from a uint8_t or a
	// uint64_t, where several integer overloads convert equally well. This is platform-dependent: on macOS
	// unsigned long is 64-bit so uint64_t matched String(unsigned long) exactly, but on Windows (LLP64, long is
	// 32-bit) it does not, so the call became ambiguous. Routing every arithmetic value through std::to_string (which
	// has its own unambiguous per-type overloads) sidesteps that. Promote small integers to a type std::to_string
	// accepts (it has no uint8_t overload) via a common_type with int.
	template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
	String(T v) : std::string(std::to_string(static_cast<typename std::common_type<T, int>::type>(v))) {}
};

// Arduino's String supports `str + number` (and mixed String/const char*) returning a String. std::string's operators
// don't cover the numeric right-hand sides the library concatenates (e.g. `"..." + runtimeMs`), so provide them here
// by stringifying the right operand.
inline String operator+(const String &lhs, const String &rhs) { return String(std::string(lhs) + std::string(rhs)); }
inline String operator+(const String &lhs, const char *rhs) { return String(std::string(lhs) + rhs); }
inline String operator+(const char *lhs, const String &rhs) { return String(std::string(lhs) + std::string(rhs)); }
template <typename T> inline String operator+(const String &lhs, T rhs) { return lhs + String(rhs); }

// A no-op Serial so sketches' println/print calls link without doing anything on the host.
struct FakeSerial {
	void begin(unsigned long) {}
	void print(const String &) {}
	void println(const String &) {}
	void print(const char *) {}
	void println(const char *) {}
};

extern FakeSerial Serial;

// No-op GPIO surface so an entry file copied from a real sketch (pinMode/digitalRead/etc.) still links. digitalRead
// returns HIGH (pull-ups idle high); there are no physical pins on the host.
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return HIGH; }

// Timing: REAL wall-clock, defined in stubs_impl.cpp against std::chrono::steady_clock. See the header note above.
unsigned long millis();
unsigned long micros();
void delay(unsigned long);
