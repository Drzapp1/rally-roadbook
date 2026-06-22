// ============================================================================
// roadbook_renderer.h  —  draws the roadbook strip + navigation aids
//
// The roadbook strip + "next" preview are drawn in an authentic paper-case
// style; the compass / off-route / minimap are HUD aids. Each module is
// individually toggled, scaled, and offset via its ModuleCfg; the renderer is
// stateless and driven by a NavState the plugin fills each frame.
// ============================================================================
#pragma once

#include "../model/roadbook.h"
#include "../model/settings.h"
#include "../generate/geometry.h"
#include "draw_cache.h"

namespace render {

struct NavState
{
	double tripM         = 0.0;
	float  offRouteM     = 0.0f;
	bool   matched       = false;
	float  curHeading    = 0.0f;
	float  targetHeading = 0.0f;
	float  speedKmh      = 0.0f;
	float  bearingToRoute = 0.0f;
	geo::Vec2 pos;

	bool  challenge    = false;
	bool  recording    = false; // ride recorder active (lights the dash REC indicator)
	bool  deviceFrame  = true;  // present the roadbook as a rally-computer device
	int   deviceModel  = 0;     // 0 Carbon, 1 Tablet, 2 Classic, 3 Stealth
	int   lcdColor     = 0;     // 0 = model default, else override
	bool  maskBox      = false; // training: only the current box visible
	bool  noCap        = false; // training: hide compass headings
	bool  distanceOnly = false; // training: hide tulips
	int   panelOpacity = 200;
	int   manualBox    = -1;
	float offRouteTolM = 30.0f; // distance off the line before the off-route alert fires
	int   styleIndex   = 0;     // visual theme
	bool  frenchNotation = false; // French roadbook abbreviations (G/D/TD)
	float advanceLookM = 5.0f;  // how far ahead a box becomes "current"

	// layout (filled from settings by the plugin)
	float gScale = 1.0f, gOffX = 0.0f, gOffY = 0.0f;
	model::ModuleCfg mStrip, mCompass, mNext, mOffroute, mMinimap, mDash, mDriver, mProfile;
};

class RoadbookRenderer
{
public:
	void draw(DrawCache& dc, int font, const model::Roadbook& rb, const NavState& nav);
	void drawPage(DrawCache& dc, int font, const model::Roadbook& rb, int page, int styleIndex);
};

// Restart the rally-computer power-on animation (call on session / roadbook load).
void triggerDeviceBoot();

// Turn-tulip image catalog. The renderer blits rb_tulip_<code>.tga (imported
// art, classified per turn) instead of drawing the path; DrawInit registers the
// sprites in THIS order, right after the device models.
constexpr int kTulipCount = 14;
extern const char* const kTulipCodes[kTulipCount];

} // namespace render
