// ============================================================================
// geometry.h  —  pure 2D polyline geometry for tulip generation
//
// Works on a generic 2D plane. The caller maps world coordinates onto it:
//   Vec2.x = world X (East+),  Vec2.y = world Z (North+).
// With that mapping, headingDeg() returns a compass bearing matching the
// game's m_fYaw (0 = North/+Z, 90 = East/+X), so no calibration is needed.
//
// No game dependencies — compiled into both the plugin and the offline tests.
// ============================================================================
#pragma once

#include <vector>

namespace geo {

struct Vec2 { float x = 0.0f; float y = 0.0f; };

inline Vec2  operator+(Vec2 a, Vec2 b) { return { a.x + b.x, a.y + b.y }; }
inline Vec2  operator-(Vec2 a, Vec2 b) { return { a.x - b.x, a.y - b.y }; }
inline Vec2  operator*(Vec2 a, float s) { return { a.x * s, a.y * s }; }

float length(Vec2 v);
float dist(Vec2 a, Vec2 b);

// Compass bearing of a direction vector, degrees in [0,360): 0 = +y (North),
// 90 = +x (East), increasing clockwise.
float headingDeg(Vec2 dir);

// Wrap a degree value to (-180, 180].
float wrap180(float deg);
// Wrap a degree value to [0, 360).
float wrap360(float deg);

// Total arc length of a polyline.
float pathLength(const std::vector<Vec2>& pts);

// Resample a polyline to uniform `spacing` (meters) along its arc length.
// The first point is preserved; the last point is appended. Returns >=2 points
// for any non-degenerate input.
std::vector<Vec2> resampleByArcLength(const std::vector<Vec2>& pts, float spacing);

// Centered moving-average smoothing with the given half-window (in samples).
// Endpoints use a shrinking window. halfWindow <= 0 returns the input.
std::vector<Vec2> smooth(const std::vector<Vec2>& pts, int halfWindow);

// Heading (compass degrees) at vertex i of a uniform polyline, via central
// difference. Clamps at the ends.
float headingAtIndex(const std::vector<Vec2>& pts, int i);

// Result of projecting a query point onto a polyline: the segment index, the
// parameter t in [0,1] along that segment, and the perpendicular distance.
struct Projection { int seg = -1; float t = 0.0f; float dist = 1e30f; };

// Nearest point on the polyline to q. If hint>=0 and window>0, search
// [hint-window, hint+window] segments first; if that best distance exceeds
// `reacquire`, also do a global search and take the closer (handles loops and
// off-route re-acquisition). Returns seg=-1 if the polyline has <2 points.
Projection projectToPolyline(const std::vector<Vec2>& pts, Vec2 q,
                             int hint = -1, int window = 0, float reacquire = 1e30f);

} // namespace geo
