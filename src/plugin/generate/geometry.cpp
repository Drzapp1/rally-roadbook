#include "geometry.h"

#include <algorithm>
#include <cmath>

namespace geo {

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kRad2Deg = 180.0f / kPi;
}

float length(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
float dist(Vec2 a, Vec2 b) { return length(b - a); }

float headingDeg(Vec2 dir)
{
	// atan2(East, North) = atan2(x, y); 0 = +y (N), 90 = +x (E), clockwise.
	float deg = std::atan2(dir.x, dir.y) * kRad2Deg;
	return wrap360(deg);
}

float wrap180(float deg)
{
	while (deg <= -180.0f) deg += 360.0f;
	while (deg >   180.0f) deg -= 360.0f;
	return deg;
}

float wrap360(float deg)
{
	deg = std::fmod(deg, 360.0f);
	if (deg < 0.0f) deg += 360.0f;
	return deg;
}

float pathLength(const std::vector<Vec2>& pts)
{
	float total = 0.0f;
	for (size_t i = 1; i < pts.size(); ++i) total += dist(pts[i - 1], pts[i]);
	return total;
}

std::vector<Vec2> resampleByArcLength(const std::vector<Vec2>& pts, float spacing)
{
	std::vector<Vec2> out;
	if (pts.size() < 2 || spacing <= 0.0f) return pts;

	out.push_back(pts.front());
	float carry = 0.0f; // distance accumulated past the last emitted sample

	for (size_t i = 1; i < pts.size(); ++i)
	{
		Vec2 a = pts[i - 1];
		Vec2 b = pts[i];
		float seg = dist(a, b);
		if (seg < 1e-6f) continue;

		Vec2 dir = (b - a) * (1.0f / seg);
		float posOnSeg = -carry; // where the next sample falls, measured from a
		while (posOnSeg + spacing <= seg)
		{
			posOnSeg += spacing;
			out.push_back(a + dir * posOnSeg);
		}
		carry = seg - posOnSeg; // leftover toward the next segment
	}

	// Ensure the final point is represented.
	if (dist(out.back(), pts.back()) > spacing * 0.5f) out.push_back(pts.back());
	return out;
}

std::vector<Vec2> smooth(const std::vector<Vec2>& pts, int halfWindow)
{
	if (halfWindow <= 0 || pts.size() < 3) return pts;

	const int n = static_cast<int>(pts.size());
	std::vector<Vec2> out(pts.size());
	for (int i = 0; i < n; ++i)
	{
		int lo = std::max(0, i - halfWindow);
		int hi = std::min(n - 1, i + halfWindow);
		Vec2 sum{};
		for (int j = lo; j <= hi; ++j) sum = sum + pts[j];
		float inv = 1.0f / static_cast<float>(hi - lo + 1);
		out[i] = sum * inv;
	}
	return out;
}

float headingAtIndex(const std::vector<Vec2>& pts, int i)
{
	const int n = static_cast<int>(pts.size());
	if (n < 2) return 0.0f;
	int a = std::max(0, i - 1);
	int b = std::min(n - 1, i + 1);
	if (a == b) { a = std::max(0, i - 1); b = std::min(n - 1, i + 1); }
	return headingDeg(pts[b] - pts[a]);
}

namespace {
Projection projectSegment(Vec2 a, Vec2 b, Vec2 q, int seg)
{
	Vec2 ab = b - a;
	float len2 = ab.x * ab.x + ab.y * ab.y;
	float t = (len2 > 1e-9f) ? ((q.x - a.x) * ab.x + (q.y - a.y) * ab.y) / len2 : 0.0f;
	t = std::max(0.0f, std::min(1.0f, t));
	Vec2 proj = a + ab * t;
	return { seg, t, dist(q, proj) };
}
} // namespace

Projection projectToPolyline(const std::vector<Vec2>& pts, Vec2 q, int hint, int window, float reacquire)
{
	const int n = static_cast<int>(pts.size());
	if (n < 2) return {};

	auto search = [&](int lo, int hi) -> Projection {
		Projection best;
		lo = std::max(0, lo);
		hi = std::min(n - 2, hi);
		for (int i = lo; i <= hi; ++i)
		{
			Projection p = projectSegment(pts[i], pts[i + 1], q, i);
			if (p.dist < best.dist) best = p;
		}
		return best;
	};

	Projection best;
	if (hint >= 0 && window > 0) best = search(hint - window, hint + window);
	if (best.seg < 0 || best.dist > reacquire)
	{
		Projection g = search(0, n - 2);
		if (best.seg < 0 || g.dist < best.dist) best = g;
	}
	return best;
}

} // namespace geo
