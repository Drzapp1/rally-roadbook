// ============================================================================
// test_main.cpp  —  offline harness for the tulip generator
//
//   RoadbookTests [ride.csv] [out.svg]
//
// With no CSV it builds a synthetic route (a few turns + a hairpin) so the
// pipeline can be validated without the game. It prints a box summary and
// writes an SVG preview: the route with numbered waypoints, plus a grid of
// tulip thumbnails (entry dot + path + exit arrow).
// ============================================================================
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "generate/tulip_generator.h"
#include "generate/geometry.h"
#include "model/roadbook.h"
#include "model/roadbook_json.h"

using gen::RideSample;

static std::vector<RideSample> loadCsv(const char* path)
{
	std::vector<RideSample> out;
	FILE* f = std::fopen(path, "r");
	if (!f) { std::printf("ERROR: cannot open %s\n", path); return out; }
	char line[512];
	while (std::fgets(line, sizeof line, f))
	{
		if (line[0] == '#' || line[0] == 't' || line[0] == '\n') continue; // comment/header/blank
		float t, x, y, z, yaw, vx, vy, vz, sp, pos, dist;
		int got = std::sscanf(line, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
		                      &t, &x, &y, &z, &yaw, &vx, &vy, &vz, &sp, &pos, &dist);
		if (got >= 4)
		{
			RideSample s;
			s.x = x; s.z = z; s.y = y;
			s.yaw = (got >= 5) ? yaw : 0.0f;
			s.speed = (got >= 9) ? sp : 0.0f;
			s.dist = (got >= 11) ? dist : 0.0f;
			out.push_back(s);
		}
	}
	std::fclose(f);
	return out;
}

static std::vector<RideSample> syntheticRoute()
{
	// Sequence of (length m, total turn deg over the leg). +turn = right.
	struct Leg { float len; float turn; };
	const Leg legs[] = {
		{120, 0}, {60, 90}, {150, 0}, {45, 150}, {120, 0}, {55, -85}, {100, 0}, {40, 60}, {80, 0}
	};
	std::vector<RideSample> r;
	float x = 0, z = 0, heading = 0; // compass deg, 0 = north(+z)
	const float pi = 3.14159265358979f;
	for (const Leg& leg : legs)
	{
		int steps = static_cast<int>(leg.len);
		float dTurn = (steps > 0) ? leg.turn / steps : 0.0f;
		for (int i = 0; i < steps; ++i)
		{
			heading += dTurn;
			float hr = heading * pi / 180.0f;
			x += std::sin(hr); z += std::cos(hr); // 1 m step
			RideSample s; s.x = x; s.z = z; s.yaw = heading;
			r.push_back(s);
		}
	}
	return r;
}

// --- SVG output -------------------------------------------------------------
static void writeSvg(const char* path, const std::vector<RideSample>& ride,
                     const model::Roadbook& rb, const gen::Params& P)
{
	// Recreate the resampled polyline to place box markers on the route map.
	std::vector<geo::Vec2> raw;
	for (const auto& s : ride) raw.push_back({ s.x, s.z });
	std::vector<geo::Vec2> pts = geo::smooth(geo::resampleByArcLength(raw, P.resampleM), P.smoothHalfWindow);

	float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
	for (const auto& p : pts) { minX = std::min(minX, p.x); maxX = std::max(maxX, p.x); minZ = std::min(minZ, p.y); maxZ = std::max(maxZ, p.y); }
	const float mapW = 700.0f, pad = 30.0f;
	float spanX = std::max(1.0f, maxX - minX), spanZ = std::max(1.0f, maxZ - minZ);
	float sc = (mapW - 2 * pad) / std::max(spanX, spanZ);
	float mapH = spanZ * sc + 2 * pad;
	auto sx = [&](float wx) { return pad + (wx - minX) * sc; };
	auto sy = [&](float wz) { return pad + (maxZ - wz) * sc; }; // north up

	const int cols = 6, cell = 120, gap = 14, labelH = 34;
	int nThumbs = static_cast<int>(rb.boxes.size());
	int rows = (nThumbs + cols - 1) / cols;
	float gridTop = mapH + 20;
	float gridH = rows * (cell + labelH + gap) + 20;
	float totalH = gridTop + gridH;
	float totalW = std::max(mapW, static_cast<float>(cols * (cell + gap) + gap));

	FILE* f = std::fopen(path, "w");
	if (!f) { std::printf("ERROR: cannot write %s\n", path); return; }
	std::fprintf(f, "<svg xmlns='http://www.w3.org/2000/svg' width='%.0f' height='%.0f' "
	                "font-family='monospace' font-size='11'>\n", totalW, totalH);
	std::fprintf(f, "<rect width='100%%' height='100%%' fill='#10141a'/>\n");

	// Route polyline.
	std::fprintf(f, "<polyline fill='none' stroke='#3a86ff' stroke-width='2' points='");
	for (const auto& p : pts) std::fprintf(f, "%.1f,%.1f ", sx(p.x), sy(p.y));
	std::fprintf(f, "'/>\n");

	// Box markers on the route.
	for (const auto& b : rb.boxes)
	{
		int idx = static_cast<int>(std::lround(b.distTotalKm * 1000.0 / P.resampleM));
		idx = std::max(0, std::min(static_cast<int>(pts.size()) - 1, idx));
		float mx = sx(pts[idx].x), my = sy(pts[idx].y);
		const char* col = (b.dangerLevel > 0) ? "#ff5a3c" :
		                  (b.type == model::BoxType::Straight) ? "#9aa0a6" : "#ffbe28";
		std::fprintf(f, "<circle cx='%.1f' cy='%.1f' r='4' fill='%s'/>\n", mx, my, col);
		std::fprintf(f, "<text x='%.1f' y='%.1f' fill='#e8eaed'>%d</text>\n", mx + 6, my - 4, b.index);
	}

	// Tulip thumbnails.
	for (int i = 0; i < nThumbs; ++i)
	{
		const model::Box& b = rb.boxes[i];
		int r = i / cols, c = i % cols;
		float cx = gap + c * (cell + gap);
		float cy = gridTop + r * (cell + labelH + gap);
		std::fprintf(f, "<rect x='%.1f' y='%.1f' width='%d' height='%d' fill='#181d25' stroke='#2a313c'/>\n",
		             cx, cy, cell, cell);
		auto tx = [&](float px) { return cx + px * cell; };
		auto ty = [&](float py) { return cy + (1.0f - py) * cell; }; // travel-up
		if (b.tulip.size() >= 2)
		{
			std::fprintf(f, "<polyline fill='none' stroke='#ffffff' stroke-width='2.5' points='");
			for (const auto& tp : b.tulip) std::fprintf(f, "%.1f,%.1f ", tx(tp.x), ty(tp.y));
			std::fprintf(f, "'/>\n");
		}
		// entry dot
		std::fprintf(f, "<circle cx='%.1f' cy='%.1f' r='4' fill='#5adc78'/>\n", tx(b.entry.x), ty(b.entry.y));
		// exit arrow head
		std::fprintf(f, "<circle cx='%.1f' cy='%.1f' r='3' fill='#ffbe28'/>\n", tx(b.exitAt.x), ty(b.exitAt.y));
		// label
		std::fprintf(f, "<text x='%.1f' y='%.1f' fill='#e8eaed'>#%d %s</text>\n",
		             cx + 2, cy + cell + 13, b.index, model::toString(b.type));
		std::fprintf(f, "<text x='%.1f' y='%.1f' fill='#9aa0a6'>%.2fkm %dm cap%03d %s</text>\n",
		             cx + 2, cy + cell + 27, b.distTotalKm, b.distPartialM, b.capDeg,
		             (b.turnDir == model::TurnDir::None) ? "" : model::toString(b.turnDir));
	}

	std::fprintf(f, "</svg>\n");
	std::fclose(f);
	std::printf("wrote SVG preview: %s\n", path);
}

// ---- synthetic rides for the regression self-test ----
static std::vector<RideSample> straightRide(float len, float speed)
{
	std::vector<RideSample> r;
	for (float d = 0.0f; d <= len; d += 2.0f) { RideSample s{}; s.x = d; s.z = 0.0f; s.y = 100.0f; s.speed = speed; s.dist = d; r.push_back(s); }
	return r;
}
static std::vector<RideSample> figure8Ride()
{
	std::vector<RideSample> r; float dist = 0.0f; bool first = true; RideSample prev{};
	for (float t = 0.0f; t < 6.30f; t += 0.02f)
	{
		RideSample s{}; s.x = 100.0f * std::sin(t); s.z = 50.0f * std::sin(2.0f * t); s.y = 100.0f; s.speed = 12.0f;
		if (!first) { float dx = s.x - prev.x, dz = s.z - prev.z; dist += std::sqrt(dx * dx + dz * dz); }
		s.dist = dist; r.push_back(s); prev = s; first = false;
	}
	return r;
}
static std::vector<RideSample> crestDipRide()
{
	std::vector<RideSample> r;
	for (float d = 0.0f; d <= 400.0f; d += 2.0f)
	{
		RideSample s{}; s.x = d; s.z = 0.0f; float y = 100.0f, speed = 16.0f;
		if (std::fabs(d - 100.0f) < 22.0f) y += 4.5f * std::cos((d - 100.0f) / 22.0f * 1.5708f);          // crest
		if (std::fabs(d - 250.0f) < 22.0f) { y -= 4.0f * std::cos((d - 250.0f) / 22.0f * 1.5708f); speed = 6.0f; } // slow dip
		s.y = y; s.speed = speed; s.dist = d; r.push_back(s);
	}
	return r;
}
static int runSelfTest()
{
	int pass = 0, fail = 0;
	auto check = [&](const char* name, bool cond) { std::printf("[%s] %s\n", cond ? "PASS" : "FAIL", name); if (cond) ++pass; else ++fail; };
	gen::Params P;
	{ auto rb = gen::generate({}, P, "t", "empty"); check("empty ride -> 0 boxes, no crash", rb.boxes.empty()); }
	{ auto rb = gen::generate(straightRide(600.0f, 15.0f), P, "t", "straight");
	  bool cross = false; for (auto& b : rb.boxes) if (b.crossDeg > -900.0f) cross = true;
	  check("straight -> >=2 boxes", rb.boxes.size() >= 2); check("straight -> no false crossing", !cross); }
	{ auto rb = gen::generate(figure8Ride(), P, "t", "fig8");
	  int cr = 0; for (auto& b : rb.boxes) if (b.crossDeg > -900.0f) ++cr;
	  check("figure-8 -> >=1 crossing box", cr >= 1); }
	{ auto rb = gen::generate(crestDipRide(), P, "t", "crestdip");
	  bool bump = false, dipw = false; for (auto& b : rb.boxes) for (auto& pg : b.pictograms) { if (pg == "bump") bump = true; if (pg == "dip" || pg == "water") dipw = true; }
	  check("crest/dip -> bump sign present", bump); check("crest/dip -> dip/water sign present", dipw); }
	{ std::vector<RideSample> r; for (float d = 0; d <= 300; d += 2.0f) { RideSample s{}; s.x = d; s.z = 0; float y = 100, sp = 18; if (std::fabs(d - 150) < 22) { y -= 4.0f * std::cos((d - 150) / 22 * 1.5708f); sp = 17; } s.y = y; s.speed = sp; s.dist = d; r.push_back(s); }
	  auto rb = gen::generate(r, P, "t", "compression"); bool comp = false; for (auto& b : rb.boxes) for (auto& pg : b.pictograms) if (pg == "compression") comp = true;
	  check("fast dip -> compression sign", comp); }
	{ std::vector<RideSample> r; float x = 0, z = 0, y = 160, head = 0, dist = 0;
	  for (int i = 0; i < 140; ++i) { RideSample s{}; if (i > 70) head += 0.03f; x += 2.0f * std::sin(head); z += 2.0f * std::cos(head); y -= 0.5f; s.x = x; s.z = z; s.y = y; s.speed = 12; s.dist = dist; dist += 2.0f; r.push_back(s); }
	  auto rb = gen::generate(r, P, "t", "downhill"); bool dh = false; for (auto& b : rb.boxes) for (auto& pg : b.pictograms) if (pg == "downhill") dh = true;
	  check("descending turn -> downhill sign", dh); }
	{ std::vector<RideSample> r; for (float d = 0; d <= 240; d += 2.0f) { RideSample s{}; s.x = d; s.z = 0; float y = 100, sp = 19; if (std::fabs(d - 120) < 13) y += 4.0f * std::cos((d - 120) / 13 * 1.5708f); s.y = y; s.speed = sp; s.dist = d; r.push_back(s); }
	  auto rb = gen::generate(r, P, "t", "jump"); bool jp = false; for (auto& b : rb.boxes) for (auto& pg : b.pictograms) if (pg == "jump") jp = true;
	  check("sharp fast crest -> jump sign", jp); }
	{ std::vector<RideSample> r; for (float d = 0; d <= 240; d += 2.0f) { RideSample s{}; s.x = d; s.z = 0; float y = 100, sp = 8; if (std::fabs(d - 120) < 13) y += 4.0f * std::cos((d - 120) / 13 * 1.5708f); s.y = y; s.speed = sp; s.dist = d; r.push_back(s); }
	  auto rb = gen::generate(r, P, "t", "slowcrest"); bool jp = false, bp = false; for (auto& b : rb.boxes) for (auto& pg : b.pictograms) { if (pg == "jump") jp = true; if (pg == "bump") bp = true; }
	  check("same crest ridden slow -> bump not jump", bp && !jp); }
	{ auto rb = gen::generate(straightRide(300.0f, 15.0f), P, "t", "json");
	  std::string p = "D:\\BikesRoadbook\\build\\_selftest.json"; bool w = model::writeJson(rb, p);
	  model::Roadbook rb2; bool rd = model::readJson(p, rb2);
	  check("JSON round-trip preserves box count", w && rd && rb.boxes.size() == rb2.boxes.size()); }
	{ std::string p = "D:\\BikesRoadbook\\build\\_selftest_hist.json"; std::remove(p.c_str());
	  model::RunRecord a; a.track = "X"; a.date = "2026"; a.timeSec = 90; a.distKm = 1.5; a.score = 80; a.grade = "B";
	  model::appendRunRecord(p, a); a.score = 70; model::appendRunRecord(p, a);
	  auto h = model::loadRunHistory(p);
	  check("history append+load -> 2 records", h.size() == 2);
	  check("history fields preserved", h.size() == 2 && h[0].timeSec == 90 && h[0].grade == "B"); }
	{ auto rb = gen::generate(figure8Ride(), P, "t", "tulip"); size_t mx = 0; for (auto& b : rb.boxes) mx = (std::max)(mx, b.tulip.size());
	  check("tulips simplified (<=40 pts)", mx <= 40); }
	{ auto r = straightRide(300.0f, 15.0f); r[40].x = std::numeric_limits<float>::quiet_NaN(); r[80].y = std::numeric_limits<float>::infinity();
	  auto rb = gen::generate(r, P, "t", "nan"); check("non-finite samples filtered -> still generates", rb.boxes.size() >= 2); }
	{ model::BestRun b; b.score = 88; b.timeSec = 123; b.traceX = {1, 2, 3}; b.traceZ = {4, 5, 6}; b.off = {0, 1, 0}; b.boxTimeMs = {1000, 2500};
	  std::string p = "D:\\BikesRoadbook\\build\\_selftest_best.json"; model::saveBestRun(p, b);
	  model::BestRun o; bool ok = model::loadBestRun(p, o);
	  check("best-run round-trip", ok && o.score == 88 && o.timeSec == 123 && o.traceX.size() == 3 && o.boxTimeMs.size() == 2); }
	// degenerate inputs must not crash or hang
	{ std::vector<RideSample> one(1); one[0].x = 5; one[0].z = 5; one[0].y = 100; auto rb = gen::generate(one, P, "t", "single"); check("single-point ride -> no crash", rb.boxes.size() <= 2); }
	{ std::vector<RideSample> same(200); for (auto& s : same) { s.x = 3; s.z = 7; s.y = 100; s.speed = 0; } auto rb = gen::generate(same, P, "t", "stationary"); check("stationary ride (200 identical) -> no crash", true); (void)rb; }
	{ std::vector<RideSample> big; big.reserve(40000); for (int i = 0; i < 40000; ++i) { RideSample s{}; s.x = i * 0.5f; s.z = 20.0f * std::sin(i * 0.001f); s.y = 100.0f; s.speed = 18.0f; s.dist = i * 0.5f; big.push_back(s); } auto rb = gen::generate(big, P, "t", "huge"); check("huge ride (40k samples) -> completes, route bounded", rb.route.size() < 20000 && !rb.boxes.empty()); }
	std::printf("\nSELFTEST: %d passed, %d failed\n", pass, fail);
	return fail == 0 ? 0 : 1;
}

int main(int argc, char** argv)
{
	if (argc >= 2 && std::string(argv[1]) == "selftest") return runSelfTest();
	if (argc >= 4 && std::string(argv[1]) == "union")
	{
		std::vector<RideSample> primary = loadCsv(argv[2]);
		std::vector<std::vector<RideSample>> others;
		for (int i = 3; i < argc; ++i) { auto o = loadCsv(argv[i]); if (!o.empty()) others.push_back(o); }
		gen::Params P;
		model::Roadbook rb = gen::generate(primary, P, "union", argv[2]);
		gen::addBranches(rb, others, P);
		int branched = 0, spokes = 0;
		for (auto& b : rb.boxes) if (!b.branchDeg.empty()) { ++branched; spokes += static_cast<int>(b.branchDeg.size()); }
		std::printf("union: primary %zu samples + %zu rides -> %zu boxes; %d junctions with branches (%d spokes)\n",
		            primary.size(), others.size(), rb.boxes.size(), branched, spokes);
		model::writeJson(rb, "D:\\BikesRoadbook\\build\\union.json");
		std::printf("wrote D:\\BikesRoadbook\\build\\union.json\n");
		return 0;
	}
	std::vector<RideSample> ride;
	std::string src;
	if (argc >= 2) { ride = loadCsv(argv[1]); src = argv[1]; }
	if (ride.empty()) { ride = syntheticRoute(); src = "(synthetic route)"; }

	gen::Params P;
	model::Roadbook rb = gen::generate(ride, P, "test", src);

	std::printf("source: %s  (%zu samples)\n", src.c_str(), ride.size());
	std::printf("total distance: %.3f km   boxes: %zu\n", rb.totalDistanceKm, rb.boxes.size());
	std::printf("%-4s %-9s %-7s %-6s %-6s %-8s %-6s %s\n",
	            "#", "type", "totKm", "partM", "cap", "dir", "danger", "tulipPts");
	for (const auto& b : rb.boxes)
	{
		std::printf("%-4d %-9s %-7.3f %-6d %-6d %-8s %-6d %zu\n",
		            b.index, model::toString(b.type), b.distTotalKm, b.distPartialM,
		            b.capDeg, model::toString(b.turnDir), b.dangerLevel, b.tulip.size());
	}

	const char* out = (argc >= 3) ? argv[2] : "D:\\BikesRoadbook\\build\\roadbook_preview.svg";

	// JSON write + round-trip verify.
	std::string jsonPath = out;
	size_t dot = jsonPath.rfind('.');
	jsonPath = (dot != std::string::npos ? jsonPath.substr(0, dot) : jsonPath) + ".json";
	if (model::writeJson(rb, jsonPath))
	{
		model::Roadbook rb2;
		bool ok = model::readJson(jsonPath, rb2);
		std::printf("JSON: wrote %s; round-trip %s (boxes %zu -> %zu)\n",
		            jsonPath.c_str(), ok ? "OK" : "FAIL", rb.boxes.size(), rb2.boxes.size());
	}
	else std::printf("JSON: write FAILED for %s\n", jsonPath.c_str());

	// Map-matching sanity: projecting a route vertex onto the route should
	// return ~zero offset, and a point offset sideways should report that offset.
	if (!rb.route.empty())
	{
		std::vector<geo::Vec2> rpts;
		for (const auto& r : rb.route) rpts.push_back({ r.x, r.z });
		int mid = static_cast<int>(rb.route.size() / 2);
		geo::Vec2 q{ rb.route[mid].x, rb.route[mid].z };
		auto pr = geo::projectToPolyline(rpts, q, -1, 0);
		geo::Vec2 q2{ q.x + 5.0f, q.y };
		auto pr2 = geo::projectToPolyline(rpts, q2, mid, 30);
		std::printf("route: %zu pts; self-match offset=%.3f (expect ~0); +5m offset=%.3f (expect ~5)\n",
		            rb.route.size(), pr.dist, pr2.dist);

		// Nav simulation: replay the ride through map-matching against its own
		// route (the exact code path the plugin uses) and check the trip tracks.
		int hint = -1, backjumps = 0;
		double prevTrip = -1.0, finalTrip = 0.0, maxOff = 0.0;
		for (size_t k = 0; k < ride.size(); k += 5)
		{
			auto m = geo::projectToPolyline(rpts, { ride[k].x, ride[k].z }, hint, 40, 30.0f);
			if (m.seg < 0) continue;
			hint = m.seg;
			double trip = rb.route[m.seg].distM + m.t * (rb.route[m.seg + 1].distM - rb.route[m.seg].distM);
			maxOff = std::max(maxOff, static_cast<double>(m.dist));
			if (prevTrip >= 0 && trip < prevTrip - 5.0) ++backjumps;
			prevTrip = trip; finalTrip = trip;
		}
		std::printf("nav sim: final trip=%.0fm (route %.0fm)  maxOff=%.2fm  backjumps=%d\n",
		            finalTrip, rb.route.back().distM, maxOff, backjumps);
	}

	writeSvg(out, ride, rb, P);
	return 0;
}
