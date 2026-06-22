#include "tulip_generator.h"

#include <algorithm>
#include <cmath>

namespace gen {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kDeg2Rad = kPi / 180.0f;

// Ramer-Douglas-Peucker simplification of a normalized tulip polyline: keeps
// the corners, drops collinear GPS jitter -> a clean schematic diagram.
void rdp(const std::vector<model::TulipPoint>& p, int a, int b, float eps, std::vector<int>& keep)
{
	if (b <= a + 1) return;
	float x1 = p[a].x, y1 = p[a].y, dx = p[b].x - x1, dy = p[b].y - y1;
	float len = std::sqrt(dx * dx + dy * dy);
	float maxD = -1.0f; int idx = -1;
	for (int i = a + 1; i < b; ++i)
	{
		float d = (len < 1e-6f) ? std::sqrt((p[i].x - x1) * (p[i].x - x1) + (p[i].y - y1) * (p[i].y - y1))
		                        : std::fabs((p[i].x - x1) * dy - (p[i].y - y1) * dx) / len;
		if (d > maxD) { maxD = d; idx = i; }
	}
	if (maxD > eps) { rdp(p, a, idx, eps, keep); keep.push_back(idx); rdp(p, idx, b, eps, keep); }
}
std::vector<model::TulipPoint> simplifyTulip(const std::vector<model::TulipPoint>& p, float eps)
{
	if (p.size() < 3) return p;
	std::vector<int> keep = { 0 };
	rdp(p, 0, static_cast<int>(p.size()) - 1, eps, keep);
	keep.push_back(static_cast<int>(p.size()) - 1);
	std::sort(keep.begin(), keep.end());
	std::vector<model::TulipPoint> out;
	for (int i : keep) out.push_back(p[i]);
	return out;
}

// Build the normalized, travel-up tulip polyline for a waypoint at index `ai`.
void buildTulip(const std::vector<geo::Vec2>& pts, int ai, float spacing,
                float windowM, model::Box& box)
{
	const int n = static_cast<int>(pts.size());
	const int W = std::max(1, static_cast<int>(windowM / spacing));
	const int a = std::max(0, ai - W);
	const int b = std::min(n - 1, ai + W);
	if (b <= a) return;

	// Rotate so the entry travel direction points up (+y). entryHeading is the
	// compass heading at the window start; rotating by +entryHeading maps that
	// direction onto (0,1).
	const float theta = geo::headingAtIndex(pts, a) * kDeg2Rad;
	const float ct = std::cos(theta), st = std::sin(theta);
	const geo::Vec2 apex = pts[ai];

	std::vector<geo::Vec2> rot;
	rot.reserve(b - a + 1);
	for (int j = a; j <= b; ++j)
	{
		geo::Vec2 p = pts[j] - apex;
		rot.push_back({ p.x * ct - p.y * st, p.x * st + p.y * ct });
	}

	// Uniform-scale normalize into [0,1]^2 with a margin (preserve aspect).
	float minX = rot[0].x, maxX = rot[0].x, minY = rot[0].y, maxY = rot[0].y;
	for (const auto& r : rot)
	{
		minX = std::min(minX, r.x); maxX = std::max(maxX, r.x);
		minY = std::min(minY, r.y); maxY = std::max(maxY, r.y);
	}
	const float margin = 0.12f;
	const float span = std::max({ maxX - minX, maxY - minY, 1e-3f });
	const float scale = (1.0f - 2.0f * margin) / span;
	const float cx = 0.5f * (minX + maxX), cy = 0.5f * (minY + maxY);

	box.tulip.clear();
	box.tulip.reserve(rot.size());
	for (const auto& r : rot)
		box.tulip.push_back({ 0.5f + (r.x - cx) * scale, 0.5f + (r.y - cy) * scale });

	box.tulip = simplifyTulip(box.tulip, 0.040f); // clean GPS jitter into a schematic line

	box.entry  = box.tulip.front();
	box.exitAt = box.tulip.back();
	if (box.tulip.size() >= 2)
	{
		const auto& q1 = box.tulip[box.tulip.size() - 2];
		const auto& q2 = box.tulip.back();
		box.exitHeadingDeg = std::atan2(q2.x - q1.x, q2.y - q1.y) / kDeg2Rad; // 0 = up
	}

	// junction indicator: does the route pass near this apex elsewhere, on a
	// clearly different heading? (a crossing the rider went through) -> stub.
	box.crossDeg = -999.0f;
	const float thetaDeg = theta / kDeg2Rad;
	for (int j = 0; j < n; ++j)
	{
		if (std::abs(j - ai) * spacing < 60.0f) continue; // ignore the immediate path
		float ddx = pts[j].x - apex.x, ddz = pts[j].y - apex.y;
		if (ddx * ddx + ddz * ddz < 36.0f) // within ~6 m
		{
			float rel = geo::wrap180(geo::headingAtIndex(pts, j) - thetaDeg);
			if (std::fabs(rel) > 55.0f && std::fabs(rel) < 125.0f) { box.crossDeg = rel; break; }
		}
	}
}

model::Box makeBox(const std::vector<geo::Vec2>& capPts, const std::vector<geo::Vec2>& tulPts, int idx, float spacing,
                   float prevS, float turnDeg, bool forcedStraight, const Params& p)
{
	model::Box box;
	const float s = idx * spacing;
	box.distTotalKm  = s / 1000.0;
	box.distPartialM = static_cast<int>(std::lround(s - prevS));
	box.capDeg       = static_cast<int>(std::lround(geo::headingAtIndex(capPts, idx)));
	if (box.capDeg >= 360) box.capDeg -= 360;

	const float aT = std::fabs(turnDeg);
	if (forcedStraight || aT < p.turnThresholdDeg)
	{
		box.type        = model::BoxType::Straight;
		box.turnDir     = model::TurnDir::None;
		box.dangerLevel = 0;
	}
	else
	{
		box.turnDir = (turnDeg > 0.0f) ? model::TurnDir::Right : model::TurnDir::Left;
		if      (aT >= 160.0f)        { box.type = model::BoxType::Hairpin; box.dangerLevel = 3; }
		else if (aT >= p.hairpinDeg)  { box.type = model::BoxType::Hairpin; box.dangerLevel = 2; }
		else if (aT >= p.easyTurnDeg) { box.type = model::BoxType::Turn;    box.dangerLevel = 1; }
		else                          { box.type = model::BoxType::Turn;    box.dangerLevel = 0; }
	}

	buildTulip(tulPts, idx, spacing, p.tulipWindowM, box);
	return box;
}

} // namespace

model::Roadbook generate(const std::vector<RideSample>& rideIn, const Params& p,
                         const std::string& trackId, const std::string& trackName)
{
	model::Roadbook rb;
	rb.trackId = trackId;
	rb.trackName = trackName;
	rb.generatedBy = "plugin";
	// drop any non-finite telemetry frames (corrupt input) before anything else
	std::vector<RideSample> ride; ride.reserve(rideIn.size());
	for (const auto& s : rideIn) if (std::isfinite(s.x) && std::isfinite(s.z) && std::isfinite(s.y)) ride.push_back(s);
	if (ride.size() < 2) return rb;

	std::vector<geo::Vec2> raw;
	raw.reserve(ride.size());
	for (const auto& s : ride) raw.push_back({ s.x, s.z });

	std::vector<geo::Vec2> res = geo::resampleByArcLength(raw, p.resampleM);
	std::vector<geo::Vec2> pts = geo::smooth(res, p.smoothHalfWindow);             // detection (heavier smooth)
	std::vector<geo::Vec2> tul = geo::smooth(res, std::max(1, p.smoothHalfWindow / 2)); // tulip shape (cleaner schematic)
	const int n = static_cast<int>(pts.size());
	if (n < 2) return rb;

	std::vector<float> hd(n);
	for (int i = 0; i < n; ++i) hd[i] = geo::headingAtIndex(pts, i);

	// --- waypoint detection -------------------------------------------------
	struct WP { int idx; float turn; bool straight; };
	std::vector<WP> wps;

	int   lastIdx = 0;
	float accum = 0.0f, peak = 0.0f;
	int   peakIdx = -1;
	bool  inTurn = false;
	const int flatWin = std::max(1, static_cast<int>(p.flatWindowM / p.resampleM));

	for (int i = 1; i < n; ++i)
	{
		float seg = geo::wrap180(hd[i] - hd[i - 1]);
		accum += seg;
		if (std::fabs(seg) > peak) { peak = std::fabs(seg); peakIdx = i; }

		// "Flat" = heading barely changed over the last flatWin meters. This ends
		// a turn regardless of how gentle it was (rate-independent), unlike a
		// per-meter curvature threshold.
		bool flat = (i >= flatWin) &&
		            (std::fabs(geo::wrap180(hd[i] - hd[i - flatWin])) < p.flatDeg);

		if (!inTurn)
		{
			if (std::fabs(accum) >= p.turnThresholdDeg) inTurn = true;
			else if (flat) { accum = 0.0f; peak = 0.0f; peakIdx = -1; } // keep straight baseline clean
		}
		else if (flat) // the turn has ended
		{
			if (peakIdx > 0 && (peakIdx - lastIdx) * p.resampleM >= p.minSpacingM)
			{
				wps.push_back({ peakIdx, accum, false });
				lastIdx = peakIdx;
			}
			accum = 0.0f; peak = 0.0f; peakIdx = -1; inTurn = false;
		}

		if (!inTurn && (i - lastIdx) * p.resampleM >= p.maxSpacingM)
		{
			wps.push_back({ i, 0.0f, true });
			lastIdx = i; accum = 0.0f; peak = 0.0f; peakIdx = -1;
		}
	}
	if (inTurn && peakIdx > 0 && (peakIdx - lastIdx) * p.resampleM >= p.minSpacingM)
		wps.push_back({ peakIdx, accum, false });

	// add a waypoint wherever the route crosses near itself on a different
	// heading (a junction the rider passed through) so the tulip shows a stub
	{
		const int gap = std::max(10, static_cast<int>(60.0f / p.resampleM));
		int lastCross = -100000;
		for (int i = 0; i < n; ++i)
		{
			for (int j = 0; j < i - gap; ++j)
			{
				float dx = pts[i].x - pts[j].x, dz = pts[i].y - pts[j].y;
				if (dx * dx + dz * dz < 25.0f) // within ~5 m
				{
					float hdiff = std::fabs(geo::wrap180(hd[i] - hd[j])); // a real crossing is ~perpendicular
					if (hdiff > 55.0f && hdiff < 125.0f && (i - lastCross) * p.resampleM > p.minSpacingM)
					{ wps.push_back({ i, 0.0f, true }); lastCross = i; }
					break;
				}
			}
		}
		std::sort(wps.begin(), wps.end(), [](const WP& a, const WP& b) { return a.idx < b.idx; });
	}

	// add a waypoint at prominent crests and dips so an elevation change gets
	// its own box (real roadbooks note a crest / dip even on a straight). The
	// elevation-tagging pass below then attaches the bump / dip / water sprite.
	{
		const size_t m = ride.size();
		if (m >= 3 && n > 4)
		{
			std::vector<float> rarc(m), rel(m); float ac = 0.0f;
			for (size_t k = 0; k < m; ++k) { if (k) { float dx = ride[k].x - ride[k - 1].x, dz = ride[k].z - ride[k - 1].z; ac += std::sqrt(dx * dx + dz * dz); } rarc[k] = ac; rel[k] = ride[k].y; }
			auto eAt = [&](float s) -> float { if (s <= rarc.front()) return rel.front(); for (size_t k = 1; k < m; ++k) if (rarc[k] >= s) { float sg = std::max(1e-3f, rarc[k] - rarc[k - 1]); return rel[k - 1] + (rel[k] - rel[k - 1]) * ((s - rarc[k - 1]) / sg); } return rel.back(); };
			std::vector<float> pe(n); for (int i = 0; i < n; ++i) pe[i] = eAt(i * p.resampleM);
			// A crest/dip becomes its own box only when it is prominent *relative to
			// the local terrain*: on smooth tarmac a 3 m rise stands out, but uniformly
			// choppy dunes undulate metres constantly, so there the bar rises with the
			// local roughness (those are covered by the "bumpy" surface tag instead).
			const float absThr = 3.5f, elevSpacing = 80.0f;
			const int w = std::max(2, static_cast<int>(22.0f / p.resampleM));
			const int inner = std::max(2, static_cast<int>(25.0f / p.resampleM));
			const int outer = std::max(inner + 2, static_cast<int>(75.0f / p.resampleM));
			int lastE = -100000;
			std::vector<std::pair<float, int>> cand; // (prominence, idx)
			for (int i = w; i < n - w; ++i)
			{
				float c = pe[i]; bool isMax = true, isMin = true;
				for (int k = i - w; k <= i + w; ++k) { if (pe[k] > c + 1e-3f) isMax = false; if (pe[k] < c - 1e-3f) isMin = false; }
				if (!isMax && !isMin) continue;
				float prom = isMax ? std::min(c - pe[i - w], c - pe[i + w]) : std::min(pe[i - w] - c, pe[i + w] - c);
				if (prom <= 0.0f || (i - lastE) * p.resampleM <= elevSpacing) continue;
				// background roughness from side-bands that EXCLUDE the +/-inner feature
				// zone, so a prominent isolated crest doesn't inflate its own bar; a dune
				// field has choppy side-bands and is suppressed.
				int a0 = std::max(0, i - outer), b0 = std::min(n - 1, i + outer);
				double sx = 0, sy = 0, sxx = 0, sxy = 0; int rc = 0;
				for (int k = a0; k <= b0; ++k) { if (std::abs(k - i) < inner) continue; sx += k; sy += pe[k]; sxx += (double)k * k; sxy += (double)k * pe[k]; ++rc; }
				if (rc < 4) continue;
				double den = rc * sxx - sx * sx, sl = den != 0 ? (rc * sxy - sx * sy) / den : 0, ic = (sy - sl * sx) / rc, ss = 0;
				for (int k = a0; k <= b0; ++k) { if (std::abs(k - i) < inner) continue; double r = pe[k] - (ic + sl * k); ss += r * r; }
				float rough = static_cast<float>(std::sqrt(ss / rc));
				// a crest/dip earns its own box when it clearly stands out from the
				// surrounding terrain (side-band roughness) and clears an absolute bar.
				if (prom > std::max(absThr, 1.8f * rough)) { cand.push_back({ prom, i }); lastE = i; }
			}
			// Keep only the most prominent ~2 features per km (floor 4) so a repetitive
			// dune field doesn't fill the book with crest boxes, while isolated crests on
			// mild terrain always survive.
			float km = (n - 1) * p.resampleM / 1000.0f;
			int keep = std::max(4, static_cast<int>(std::lround(km * 2.0)));
			std::sort(cand.begin(), cand.end(), [](const std::pair<float, int>& a, const std::pair<float, int>& b) { return a.first > b.first; });
			if (static_cast<int>(cand.size()) > keep) cand.resize(keep);
			for (const auto& cn : cand)
			{
				bool near = false; for (const auto& wp : wps) if (std::abs(wp.idx - cn.second) * p.resampleM < p.minSpacingM) { near = true; break; }
				if (!near) wps.push_back({ cn.second, 0.0f, true });
			}
			std::sort(wps.begin(), wps.end(), [](const WP& a, const WP& b) { return a.idx < b.idx; });
		}
	}

	// --- build boxes (start + waypoints + finish) ---------------------------
	float prevS = 0.0f;

	model::Box start = makeBox(pts, tul, 0, p.resampleM, 0.0f, 0.0f, true, p);
	start.type = model::BoxType::Start;
	rb.boxes.push_back(start);

	for (const auto& w : wps)
	{
		rb.boxes.push_back(makeBox(pts, tul, w.idx, p.resampleM, prevS, w.turn, w.straight, p));
		prevS = w.idx * p.resampleM;
	}

	model::Box finish = makeBox(pts, tul, n - 1, p.resampleM, prevS, 0.0f, true, p);
	finish.type = model::BoxType::Finish;
	rb.boxes.push_back(finish);

	// --- elevation signs: tag boxes that sit on a crest or a dip -------------
	{
		const size_t m = ride.size();
		if (m >= 2)
		{
			std::vector<float> arc(m), elev(m), spd(m);
			float acc = 0.0f;
			for (size_t i = 0; i < m; ++i)
			{
				if (i) { float dx = ride[i].x - ride[i - 1].x, dz = ride[i].z - ride[i - 1].z; acc += std::sqrt(dx * dx + dz * dz); }
				arc[i] = acc; elev[i] = ride[i].y; spd[i] = ride[i].speed;
			}
			auto elevAt = [&](float s) -> float {
				if (s <= arc.front()) return elev.front();
				for (size_t i = 1; i < m; ++i)
					if (arc[i] >= s) { float seg = std::max(1e-3f, arc[i] - arc[i - 1]); return elev[i - 1] + (elev[i] - elev[i - 1]) * ((s - arc[i - 1]) / seg); }
				return elev.back();
			};
			auto speedAt = [&](float s) -> float {
				if (s <= arc.front()) return spd.front();
				for (size_t i = 1; i < m; ++i)
					if (arc[i] >= s) { float seg = std::max(1e-3f, arc[i] - arc[i - 1]); return spd[i - 1] + (spd[i] - spd[i - 1]) * ((s - arc[i - 1]) / seg); }
				return spd.back();
			};
			const float thr = 1.8f; // metres of prominence over a ~30 m window
			for (auto& box : rb.boxes)
			{
				if (box.type == model::BoxType::Start || box.type == model::BoxType::Finish) continue;
				float s = static_cast<float>(box.distTotalKm * 1000.0);
				float lo = elevAt(s - 30.0f), c1 = elevAt(s - 15.0f), c0 = elevAt(s), c2 = elevAt(s + 15.0f), hi = elevAt(s + 30.0f);
				float pk     = std::max({ c1, c0, c2 });
				float valley = std::min({ c1, c0, c2 });
				// "bump" and "dip" are the official lexicon sprites that exist in
				// picto::kAll (there is no separate "crest" sprite).
				if (pk - lo > thr && pk - hi > thr)
				{
					// a crest sharp + fast enough that following it would need more than g of
					// down-force = the bike goes airborne = a jump; a gentle or slow crest you
					// stay grounded over is just a "bump" (no spam on rolling terrain).
					const float D = 12.0f;
					float curv = (elevAt(s - D) - 2.0f * c0 + elevAt(s + D)) / (D * D); // < 0 at a crest
					float vc = speedAt(s);
					bool jump = (-curv > 0.0f) && (vc * vc * (-curv) > 12.0f) && (pk - lo > 2.0f);
					box.pictograms.push_back(jump ? "jump" : "bump");
					if (jump && box.dangerLevel < 1) box.dangerLevel = 1; // a jump warrants a caution
				}
				else if (lo - valley > thr && hi - valley > thr)
				{
					// classify the dip by how fast it was ridden through:
					//   slow = water crossing / wading;  hit fast = compression (g-out)
					float vmin = speedAt(s), vsur = std::max(speedAt(s - 28.0f), speedAt(s + 28.0f));
					const char* dipSign = "dip";
					if (vsur > 9.0f && vmin < 0.55f * vsur)       dipSign = "water";
					else if (vmin > 11.0f && vmin > 0.82f * vsur) dipSign = "compression";
					box.pictograms.push_back(dipSign);
				}
				// a turn / hairpin on a steep sustained descent = a "downhill" hazard note
				if (box.type == model::BoxType::Turn || box.type == model::BoxType::Hairpin)
				{
					float grade = (elevAt(s + 45.0f) - elevAt(s - 45.0f)) / 90.0f;
					if (grade < -0.12f) box.pictograms.push_back("downhill");
				}
			}
		}
	}

	// surface: tag sections clearly rougher than the track median as "bumpy"
	{
		const size_t m = ride.size();
		if (m >= 5)
		{
			std::vector<float> arc(m), elev(m); float ac = 0.0f;
			for (size_t i = 0; i < m; ++i) { if (i) { float dx = ride[i].x - ride[i-1].x, dz = ride[i].z - ride[i-1].z; ac += std::sqrt(dx*dx + dz*dz); } arc[i] = ac; elev[i] = ride[i].y; }
			std::vector<float> rough(rb.boxes.size(), -1.0f);
			for (size_t bi = 0; bi < rb.boxes.size(); ++bi)
			{
				if (rb.boxes[bi].type == model::BoxType::Start || rb.boxes[bi].type == model::BoxType::Finish) continue;
				float s = static_cast<float>(rb.boxes[bi].distTotalKm * 1000.0);
				double sx = 0, sy = 0, sxx = 0, sxy = 0; int rc = 0;
				for (size_t k = 0; k < m; ++k) { if (arc[k] < s - 30.0f || arc[k] > s + 30.0f) continue; sx += arc[k]; sy += elev[k]; sxx += (double)arc[k]*arc[k]; sxy += (double)arc[k]*elev[k]; rc++; }
				if (rc > 4) // RMS of elevation about its linear trend = bump amplitude
				{
					double den = rc * sxx - sx * sx; double b = (den != 0) ? (rc * sxy - sx * sy) / den : 0; double a = (sy - b * sx) / rc; double ss = 0;
					for (size_t k = 0; k < m; ++k) { if (arc[k] < s - 30.0f || arc[k] > s + 30.0f) continue; double r = elev[k] - (a + b * arc[k]); ss += r * r; }
					rough[bi] = static_cast<float>(std::sqrt(ss / rc));
				}
				else rough[bi] = 0.0f;
			}
			std::vector<float> srt; for (float v : rough) if (v >= 0.0f) srt.push_back(v);
			std::sort(srt.begin(), srt.end());
			float med = srt.empty() ? 0.0f : srt[srt.size() / 2];
			for (size_t bi = 0; bi < rb.boxes.size(); ++bi)
				if (rough[bi] > med * 1.8f && rough[bi] > 0.12f) rb.boxes[bi].pictograms.push_back("bumpy");
		}
	}

	// merge boxes that ended up coincident / too close (overlapping turn + elevation
	// + crossing waypoints) so the book isn't cluttered with duplicates
	{
		const float mergeM = 12.0f;
		auto prio = [](const model::Box& b) {
			int p = b.dangerLevel * 10;
			if (b.type == model::BoxType::Hairpin) p += 4; else if (b.type == model::BoxType::Turn) p += 3;
			else if (b.type == model::BoxType::Straight) p += 1;
			if (b.crossDeg > -900.0f) p += 1;
			return p + static_cast<int>(b.pictograms.size());
		};
		std::vector<model::Box> merged;
		for (const auto& b : rb.boxes)
		{
			if (!merged.empty())
			{
				model::Box& prev = merged.back();
				bool mergeable = b.type != model::BoxType::Start && b.type != model::BoxType::Finish
				              && prev.type != model::BoxType::Start && prev.type != model::BoxType::Finish
				              && (b.distTotalKm - prev.distTotalKm) * 1000.0 < mergeM;
				if (mergeable)
				{
					bool bWins = prio(b) >= prio(prev);
					model::Box keep = bWins ? b : prev;
					const model::Box& other = bWins ? prev : b;
					for (const auto& pg : other.pictograms) if (std::find(keep.pictograms.begin(), keep.pictograms.end(), pg) == keep.pictograms.end()) keep.pictograms.push_back(pg);
					for (float bd : other.branchDeg) keep.branchDeg.push_back(bd);
					if (keep.crossDeg <= -900.0f && other.crossDeg > -900.0f) keep.crossDeg = other.crossDeg;
					merged.back() = keep;
					continue;
				}
			}
			// drop a redundant, sign-less "keep straight" right after the start
			if (!merged.empty() && b.type == model::BoxType::Straight && b.pictograms.empty()
			    && merged.back().type == model::BoxType::Start
			    && (b.distTotalKm - merged.back().distTotalKm) * 1000.0 < 12.0) continue;
			merged.push_back(b);
		}
		rb.boxes = merged;
		double prevKm = 0.0;
		for (auto& b : rb.boxes) { b.distPartialM = static_cast<int>(std::lround((b.distTotalKm - prevKm) * 1000.0)); prevKm = b.distTotalKm; }
	}

	for (size_t i = 0; i < rb.boxes.size(); ++i) rb.boxes[i].index = static_cast<int>(i);
	rb.totalDistanceKm = (n - 1) * p.resampleM / 1000.0;

	// Downsampled route polyline (~3 m) for map-matching + minimap.
	// elevation per route point (from recorded altitude by arc length)
	std::vector<float> eArc(ride.size()), eY(ride.size());
	{ float ac = 0.0f; for (size_t i = 0; i < ride.size(); ++i) { if (i) { float dx = ride[i].x - ride[i-1].x, dz = ride[i].z - ride[i-1].z; ac += std::sqrt(dx*dx + dz*dz); } eArc[i] = ac; eY[i] = ride[i].y; } }
	auto eAt = [&](float s) -> float { if (ride.empty()) return 0.0f; if (s <= eArc.front()) return eY.front(); for (size_t i = 1; i < ride.size(); ++i) if (eArc[i] >= s) { float sg = std::max(1e-3f, eArc[i] - eArc[i-1]); return eY[i-1] + (eY[i] - eY[i-1]) * ((s - eArc[i-1]) / sg); } return eY.back(); };
	const int stride = std::max(1, static_cast<int>(std::lround(2.0f / p.resampleM)));
	for (int i = 0; i < n; i += stride)
		rb.route.push_back({ pts[i].x, pts[i].y, i * p.resampleM, eAt(i * p.resampleM) });
	if ((n - 1) % stride != 0)
		rb.route.push_back({ pts[n - 1].x, pts[n - 1].y, (n - 1) * p.resampleM, eAt((n - 1) * p.resampleM) });

	return rb;
}

// Multi-ride union: add un-taken junction branches. For each box, look at the
// OTHER recorded rides; where one passes through the same junction but leaves on
// a clearly different heading, record that as a faint branch spoke on the tulip.
void addBranches(model::Roadbook& rb, const std::vector<std::vector<RideSample>>& others, const Params& p)
{
	(void)p;
	if (rb.route.empty()) return;
	auto routeAt = [&](double s) -> geo::Vec2 {
		if (s <= rb.route.front().distM) return { rb.route.front().x, rb.route.front().z };
		for (size_t i = 1; i < rb.route.size(); ++i)
			if (rb.route[i].distM >= s) { double seg = (std::max)(1e-3, static_cast<double>(rb.route[i].distM - rb.route[i - 1].distM)); double t = (s - rb.route[i - 1].distM) / seg; return { static_cast<float>(rb.route[i - 1].x + (rb.route[i].x - rb.route[i - 1].x) * t), static_cast<float>(rb.route[i - 1].z + (rb.route[i].z - rb.route[i - 1].z) * t) }; }
		return { rb.route.back().x, rb.route.back().z };
	};
	for (auto& box : rb.boxes)
	{
		if (box.type == model::BoxType::Start || box.type == model::BoxType::Finish) continue;
		geo::Vec2 bp = routeAt(box.distTotalKm * 1000.0);
		float boxHeading = static_cast<float>(box.capDeg);
		for (const auto& ride : others)
		{
			if (ride.size() < 3) continue;
			int best = -1; float bestd = 14.0f;
			for (int i = 0; i < static_cast<int>(ride.size()); ++i) { float dx = ride[i].x - bp.x, dz = ride[i].z - bp.y; float d = std::sqrt(dx * dx + dz * dz); if (d < bestd) { bestd = d; best = i; } }
			if (best < 1 || best >= static_cast<int>(ride.size()) - 1) continue;
			float dx = ride[best + 1].x - ride[best - 1].x, dz = ride[best + 1].z - ride[best - 1].z;
			if (std::fabs(dx) + std::fabs(dz) < 0.5f) continue;
			float rel = geo::wrap180(geo::headingDeg({ dx, dz }) - boxHeading);
			if (std::fabs(rel) < 25.0f || std::fabs(rel) > 160.0f) continue;   // not divergent / just the opposite direction
			bool dup = false; for (float b : box.branchDeg) if (std::fabs(geo::wrap180(b - rel)) < 20.0f) dup = true;
			if (!dup && box.branchDeg.size() < 4) box.branchDeg.push_back(rel);
		}
	}
}

} // namespace gen
