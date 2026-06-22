// ============================================================================
// roadbook.h  —  the roadbook data model (shared by generator and renderer)
//
// One schema serves both auto-generated and hand-authored roadbooks. Tulip
// geometry is a normalized polyline in [0,1]^2, oriented travel-up (direction
// of travel points toward +y / the top of the box). JSON (de)serialization is
// added in a later step; these structs are the in-memory form.
// ============================================================================
#pragma once

#include <string>
#include <vector>

namespace model {

struct TulipPoint { float x = 0.0f; float y = 0.0f; }; // normalized [0,1], travel-up

// A point on the recorded route, in world ground-plane coords, with its
// cumulative arc-length. Used for map-matching (trip alignment, "where am I",
// off-route distance) and the minimap.
struct RoutePoint { float x = 0.0f; float z = 0.0f; float distM = 0.0f; float elev = 0.0f; };

enum class BoxType { Straight, Turn, Hairpin, Start, Finish };
enum class TurnDir { None, Left, Right };

// Standard rally pictogram codes (drawn procedurally). Keep in sync with the
// renderer's drawPictogram() and the editor palette.
namespace picto {
// Order MUST match the sprite registration order in DrawInit (sprite index =
// position + 1). Files: plugins/Roadbook_data/icons/rb_<code>.tga.
inline const char* kAll[] = {
	// directions (0-10)
	"keepL", "keepR", "keepS", "left", "right", "onL", "onR",
	"hairpin", "roundaboutL", "roundaboutR", "keepMain",
	// danger / hazards (11-25)
	"caution", "danger2", "danger3", "cutL", "cutR", "crestL", "crestR",
	"downhill", "inclineL", "inclineR", "slowdown", "deadend", "lessVisible", "noentry", "stop",
	// terrain / surface (26-46)
	"bump", "jump", "dip", "ditch", "compression", "hole", "bumpy", "ruts", "rough",
	"dune", "dunes", "sandpit", "camelgrass", "rocky", "rocks", "chott", "mountain",
	"hill", "cliff", "collapse", "narrow",
	// water (47-54)
	"water", "river", "lake", "pond", "dryriver", "wadi", "bigwadi", "canal",
	// man-made (55-71)
	"fence", "gate", "bridge", "pole", "antenna", "pipeline", "railroad", "wall",
	"barbwire", "cattleguard", "gatebar", "hvline", "pump", "mine", "cairn", "wellpost", "roadworks",
	// scenery (72-82)
	"tree", "bush", "village", "house", "church", "cemetery", "monument", "ruins", "castle", "camp", "animals",
	// service / control (83-93)
	"fuel", "fuelzone", "checkpoint", "mediapt", "photo", "spectators", "police", "medical", "helicopter", "info", "north",
};
constexpr int kCount = 94;

// Category boundaries for the editor's grouped sign picker (start index in kAll).
struct Cat { const char* name; int start; };
inline const Cat kCats[] = {
	{ "Direction", 0 }, { "Danger", 11 }, { "Terrain", 26 }, { "Water", 47 },
	{ "Man-made", 55 }, { "Scenery", 72 }, { "Service", 83 },
};
constexpr int kCatCount = 7;
}

struct Box
{
	int    index        = 0;
	double distTotalKm  = 0.0;   // cumulative distance from start, km
	int    distPartialM = 0;     // distance since previous box, meters
	int    capDeg       = 0;     // compass heading at the waypoint, 0..359

	BoxType type        = BoxType::Turn;
	TurnDir turnDir     = TurnDir::None;
	int     dangerLevel = 0; // 0 = none, 1..3 = ! / !! / !!!

	std::vector<std::string> pictograms; // picto:: codes
	std::string note;                    // optional, CP1252, <=99 chars

	// Tulip schematic, normalized [0,1]^2, travel-up.
	std::vector<TulipPoint> tulip;
	TulipPoint entry        { 0.5f, 0.05f }; // entry dot (bottom = where you come from)
	TulipPoint exitAt       { 0.5f, 0.95f }; // arrow tip
	float      exitHeadingDeg = 0.0f;        // arrow direction in box space (deg, 0=up)
	float      crossDeg = -999.0f;           // junction crossing-road angle in tulip space (deg); -999 = none
	std::vector<float> branchDeg;            // un-taken junction branches (deg in tulip space, 0 = up); from multi-ride union
};

struct Roadbook
{
	int    schemaVersion   = 1;
	std::string trackId;
	std::string trackName;
	std::string generatedBy = "plugin";
	std::string sourceRide;
	double totalDistanceKm = 0.0;

	std::vector<Box>        boxes;
	std::vector<RoutePoint> route; // downsampled recorded path for map-matching
};

// Helpers for string<->enum (used by JSON and diagnostics).
const char* toString(BoxType t);
const char* toString(TurnDir d);

// True if `code` is present in box.pictograms.
bool hasPictogram(const Box& b, const char* code);
void togglePictogram(Box& b, const char* code);

} // namespace model
