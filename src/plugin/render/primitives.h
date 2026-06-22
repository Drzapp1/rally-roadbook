// ============================================================================
// primitives.h  —  color packing + shared render constants
//
// The PiBoSo draw API takes colors as a packed ABGR unsigned long. Encoding it
// in exactly one place avoids the classic ARGB/ABGR channel-swap bug.
// ============================================================================
#pragma once

namespace render {

// Pack r,g,b,a (each 0..255) into the API's ABGR layout.
inline unsigned long abgr(int r, int g, int b, int a = 255)
{
	return (static_cast<unsigned long>(a & 0xFF) << 24) |
	       (static_cast<unsigned long>(b & 0xFF) << 16) |
	       (static_cast<unsigned long>(g & 0xFF) << 8)  |
	       (static_cast<unsigned long>(r & 0xFF));
}

// A few named colors for the overlay.
namespace col {
inline const unsigned long white   = abgr(255, 255, 255);
inline const unsigned long black   = abgr(0,   0,   0);
inline const unsigned long panel   = abgr(0,   0,   0,   180); // translucent backdrop
inline const unsigned long accent  = abgr(255, 190, 40);       // amber
inline const unsigned long good    = abgr(90,  220, 120);
inline const unsigned long warn    = abgr(240, 90,  60);
inline const unsigned long dim     = abgr(150, 150, 150);
} // namespace col

} // namespace render
