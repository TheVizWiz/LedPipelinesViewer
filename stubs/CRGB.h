#pragma once

// Host-side (native) port of FastLED's CRGB / CHSV color types for the LedPipelines web previewer.
//
// The goal is that any FastLED way of *creating or manipulating a color* works in the viewer exactly as it would on
// hardware: named colors (all 148 HTML names + FastLED's extras), hex codes, RGB triples, HSV via CHSV, and the common
// CRGB methods (nscale8, fadeToBlackBy, %, lerp8, setHSV, ...). The color VALUES and the hsv2rgb_rainbow algorithm are
// copied verbatim from the pinned FastLED release (3.1.x) so previews match on-device output.
//
// This does NOT include the wider library (palettes, fill_gradient, blur1d, CRGBSet, noise). It is the color-creation
// surface only. The CRGB memory layout (the r/g/b + red/green/blue union) is identical to what LedPipelines was
// compiled against, so it stays ABI-compatible. The multiply operators operator*(CRGB,CRGB)/operator*(CRGB,float)/*=
// are defined by LedPipelines (LedPipelineUtils) and are intentionally NOT declared here.

#include <cstdint>

// ── scale8 family (from FastLED lib8tion, non-AVR / SCALE8_FIXED==1 path) ──────────────────────────────────────────
// Fixed-point 8-bit scaling: scale8(i, s) = i * s / 255, with the +1 fixed-point correction FastLED uses by default.
static inline uint8_t stub_scale8(uint8_t i, uint8_t scale) {
	return (uint8_t)(((uint16_t)i * (uint16_t)(scale) + (uint16_t)i) >> 8);
}
// scale8_video: like scale8 but guarantees a non-zero input stays non-zero (used for brightness/saturation ramps).
static inline uint8_t stub_scale8_video(uint8_t i, uint8_t scale) {
	return (uint8_t)((((int)i * (int)scale) >> 8) + ((i && scale) ? 1 : 0));
}

struct CRGB;

/// HSV color, matching FastLED's CHSV (hue/sat/val in 0..255, hue wrapping the full circle).
struct CHSV {
	union {
		struct {
			union { uint8_t hue; uint8_t h; };
			union { uint8_t saturation; uint8_t sat; uint8_t s; };
			union { uint8_t value; uint8_t val; uint8_t v; };
		};
		uint8_t raw[3];
	};

	CHSV() : h(0), s(0), v(0) {}
	CHSV(uint8_t ih, uint8_t is, uint8_t iv) : h(ih), s(is), v(iv) {}
	CHSV(const CHSV &o) = default;
	CHSV &operator=(const CHSV &o) = default;
};

// hsv2rgb_rainbow, copied from FastLED (the default HSV->RGB conversion). Declared here, defined after CRGB.
inline void stub_hsv2rgb_rainbow(const CHSV &hsv, CRGB &rgb);

struct CRGB {
	// FastLED exposes the channels under two names (r/g/b and red/green/blue) via a union; the library uses both.
	union {
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};
		struct {
			uint8_t red;
			uint8_t green;
			uint8_t blue;
		};
		uint8_t raw[3];
	};

	// The full FastLED HTML color table (all 148 CSS/X11 names + FastLED's FairyLight and TCL extensions), values
	// copied verbatim from crgb.h so CRGB::Green (0x008000), CRGB::Lime (0x00FF00), etc. match hardware exactly.
	typedef enum {
		AliceBlue = 0xF0F8FF,
		Amethyst = 0x9966CC,
		AntiqueWhite = 0xFAEBD7,
		Aqua = 0x00FFFF,
		Aquamarine = 0x7FFFD4,
		Azure = 0xF0FFFF,
		Beige = 0xF5F5DC,
		Bisque = 0xFFE4C4,
		Black = 0x000000,
		BlanchedAlmond = 0xFFEBCD,
		Blue = 0x0000FF,
		BlueViolet = 0x8A2BE2,
		Brown = 0xA52A2A,
		BurlyWood = 0xDEB887,
		CadetBlue = 0x5F9EA0,
		Chartreuse = 0x7FFF00,
		Chocolate = 0xD2691E,
		Coral = 0xFF7F50,
		CornflowerBlue = 0x6495ED,
		Cornsilk = 0xFFF8DC,
		Crimson = 0xDC143C,
		Cyan = 0x00FFFF,
		DarkBlue = 0x00008B,
		DarkCyan = 0x008B8B,
		DarkGoldenrod = 0xB8860B,
		DarkGray = 0xA9A9A9,
		DarkGrey = 0xA9A9A9,
		DarkGreen = 0x006400,
		DarkKhaki = 0xBDB76B,
		DarkMagenta = 0x8B008B,
		DarkOliveGreen = 0x556B2F,
		DarkOrange = 0xFF8C00,
		DarkOrchid = 0x9932CC,
		DarkRed = 0x8B0000,
		DarkSalmon = 0xE9967A,
		DarkSeaGreen = 0x8FBC8F,
		DarkSlateBlue = 0x483D8B,
		DarkSlateGray = 0x2F4F4F,
		DarkSlateGrey = 0x2F4F4F,
		DarkTurquoise = 0x00CED1,
		DarkViolet = 0x9400D3,
		DeepPink = 0xFF1493,
		DeepSkyBlue = 0x00BFFF,
		DimGray = 0x696969,
		DimGrey = 0x696969,
		DodgerBlue = 0x1E90FF,
		FireBrick = 0xB22222,
		FloralWhite = 0xFFFAF0,
		ForestGreen = 0x228B22,
		Fuchsia = 0xFF00FF,
		Gainsboro = 0xDCDCDC,
		GhostWhite = 0xF8F8FF,
		Gold = 0xFFD700,
		Goldenrod = 0xDAA520,
		Gray = 0x808080,
		Grey = 0x808080,
		Green = 0x008000,
		GreenYellow = 0xADFF2F,
		Honeydew = 0xF0FFF0,
		HotPink = 0xFF69B4,
		IndianRed = 0xCD5C5C,
		Indigo = 0x4B0082,
		Ivory = 0xFFFFF0,
		Khaki = 0xF0E68C,
		Lavender = 0xE6E6FA,
		LavenderBlush = 0xFFF0F5,
		LawnGreen = 0x7CFC00,
		LemonChiffon = 0xFFFACD,
		LightBlue = 0xADD8E6,
		LightCoral = 0xF08080,
		LightCyan = 0xE0FFFF,
		LightGoldenrodYellow = 0xFAFAD2,
		LightGreen = 0x90EE90,
		LightGrey = 0xD3D3D3,
		LightPink = 0xFFB6C1,
		LightSalmon = 0xFFA07A,
		LightSeaGreen = 0x20B2AA,
		LightSkyBlue = 0x87CEFA,
		LightSlateGray = 0x778899,
		LightSlateGrey = 0x778899,
		LightSteelBlue = 0xB0C4DE,
		LightYellow = 0xFFFFE0,
		Lime = 0x00FF00,
		LimeGreen = 0x32CD32,
		Linen = 0xFAF0E6,
		Magenta = 0xFF00FF,
		Maroon = 0x800000,
		MediumAquamarine = 0x66CDAA,
		MediumBlue = 0x0000CD,
		MediumOrchid = 0xBA55D3,
		MediumPurple = 0x9370DB,
		MediumSeaGreen = 0x3CB371,
		MediumSlateBlue = 0x7B68EE,
		MediumSpringGreen = 0x00FA9A,
		MediumTurquoise = 0x48D1CC,
		MediumVioletRed = 0xC71585,
		MidnightBlue = 0x191970,
		MintCream = 0xF5FFFA,
		MistyRose = 0xFFE4E1,
		Moccasin = 0xFFE4B5,
		NavajoWhite = 0xFFDEAD,
		Navy = 0x000080,
		OldLace = 0xFDF5E6,
		Olive = 0x808000,
		OliveDrab = 0x6B8E23,
		Orange = 0xFFA500,
		OrangeRed = 0xFF4500,
		Orchid = 0xDA70D6,
		PaleGoldenrod = 0xEEE8AA,
		PaleGreen = 0x98FB98,
		PaleTurquoise = 0xAFEEEE,
		PaleVioletRed = 0xDB7093,
		PapayaWhip = 0xFFEFD5,
		PeachPuff = 0xFFDAB9,
		Peru = 0xCD853F,
		Pink = 0xFFC0CB,
		Plaid = 0xCC5533,
		Plum = 0xDDA0DD,
		PowderBlue = 0xB0E0E6,
		Purple = 0x800080,
		Red = 0xFF0000,
		RosyBrown = 0xBC8F8F,
		RoyalBlue = 0x4169E1,
		SaddleBrown = 0x8B4513,
		Salmon = 0xFA8072,
		SandyBrown = 0xF4A460,
		SeaGreen = 0x2E8B57,
		Seashell = 0xFFF5EE,
		Sienna = 0xA0522D,
		Silver = 0xC0C0C0,
		SkyBlue = 0x87CEEB,
		SlateBlue = 0x6A5ACD,
		SlateGray = 0x708090,
		SlateGrey = 0x708090,
		Snow = 0xFFFAFA,
		SpringGreen = 0x00FF7F,
		SteelBlue = 0x4682B4,
		Tan = 0xD2B48C,
		Teal = 0x008080,
		Thistle = 0xD8BFD8,
		Tomato = 0xFF6347,
		Turquoise = 0x40E0D0,
		Violet = 0xEE82EE,
		Wheat = 0xF5DEB3,
		White = 0xFFFFFF,
		WhiteSmoke = 0xF5F5F5,
		Yellow = 0xFFFF00,
		YellowGreen = 0x9ACD32,

		FairyLight = 0xFFE42D,
		FairyLightNCC = 0xFF9D2A,

		Gray0 = 0x000000,
		Gray10 = 0x1A1A1A,
		Gray25 = 0x404040,
		Gray50 = 0x7F7F7F,
		Gray75 = 0xBFBFBF,
		Gray100 = 0xFFFFFF,
		Grey0 = 0x000000,
		Grey10 = 0x1A1A1A,
		Grey25 = 0x404040,
		Grey50 = 0x7F7F7F,
		Grey75 = 0xBFBFBF,
		Grey100 = 0xFFFFFF,

		Red1 = 0xFF0000,
		Red2 = 0xEE0000,
		Red3 = 0xCD0000,
		Red4 = 0x8B0000,
		Green1 = 0x00FF00,
		Green2 = 0x00EE00,
		Green3 = 0x00CD00,
		Green4 = 0x008B00,
		Blue1 = 0x0000FF,
		Blue2 = 0x0000EE,
		Blue3 = 0x0000CD,
		Blue4 = 0x00008B,
		Orange1 = 0xFFA500,
		Orange2 = 0xEE9A00,
		Orange3 = 0xCD8500,
		Orange4 = 0x8B5A00,
		Yellow1 = 0xFFFF00,
		Yellow2 = 0xEEEE00,
		Yellow3 = 0xCDCD00,
		Yellow4 = 0x8B8B00,
		Cyan1 = 0x00FFFF,
		Cyan2 = 0x00EEEE,
		Cyan3 = 0x00CDCD,
		Cyan4 = 0x008B8B,
		Magenta1 = 0xFF00FF,
		Magenta2 = 0xEE00EE,
		Magenta3 = 0xCD00CD,
		Magenta4 = 0x8B008B,
		VioletRed = 0xD02090,
		DeepPink1 = 0xFF1493,
		DeepPink2 = 0xEE1289,
		DeepPink3 = 0xCD1076,
		DeepPink4 = 0x8B0A50,
		Gold1 = 0xFFD700,
		Gold2 = 0xEEC900,
		Gold3 = 0xCDAD00,
		Gold4 = 0x8B7500,
	} HTMLColorCode;

	CRGB() : r(0), g(0), b(0) {}
	CRGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}

	// 24-bit hex code (0xRRGGBB), including all the named-color enum values above.
	CRGB(uint32_t code) : r((code >> 16) & 0xFF), g((code >> 8) & 0xFF), b(code & 0xFF) {}

	// Construct from HSV via FastLED's rainbow conversion.
	CRGB(const CHSV &hsv) { stub_hsv2rgb_rainbow(hsv, *this); }

	CRGB(const CRGB &o) = default;
	CRGB &operator=(const CRGB &o) = default;

	CRGB &operator=(const CHSV &hsv) { stub_hsv2rgb_rainbow(hsv, *this); return *this; }
	CRGB &operator=(uint32_t code) {
		r = (code >> 16) & 0xFF; g = (code >> 8) & 0xFF; b = code & 0xFF; return *this;
	}

	// Index access (0=r,1=g,2=b), as FastLED provides.
	uint8_t &operator[](uint8_t i) { return raw[i]; }
	const uint8_t &operator[](uint8_t i) const { return raw[i]; }

	CRGB &setRGB(uint8_t nr, uint8_t ng, uint8_t nb) { r = nr; g = ng; b = nb; return *this; }
	CRGB &setHSV(uint8_t h, uint8_t s, uint8_t v) { stub_hsv2rgb_rainbow(CHSV(h, s, v), *this); return *this; }
	CRGB &setHue(uint8_t h) { stub_hsv2rgb_rainbow(CHSV(h, 255, 255), *this); return *this; }
	CRGB &setColorCode(uint32_t code) { return (*this = code); }

	// Saturating add / subtract (per-channel clamp), matching FastLED's operator+=/-=.
	CRGB &operator+=(const CRGB &o) {
		r = qadd(r, o.r); g = qadd(g, o.g); b = qadd(b, o.b); return *this;
	}
	CRGB &operator-=(const CRGB &o) {
		r = qsub(r, o.r); g = qsub(g, o.g); b = qsub(b, o.b); return *this;
	}
	CRGB operator+(const CRGB &o) const {
		return CRGB(qadd(r, o.r), qadd(g, o.g), qadd(b, o.b));
	}
	CRGB operator-(const CRGB &o) const {
		return CRGB(qsub(r, o.r), qsub(g, o.g), qsub(b, o.b));
	}

	// Scale all channels down by (scale/256): FastLED's nscale8 and its operator%=/%.
	CRGB &nscale8(uint8_t scale) {
		r = stub_scale8(r, scale); g = stub_scale8(g, scale); b = stub_scale8(b, scale); return *this;
	}
	CRGB &nscale8_video(uint8_t scale) {
		r = stub_scale8_video(r, scale); g = stub_scale8_video(g, scale); b = stub_scale8_video(b, scale); return *this;
	}
	CRGB &operator%=(uint8_t scale) { return nscale8_video(scale); }
	CRGB operator%(uint8_t scale) const { CRGB c(*this); c.nscale8_video(scale); return c; }

	CRGB &fadeToBlackBy(uint8_t amount) { return nscale8(255 - amount); }
	CRGB &fadeLightBy(uint8_t amount) { return nscale8_video(255 - amount); }

	// Perceived brightness (Rec.601-ish luma), matching FastLED's getLuma().
	uint8_t getLuma() const {
		return (uint8_t)((54 * (uint16_t)r + 183 * (uint16_t)g + 18 * (uint16_t)b) >> 8);
	}
	// Average of the three channels, matching FastLED's getAverageLight().
	uint8_t getAverageLight() const {
		return (uint8_t)(((uint16_t)r + (uint16_t)g + (uint16_t)b) / 3);
	}

	// Linear interpolation toward `other` by frac/256, matching FastLED's lerp8.
	CRGB lerp8(const CRGB &other, uint8_t frac) const {
		return CRGB(lerp8by8(r, other.r, frac), lerp8by8(g, other.g, frac), lerp8by8(b, other.b, frac));
	}

	bool operator==(const CRGB &o) const { return r == o.r && g == o.g && b == o.b; }
	bool operator!=(const CRGB &o) const { return !(*this == o); }
	explicit operator bool() const { return r || g || b; }

private:
	static uint8_t qadd(uint8_t a, uint8_t bb) { int s = (int)a + bb; return (uint8_t)(s > 255 ? 255 : s); }
	static uint8_t qsub(uint8_t a, uint8_t bb) { int s = (int)a - bb; return (uint8_t)(s < 0 ? 0 : s); }
	static uint8_t lerp8by8(uint8_t a, uint8_t bb, uint8_t frac) {
		if (bb > a) return a + stub_scale8(bb - a, frac);
		return a - stub_scale8(a - bb, frac);
	}
};

// FastLED's hsv2rgb_rainbow, copied from hsv2rgb.cpp (non-AVR, SCALE8_FIXED path). Produces the same RGB a hardware
// CHSV(...) would. K-constants and the scale8 helpers above mirror lib8tion.
inline void stub_hsv2rgb_rainbow(const CHSV &hsv, CRGB &rgb) {
	const uint8_t K255 = 255, K171 = 171, K85 = 85;
	uint8_t hue = hsv.hue, sat = hsv.sat, val = hsv.val;
	uint8_t offset = hue & 0x1F;      // 0..31
	uint8_t offset8 = offset << 3;
	uint8_t third = stub_scale8(offset8, (256 / 3)); // max = 85
	uint8_t r, g, b;

	if (!(hue & 0x80)) {
		if (!(hue & 0x40)) {
			if (!(hue & 0x20)) {          // R -> O
				r = K255 - third; g = third; b = 0;
			} else {                      // O -> Y
				r = K171; g = K85 + third; b = 0;
			}
		} else {
			if (!(hue & 0x20)) {          // Y -> G
				uint8_t twothirds = stub_scale8(offset8, ((256 * 2) / 3));
				r = K171 - twothirds; g = (uint8_t)(170 + third); b = 0;
			} else {                      // G -> A
				r = 0; g = K255 - third; b = third;
			}
		}
	} else {
		if (!(hue & 0x40)) {
			if (!(hue & 0x20)) {          // A -> B
				uint8_t twothirds = stub_scale8(offset8, ((256 * 2) / 3));
				r = 0; g = K171 - twothirds; b = K85 + twothirds;
			} else {                      // B -> P
				r = third; g = 0; b = K255 - third;
			}
		} else {
			if (!(hue & 0x20)) {          // P -> K
				r = K85 + third; g = 0; b = K171 - third;
			} else {                      // K -> R
				r = 170 + third; g = 0; b = K85 - third;
			}
		}
	}

	// Desaturate. This follows FastLED's FASTLED_SCALE8_FIXED==1 path (the modern default): scale each channel by
	// satscale (plain scale8, no +1) then add the brightness floor. Porting the legacy #else path instead causes
	// off-by-one drift and channel overflow, so match FIXED exactly.
	if (sat != 255) {
		if (sat == 0) {
			r = 255; g = 255; b = 255;
		} else {
			uint8_t desat = 255 - sat;
			desat = stub_scale8_video(desat, desat);
			uint8_t satscale = 255 - desat;
			r = stub_scale8(r, satscale);
			g = stub_scale8(g, satscale);
			b = stub_scale8(b, satscale);
			uint8_t brightness_floor = desat;
			r += brightness_floor; g += brightness_floor; b += brightness_floor;
		}
	}

	// Scale by value (again the FIXED path: plain scale8, no +1).
	if (val != 255) {
		val = stub_scale8_video(val, val);
		if (val == 0) {
			r = 0; g = 0; b = 0;
		} else {
			r = stub_scale8(r, val);
			g = stub_scale8(g, val);
			b = stub_scale8(b, val);
		}
	}

	rgb.r = r; rgb.g = g; rgb.b = b;
}
