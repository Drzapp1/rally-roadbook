// ============================================================================
// settings.h  —  user settings, persisted to savePath\Roadbook\settings.json
// ============================================================================
#pragma once

#include <string>

namespace model {

// Per-module display config: on/off, size, and position offset.
struct ModuleCfg
{
	bool  on      = true;
	float scale   = 1.0f;   // 0.5..2.0
	float x       = 0.0f;   // normalized offset
	float y       = 0.0f;
	int   opacity = -1;     // -1 = use global opacity, else 0..255
};

struct Settings
{
	float scale   = 1.0f;   // global HUD scale, 0.6..1.6
	int   opacity = 200;    // panel alpha, 0..255
	float offX    = 0.0f;   // global overlay offset
	float offY    = 0.0f;

	bool  challenge = false;
	bool  audio = true;        // warning tones (chime/danger/off-route)
	bool  voice = false;       // spoken co-driver callouts (replaces the chime)
	bool  paceSentences = true; // full spoken pace-note sentences (vs single words)
	bool  deviceFrame = true;  // present the roadbook as a rally-computer device
	int   deviceModel = 0;     // rally-computer model: 0 Carbon, 1 Tablet, 2 Classic, 3 Stealth
	int   lcdColor = 0;        // 0 = model default, 1..5 override (amber/green/red/blue/white)
	int   layoutPreset = 0;    // 0 = chase (right), 1 = bottom-centre, 2 = onboard/cockpit
	bool  maskBox = false;      // training: only the current box is visible
	bool  noCap = false;        // training: hide compass headings (navigate by tulip)
	bool  distanceOnly = false; // training: hide tulips (train the trip meter)
	int   detailDeg = 35;   // generator turn threshold (15..70; lower = more boxes)
	int   offRouteToleranceM = 30; // how far off the recorded line before "off route" (m)
	int   styleIndex = 0;          // visual theme (0..3)
	int   abbrevStyle = 0;         // turn abbreviations: 0 = international (R/L/kpS), 1 = French (D/G/TD)
	int   advanceLookM = 5;        // how far ahead a box becomes "current" (m)

	// modules
	ModuleCfg mDriver;  // co-driver pace-note callout (top-centre)
	ModuleCfg mDash;    // rally trip computer (top-left)
	ModuleCfg mStatus;  // diagnostic line (stacks below the dash)
	ModuleCfg mStrip;
	ModuleCfg mCompass;
	ModuleCfg mNext;
	ModuleCfg mOffroute;
	ModuleCfg mMinimap{ false, 1.0f, 0.0f, 0.0f }; // off by default
	ModuleCfg mProfile{ false, 1.0f, 0.0f, 0.0f }; // elevation profile, off by default
};

bool loadSettings(Settings& s, const std::string& path);
bool saveSettings(const Settings& s, const std::string& path);

} // namespace model
