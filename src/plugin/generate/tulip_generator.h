// ============================================================================
// tulip_generator.h  —  recorded ride -> roadbook (the novel core)
//
// Pipeline: raw (x,z) trace -> resample by arc length -> smooth -> detect
// waypoints (turns + forced straights) -> build boxes (distances, CAP, tulip
// geometry). Pure: no game/Win32 deps, compiled into the offline test harness.
// ============================================================================
#pragma once

#include <string>
#include <vector>

#include "geometry.h"
#include "../model/roadbook.h"

namespace gen {

// One recorded sample (subset of the ride CSV needed for generation).
struct RideSample
{
	float x = 0.0f;   // world X (East+)
	float z = 0.0f;   // world Z (North+)
	float y = 0.0f;   // elevation (up+) — for crest/dip detection
	float yaw = 0.0f; // compass heading (deg) — cross-check only
	float speed = 0.0f;
	float dist = 0.0f; // recorder odometer (m) — cross-check only
};

struct Params
{
	float resampleM          = 1.0f;   // arc-length resample spacing
	int   smoothHalfWindow   = 7;      // ~±7 m smoothing at 1 m spacing
	float turnThresholdDeg   = 35.0f;  // cumulative heading change to call a turn
	float flatWindowM        = 15.0f;  // window over which to judge "heading settled"
	float flatDeg            = 8.0f;   // heading change below this over the window = straight
	float maxSpacingM        = 300.0f; // force a keep-ahead box on long straights
	float minSpacingM        = 18.0f;  // merge guard between boxes
	float tulipWindowM       = 40.0f;  // ± window around the apex for the tulip
	float easyTurnDeg        = 80.0f;
	float hairpinDeg         = 120.0f;
};

model::Roadbook generate(const std::vector<RideSample>& ride,
                         const Params& p = Params(),
                         const std::string& trackId = "",
                         const std::string& trackName = "");

// Multi-ride union: annotate an already-generated roadbook with the un-taken
// branch spokes seen in other rides of the same track.
void addBranches(model::Roadbook& rb, const std::vector<std::vector<RideSample>>& others, const Params& p = Params());

} // namespace gen
