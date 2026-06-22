#include "roadbook_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "primitives.h"

namespace render {
namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kD2R = kPi / 180.0f;
constexpr float kAspect = 9.0f / 16.0f;
constexpr float kLine   = 0.0013f;

constexpr float kLeft = 0.832f, kRight = 0.988f;
constexpr float kCellH = 0.150f, kGap = 0.006f, kStripTop = 0.110f;
constexpr int   kVisible = 5;

// ---- style ----------------------------------------------------------------
struct Style
{
	const char* name;
	unsigned long boxBg, boxBgA, ink, inkDim, line, ball, danger, borderA, borderI;
	unsigned long tripBg, tripValue;
	unsigned long hudBg, hudInk, hudDim, hudAccent, hudGood, hudWarn;
	unsigned long routeHi; // tulip route highlight (the cyan line on a real roadbook)
};

Style getStyle(int idx)
{
	switch (idx)
	{
	default: // 0 Paper (authentic white roadbook)
		return { "Paper",
			abgr(243,241,234), abgr(252,251,247), abgr(24,21,19), abgr(120,114,106), abgr(24,21,19), abgr(24,21,19),
			abgr(206,38,28), abgr(232,148,22), abgr(70,66,60),
			abgr(16,20,26), abgr(90,220,120),
			abgr(16,20,26), abgr(235,235,235), abgr(150,150,150), abgr(255,190,40), abgr(90,220,120), abgr(240,90,60),
			abgr(40,185,235) };
	case 1: // Dakar amber
		return { "Dakar",
			abgr(40,32,18), abgr(58,46,26), abgr(242,222,170), abgr(176,150,108), abgr(238,182,92), abgr(255,200,90),
			abgr(240,95,40), abgr(255,180,40), abgr(120,96,52),
			abgr(26,18,8), abgr(255,190,60),
			abgr(28,20,10), abgr(245,228,185), abgr(170,148,108), abgr(255,185,55), abgr(150,220,120), abgr(245,110,50),
			abgr(255,180,60) };
	case 2: // Carbon / neon
		return { "Carbon",
			abgr(20,22,26), abgr(30,34,40), abgr(222,226,230), abgr(120,128,136), abgr(222,226,230), abgr(222,226,230),
			abgr(255,70,90), abgr(60,230,180), abgr(70,78,86),
			abgr(14,16,20), abgr(60,230,180),
			abgr(14,16,20), abgr(225,228,232), abgr(120,128,136), abgr(60,230,180), abgr(120,240,150), abgr(255,80,90),
			abgr(60,230,180) };
	case 3: // Blueprint
		return { "Blueprint",
			abgr(18,28,48), abgr(26,40,66), abgr(200,225,255), abgr(95,135,185), abgr(200,225,255), abgr(200,225,255),
			abgr(255,120,120), abgr(125,200,255), abgr(60,92,140),
			abgr(12,20,36), abgr(125,200,255),
			abgr(12,20,36), abgr(165,205,250), abgr(95,135,185), abgr(125,200,255), abgr(130,230,200), abgr(255,140,120),
			abgr(120,200,255) };
	}
}

int   g_opacity = 200;
Style g_style;
bool  g_noCap = false, g_distOnly = false, g_maskBox = false; // training-mode flags
bool  g_french = false;                                       // French roadbook abbreviations (G/D/TD)

// animation state (single renderer, one game thread)
bool  g_animInit = false, g_posInit = false;
unsigned long long g_lastMs = 0;
float g_dt = 0.0f, g_time = 0.0f;
float g_scroll = 0.0f, g_dHead = 0.0f, g_dTarget = 0.0f;
geo::Vec2 g_dPos{};

unsigned long withA(unsigned long c, int a) { return (c & 0x00FFFFFFul) | (static_cast<unsigned long>(a & 0xFF) << 24); }
unsigned long boxBg(bool act) { return withA(act ? g_style.boxBgA : g_style.boxBg, g_opacity); }
unsigned long tripBg()  { return withA(g_style.tripBg, std::min(255, g_opacity + 25)); }
unsigned long hudBg()   { return withA(g_style.hudBg, g_opacity); }

float wrap180(float d) { while (d <= -180.0f) d += 360.0f; while (d > 180.0f) d -= 360.0f; return d; }
float wrap360(float d) { d = std::fmod(d, 360.0f); if (d < 0) d += 360.0f; return d; }
float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
float easeAngle(float cur, float target, float rate) { return cur + wrap180(target - cur) * clamp01(rate * g_dt); }

int activeBox(const model::Roadbook& rb, const NavState& nav)
{
	if (nav.manualBox >= 0 && nav.manualBox < static_cast<int>(rb.boxes.size())) return nav.manualBox;
	int a = 0;
	for (int i = 0; i < static_cast<int>(rb.boxes.size()); ++i)
		if (rb.boxes[i].distTotalKm * 1000.0 <= nav.tripM + nav.advanceLookM) a = i; else break;
	return a;
}

void box4(DrawCache& dc, float x, float y, float w, float h, unsigned long c, float lw = kLine)
{
	dc.line(x, y, x + w, y, lw, c);
	dc.line(x, y + h, x + w, y + h, lw, c);
	dc.line(x, y, x, y + h, lw, c);
	dc.line(x + w, y, x + w, y + h, lw, c);
}

void drawArrow(DrawCache& dc, float cx, float cy, float size, float relDeg, unsigned long color)
{
	float r = relDeg * kD2R, dx = std::sin(r), dy = -std::cos(r);
	auto X = [&](float ox) { return cx + ox * kAspect; };
	float tipX = X(dx * size), tipY = cy + dy * size;
	dc.line(X(-dx * size * 0.6f), cy - dy * size * 0.6f, tipX, tipY, kLine, color);
	float c = std::cos(0.56f), s = std::sin(0.56f);
	float b1x = -dx * c + dy * s, b1y = -dx * s - dy * c;
	float b2x = -dx * c - dy * s, b2y = dx * s - dy * c;
	dc.line(tipX, tipY, X(dx * size + b1x * size * 0.55f), cy + dy * size + b1y * size * 0.55f, kLine, color);
	dc.line(tipX, tipY, X(dx * size + b2x * size * 0.55f), cy + dy * size + b2y * size * 0.55f, kLine, color);
}

// Draw sprite `sp` as a square of visual half-size s centred at (cx,cy),
// rotated so the sprite's top (its "up") faces screen direction (dirX,dirY).
// stock tulip catalog + turn classification — clean shapes mapped per turn,
// not the raw recorded path (x:0..1 L->R, y:0..1 where 1=top; entry first).
static std::vector<geo::Vec2> stockTulip(const std::string& code)
{
	if (code == "start")    return { {.5f,.10f},{.5f,.84f} };
	if (code == "slightL")  return { {.5f,.08f},{.5f,.52f},{.30f,.88f} };
	if (code == "slightR")  return { {.5f,.08f},{.5f,.52f},{.70f,.88f} };
	if (code == "left")     return { {.5f,.08f},{.5f,.56f},{.14f,.56f} };
	if (code == "right")    return { {.5f,.08f},{.5f,.56f},{.86f,.56f} };
	if (code == "sharpL")   return { {.5f,.08f},{.5f,.60f},{.26f,.36f} };
	if (code == "sharpR")   return { {.5f,.08f},{.5f,.60f},{.74f,.36f} };
	if (code == "hairpinL") return { {.5f,.08f},{.5f,.70f},{.30f,.80f},{.30f,.34f} };
	if (code == "hairpinR") return { {.5f,.08f},{.5f,.70f},{.70f,.80f},{.70f,.34f} };
	if (code == "finish")   return { {.5f,.08f},{.5f,.80f} };
	return { {.5f,.08f},{.5f,.92f} }; // straight
}
static float tulipBend(const std::vector<model::TulipPoint>& p)
{
	if (p.size() < 2) return 90.0f;
	size_t n = p.size();
	float v1x = p[1].x - p[0].x, v1y = p[1].y - p[0].y;
	float v2x = p[n-1].x - p[n-2].x, v2y = p[n-1].y - p[n-2].y;
	float m1 = std::sqrt(v1x*v1x + v1y*v1y), m2 = std::sqrt(v2x*v2x + v2y*v2y);
	if (m1 < 1e-5f || m2 < 1e-5f) return 90.0f;
	float c = std::max(-1.0f, std::min(1.0f, (v1x*v2x + v1y*v2y) / (m1*m2)));
	return std::acos(c) * 57.29578f;
}
static std::string tulipCode(const model::Box& b)
{
	bool R = (b.turnDir == model::TurnDir::Right);
	if (b.type == model::BoxType::Start)   return "start";
	if (b.type == model::BoxType::Finish)  return "finish";
	if (b.type == model::BoxType::Hairpin) return R ? "hairpinR" : "hairpinL";
	if (b.type == model::BoxType::Turn)
	{
		float m = tulipBend(b.tulip);
		if (m < 32.0f)   return R ? "slightR" : "slightL";
		if (m >= 100.0f) return R ? "sharpR" : "sharpL";
		return R ? "right" : "left";
	}
	return "straight";
}

void drawTulip(DrawCache& dc, const model::Box& b, float cx, float cy, float halfH,
               unsigned long lineColor, unsigned long inkColor)
{
	(void)inkColor;
	const float halfW = halfH * kAspect;
	const float halo = kLine * 2.6f, core = kLine * 2.1f;
	auto SX = [&](float px) { return cx + (px - 0.5f) * 2.0f * halfW; };
	auto SY = [&](float py) { return cy + (0.5f - py) * 2.0f * halfH; };

	// classify into a clean stock tulip instead of drawing the raw recorded path
	const std::string code = tulipCode(b);
	const std::vector<geo::Vec2> pts = stockTulip(code);

	// route highlight (cyan) under the dark line — the real-roadbook look
	for (size_t i = 1; i < pts.size(); ++i)
		dc.line(SX(pts[i - 1].x), SY(pts[i - 1].y), SX(pts[i].x), SY(pts[i].y), halo, g_style.routeHi);
	for (size_t i = 1; i < pts.size(); ++i)
		dc.line(SX(pts[i - 1].x), SY(pts[i - 1].y), SX(pts[i].x), SY(pts[i].y), core, lineColor);

	// junction crossing stub (faint) through the tulip centre
	if (b.crossDeg > -900.0f)
	{
		float ar = b.crossDeg * kD2R;
		float dxc = std::sin(ar), dyc = std::cos(ar), L = 0.40f;
		dc.line(SX(0.5f - dxc * L), SY(0.5f - dyc * L), SX(0.5f + dxc * L), SY(0.5f + dyc * L), kLine * 1.3f, withA(lineColor, 80));
	}

	// un-taken junction branches (faint spokes from the centre; multi-ride union)
	for (float bd : b.branchDeg)
	{
		float br = bd * kD2R;
		dc.line(SX(0.5f), SY(0.5f), SX(0.5f + std::sin(br) * 0.40f), SY(0.5f + std::cos(br) * 0.40f), kLine * 1.15f, withA(lineColor, 95));
	}

	// entry ball (where you come from)
	float ex = SX(pts[0].x), ey = SY(pts[0].y), d = halfH * 0.14f;
	const float diam[4][2] = { { ex, ey - d * 1.5f }, { ex + d * 1.5f * kAspect, ey }, { ex, ey + d * 1.5f }, { ex - d * 1.5f * kAspect, ey } };
	dc.quad(diam, lineColor);

	// end decoration: a clean arrowhead triangle at the exit (or a finish bar)
	size_t n = pts.size();
	if (n >= 2)
	{
		float ax = SX(pts[n - 1].x), ay = SY(pts[n - 1].y);
		float bx = SX(pts[n - 2].x), by = SY(pts[n - 2].y);
		float dx = ax - bx, dy = ay - by, len = std::sqrt(dx * dx + dy * dy);
		if (len > 1e-5f)
		{
			dx /= len; dy /= len;
			float nx = -dy, ny = dx;
			if (code == "finish")
			{
				float bw = halfW * 0.5f, th = kLine * 2.2f;
				dc.rect(ax - bw, ay - th, 2.0f * bw, 2.0f * th, lineColor);
			}
			else
			{
				float s = halfH * 0.36f;
				const float tri[4][2] = {
					{ ax + dx * s,        ay + dy * s },
					{ ax + nx * s * 0.6f, ay + ny * s * 0.6f },
					{ ax - nx * s * 0.6f, ay - ny * s * 0.6f },
					{ ax - nx * s * 0.6f, ay - ny * s * 0.6f },
				};
				dc.quad(tri, lineColor);
			}
		}
	}
}

geo::Vec2 worldAtDist(const std::vector<model::RoutePoint>& route, float dval)
{
	if (route.empty()) return {};
	if (dval <= route.front().distM) return { route.front().x, route.front().z };
	for (size_t i = 1; i < route.size(); ++i)
		if (route[i].distM >= dval)
		{
			float seg = std::max(1e-3f, route[i].distM - route[i - 1].distM);
			float t = (dval - route[i - 1].distM) / seg;
			return { route[i - 1].x + (route[i].x - route[i - 1].x) * t, route[i - 1].z + (route[i].z - route[i - 1].z) * t };
		}
	return { route.back().x, route.back().z };
}

int spriteForCode(const std::string& code)
{
	for (int i = 0; i < model::picto::kCount; ++i)
		if (code == model::picto::kAll[i]) return i + 1;
	return 0;
}

// A roadbook sign: a registered sprite (tinted by `color`) if one exists for
// the code, else a procedural fallback drawing.
void drawPictogram(DrawCache& dc, const std::string& code, float cx, float cy, float s, unsigned long color)
{
	int sp = spriteForCode(code);
	if (sp > 0)
	{
		float hw = s * kAspect, hh = s;
		dc.sprite(cx - hw, cy - hh, 2 * hw, 2 * hh, color, sp);
		return;
	}
	const float lw = kLine * 1.4f;
	auto L = [&](float x1, float y1, float x2, float y2) { dc.line(cx + x1 * s * kAspect, cy + y1 * s, cx + x2 * s * kAspect, cy + y2 * s, lw, color); };
	auto dot = [&](float x, float y, float r) { dc.rect(cx + (x - r) * s * kAspect, cy + (y - r) * s, 2 * r * s * kAspect, 2 * r * s, color); };

	if (code == "caution") { L(0,-1,0.95f,0.85f); L(0,-1,-0.95f,0.85f); L(-0.95f,0.85f,0.95f,0.85f); L(0,-0.25f,0,0.35f); dot(0,0.62f,0.12f); }
	else if (code == "crest") { L(-1,0.6f,-0.5f,-0.55f); L(-0.5f,-0.55f,0.5f,-0.55f); L(0.5f,-0.55f,1,0.6f); }
	else if (code == "dip")   { L(-1,-0.6f,-0.5f,0.55f); L(-0.5f,0.55f,0.5f,0.55f); L(0.5f,0.55f,1,-0.6f); }
	else if (code == "bump")  { L(-1,0.4f,-0.45f,0.4f); L(-0.45f,0.4f,0,-0.5f); L(0,-0.5f,0.45f,0.4f); L(0.45f,0.4f,1,0.4f); }
	else if (code == "hole")  { for (int i = 0; i < 8; ++i) { float a0 = i / 8.0f * 6.2832f, a1 = (i + 1) / 8.0f * 6.2832f; L(0.7f*std::cos(a0),0.7f*std::sin(a0),0.7f*std::cos(a1),0.7f*std::sin(a1)); } }
	else if (code == "rocks") { L(-1,0.7f,-0.4f,-0.4f); L(-0.4f,-0.4f,0.1f,0.7f); L(0.1f,0.7f,0.55f,-0.1f); L(0.55f,-0.1f,1,0.7f); }
	else if (code == "ruts")  { L(-0.6f,-0.8f,-0.4f,-0.3f); L(-0.4f,-0.3f,-0.6f,0.2f); L(-0.6f,0.2f,-0.4f,0.7f); L(0.4f,-0.8f,0.6f,-0.3f); L(0.6f,-0.3f,0.4f,0.2f); L(0.4f,0.2f,0.6f,0.7f); }
	else if (code == "water") { for (int k = -1; k <= 1; k += 2) { float y = k*0.35f; L(-1,y,-0.5f,y-0.25f); L(-0.5f,y-0.25f,0,y); L(0,y,0.5f,y-0.25f); L(0.5f,y-0.25f,1,y); } }
	else if (code == "jump")  { L(-1,0.7f,-0.1f,-0.5f); L(-0.1f,-0.5f,0.2f,-0.6f); L(0.5f,-0.2f,1,0.6f); L(1,0.6f,0.72f,0.55f); L(1,0.6f,0.95f,0.2f); }
	else if (code == "narrow"){ L(-1,-0.8f,-0.25f,0); L(-0.25f,0,-1,0.8f); L(1,-0.8f,0.25f,0); L(0.25f,0,1,0.8f); }
	else if (code == "keepL") { L(0.8f,0,-0.8f,0); L(-0.8f,0,-0.2f,-0.5f); L(-0.8f,0,-0.2f,0.5f); }
	else if (code == "keepR") { L(-0.8f,0,0.8f,0); L(0.8f,0,0.2f,-0.5f); L(0.8f,0,0.2f,0.5f); }
	else if (code == "fork")  { L(0,0.8f,0,-0.05f); L(0,-0.05f,-0.7f,-0.8f); L(0,-0.05f,0.7f,-0.8f); }
	else if (code == "gate")  { L(-0.65f,-0.8f,-0.65f,0.8f); L(0.65f,-0.8f,0.65f,0.8f); L(-0.65f,-0.45f,0.65f,-0.45f); L(-0.65f,0.1f,0.65f,0.1f); }
	else { dc.rect(cx - 0.5f*s*kAspect, cy - 0.5f*s, s*kAspect, s, color); }
}

// Danger level rendered as the real roadbook danger sign (! / !! / !!!),
// centred at (cx,cy) with half-height halfH, tinted by `color`.
void drawDanger(DrawCache& dc, int font, int level, float cx, float cy, float halfH, unsigned long color)
{
	if (level <= 0) return;
	const char* code = (level >= 3) ? "danger3" : (level == 2) ? "danger2" : "caution";
	int sp = spriteForCode(code);
	if (sp > 0) { dc.sprite(cx - halfH * kAspect, cy - halfH, 2 * halfH * kAspect, 2 * halfH, color, sp); return; }
	char m[4] = { 0 };
	for (int i = 0; i < std::min(3, level); ++i) m[i] = '!';
	dc.text(m, cx, cy, halfH * 1.2f, 1, color, font);
}

void drawPictoRow(DrawCache& dc, const model::Box& b, float x0, float y, float zoneW, float halfS, int maxN, unsigned long color)
{
	int np = std::min(maxN, static_cast<int>(b.pictograms.size()));
	if (np <= 0) return;
	float gap = zoneW / maxN;
	for (int i = 0; i < np; ++i)
		drawPictogram(dc, b.pictograms[i], x0 + gap * (i + 0.5f), y, halfS, color);
}

void fmtComma(char* buf, size_t n, double v) { std::snprintf(buf, n, "%.2f", v); for (char* p = buf; *p; ++p) if (*p == '.') *p = ','; }

const char* turnAbbrev(const model::Box& b)
{
	switch (b.type)
	{
	case model::BoxType::Start:    return g_french ? "DEP" : "S";
	case model::BoxType::Finish:   return g_french ? "ARR" : "F";
	case model::BoxType::Straight: return g_french ? "TD" : "kpS";
	default: return (b.turnDir == model::TurnDir::Right) ? (g_french ? "D" : "R") : (g_french ? "G" : "L");
	}
}

void drawCell(DrawCache& dc, int font, const model::Box& b, int idx, float x, float cy, bool act, float w)
{
	const int bold = font + 1;
	char buf[64];
	const float leftW = w * 0.30f, centerW = w * 0.42f, rightW = w - leftW - centerW;
	const float zx1 = x + leftW, zx2 = x + leftW + centerW;

	dc.rect(x, cy, w, kCellH, boxBg(act));
	box4(dc, x, cy, w, kCellH, act ? g_style.borderA : g_style.borderI, act ? kLine * 1.8f : kLine);
	dc.line(zx1, cy, zx1, cy + kCellH, kLine, g_style.inkDim);
	dc.line(zx2, cy, zx2, cy + kCellH, kLine, g_style.inkDim);
	if (act)
	{
		float p = 0.5f + 0.5f * std::sin(g_time * 3.2f);
		dc.rect(x + 0.0015f, cy + 0.003f, 0.0035f, kCellH - 0.006f, withA(g_style.borderA, 90 + static_cast<int>(130 * p)));
	}

	// left column: big bold total, partial box, box-number square
	fmtComma(buf, sizeof buf, b.distTotalKm);
	dc.text(buf, x + 0.008f, cy + 0.032f, 0.021f, 0, g_style.ink, bold);
	float pbY = cy + kCellH - 0.036f;
	dc.rect(x + 0.006f, pbY, leftW * 0.62f, 0.026f, withA(g_style.boxBgA, g_opacity));
	box4(dc, x + 0.006f, pbY, leftW * 0.62f, 0.026f, g_style.inkDim);
	if (b.type == model::BoxType::Start)       std::snprintf(buf, sizeof buf, "0,00");
	else if (b.type == model::BoxType::Finish) std::snprintf(buf, sizeof buf, "END");
	else                                       fmtComma(buf, sizeof buf, b.distPartialM / 1000.0);
	dc.text(buf, x + 0.011f, pbY + 0.019f, 0.013f, 0, g_style.ink, font);
	float nsX = zx1 - 0.022f, nsY = cy + kCellH - 0.026f;
	dc.rect(nsX, nsY, 0.018f, 0.020f, withA(g_style.boxBgA, g_opacity));
	box4(dc, nsX, nsY, 0.018f, 0.020f, g_style.inkDim);
	std::snprintf(buf, sizeof buf, "%d", idx);
	dc.text(buf, nsX + 0.009f, nsY + 0.015f, 0.012f, 1, g_style.ink, font);

	// centre column: tulip + pictograms (tulip hidden in distance-only training)
	if (!g_distOnly)
		drawTulip(dc, b, zx1 + centerW * 0.5f, cy + kCellH * 0.45f, kCellH * 0.32f, g_style.line, g_style.ball);
	drawPictoRow(dc, b, zx1 + 0.004f, cy + kCellH - 0.024f, centerW - 0.008f, 0.017f, 3, g_style.ink);

	// right column: turn abbreviation (big) + CAP + danger + note
	float rcx = zx2 + rightW * 0.5f;
	dc.text(turnAbbrev(b), rcx, cy + 0.038f, 0.026f, 1, g_style.ink, bold);
	if (!g_noCap) // CAP hidden in no-CAP training
	{
		std::snprintf(buf, sizeof buf, "%03d", b.capDeg);
		dc.text(buf, rcx, cy + 0.060f, 0.012f, 1, g_style.inkDim, font);
	}
	drawDanger(dc, font, b.dangerLevel, rcx, cy + kCellH - 0.024f, 0.019f, g_style.danger);
	if (!b.note.empty()) { std::snprintf(buf, sizeof buf, "%.16s", b.note.c_str()); dc.text(buf, zx2 + 0.005f, cy + 0.082f, 0.0095f, 0, g_style.ink, font); }
}

void drawStrip(DrawCache& dc, int font, const model::Roadbook& rb, double tripM, int active)
{
	const int n = static_cast<int>(rb.boxes.size());
	const float w = kRight - kLeft;
	char buf[64];

	// smooth scroll toward keeping `active` in the second visible slot
	float target = static_cast<float>(std::min(std::max(0, active - 1), std::max(0, n - kVisible)));
	g_scroll += (target - g_scroll) * clamp01(g_dt * 9.0f);
	if (std::fabs(target - g_scroll) < 0.002f) g_scroll = target;

	int lo = std::max(0, static_cast<int>(std::floor(g_scroll)) - 1);
	int hi = std::min(n - 1, static_cast<int>(std::floor(g_scroll)) + kVisible + 1);
	for (int bi = lo; bi <= hi; ++bi)
	{
		float cy = kStripTop + (bi - g_scroll) * (kCellH + kGap);
		if (cy < kStripTop - kCellH || cy > 0.965f) continue;
		if (g_maskBox && bi != active) continue;
		drawCell(dc, font, rb.boxes[bi], bi, kLeft, cy, bi == active, w);
	}

	// trip readout (drawn over the strip top to mask sliding boxes)
	dc.rect(kLeft, 0.030f, w, 0.066f, tripBg());
	box4(dc, kLeft, 0.030f, w, 0.066f, g_style.hudDim);
	dc.text("TRIP", kLeft + 0.008f, 0.047f, 0.013f, 0, g_style.hudDim, font);
	std::snprintf(buf, sizeof buf, "%.2f", tripM / 1000.0);
	dc.text(buf, kLeft + 0.008f, 0.076f, 0.026f, 0, g_style.tripValue, font);
	dc.text("km", kLeft + 0.058f, 0.076f, 0.013f, 0, g_style.hudDim, font);
	double toNext = (active + 1 < n) ? rb.boxes[active + 1].distTotalKm * 1000.0 - tripM : 0.0;
	std::snprintf(buf, sizeof buf, "next %dm", std::max(0, static_cast<int>(toNext)));
	dc.text(buf, kRight - 0.008f, 0.092f, 0.013f, 2, g_style.hudInk, font);

	// overall progress bar
	float prog = clamp01(static_cast<float>(tripM / (rb.totalDistanceKm * 1000.0 + 1.0)));
	dc.rect(kLeft, 0.099f, w, 0.006f, hudBg());
	dc.rect(kLeft, 0.099f, w * prog, 0.006f, g_style.tripValue);
}

// The whole roadbook presented as an ERTF/F2R-style rally computer: a baked
// carbon device sprite (with a transparent screen window) drawn ON TOP of the
// scrolling cases, plus live amber-LCD trip/heading readouts. Fractions match
// the texture layout in tools/iconpack/make_device.py.
static unsigned long long g_deviceBootMs = 0;

void drawDevice(DrawCache& dc, int font, const model::Roadbook& rb, const NavState& nav, int active)
{
	const int n = static_cast<int>(rb.boxes.size());
	const int bold = font + 1;
	static const unsigned long modelLcd[23] = { abgr(255,176,40), abgr(95,195,255), abgr(120,230,95), abgr(255,84,74),
	                                           abgr(255,176,40), abgr(110,200,255), abgr(130,235,110), abgr(255,240,235),
	                                           abgr(180,220,255), abgr(130,235,110), abgr(255,90,80), abgr(95,200,255),
	                                           abgr(255,186,60), abgr(240,242,245), abgr(255,200,90), abgr(140,235,110),
	                                           abgr(200,225,255), abgr(255,176,60), abgr(160,235,70), abgr(95,200,255),
	                                           abgr(255,176,40), abgr(255,186,60), abgr(255,120,90) };
	static const unsigned long ovrLcd[6]   = { 0, abgr(255,176,40), abgr(120,230,95), abgr(255,84,74), abgr(95,195,255), abgr(235,235,235) };
	const int dm = std::max(0, std::min(22, nav.deviceModel));
	const int spDevice = model::picto::kCount + 2 + dm;
	const float DX = 0.812f, DY = 0.022f, DW = 0.182f, DH = 0.950f;
	auto FX = [&](float f) { return DX + f * DW; };
	auto FY = [&](float f) { return DY + f * DH; };

	const float winX = FX(0.0625f), winY = FY(0.178f);
	const float winW = 0.875f * DW, winH = 0.724f * DH;
	const float caseX = FX(0.066f), caseW = 0.868f * DW;

	// 1) screen background behind the cases
	dc.rect(winX, winY, winW, winH, abgr(20, 21, 25, std::min(255, g_opacity + 50)));

	// 2) cases (smooth scroll; overflow is masked by the body drawn on top)
	float target = static_cast<float>(std::min(std::max(0, active - 1), std::max(0, n - 4)));
	g_scroll += (target - g_scroll) * clamp01(g_dt * 9.0f);
	if (std::fabs(target - g_scroll) < 0.002f) g_scroll = target;
	const float caseTop = winY + 0.006f;
	int lo = std::max(0, static_cast<int>(std::floor(g_scroll)) - 1);
	int hi = std::min(n - 1, static_cast<int>(std::floor(g_scroll)) + 5);
	for (int bi = lo; bi <= hi; ++bi)
	{
		float cy = caseTop + (bi - g_scroll) * (kCellH + kGap);
		if (cy < winY - kCellH - 0.01f || cy > winY + winH + 0.01f) continue;
		if (g_maskBox && bi != active) continue;
		drawCell(dc, font, rb.boxes[bi], bi, caseX, cy, bi == active, caseW);
	}

	// 3) carbon device chrome on top (full-colour sprite, white tint)
	dc.sprite(DX, DY, DW, DH, 0xFFFFFFFFu, spDevice);
	// subtle glass sheen across the top of the screen
	dc.rect(winX, winY, winW, winH * 0.10f, abgr(255, 255, 255, std::min(22, g_opacity / 8)));

	// 4) live LCD readouts (per-model colour, or override)
	const unsigned long lcd = (nav.lcdColor > 0 && nav.lcdColor < 6) ? ovrLcd[nav.lcdColor] : modelLcd[dm];
	unsigned long long bootEl = (g_deviceBootMs ? GetTickCount64() - g_deviceBootMs : 999999ULL);
	if (bootEl < 1700) // power-on self-test animation
	{
		dc.rect(winX, winY, winW, winH, abgr(8, 9, 11, std::min(255, g_opacity + 70))); // blank the screen
		int flick = static_cast<int>((bootEl / 80) % 2);
		unsigned long bl = withA(lcd, flick ? 235 : 110);
		dc.text("88.88", FX(0.455f), FY(0.094f), 0.024f, 2, bl, bold);
		dc.text("888",   FX(0.94f),  FY(0.094f), 0.024f, 2, bl, bold);
		dc.text("888",   FX(0.17f),  FY(0.150f), 0.013f, 0, bl, font);
		dc.text("888",   FX(0.475f), FY(0.150f), 0.013f, 0, bl, font);
		dc.text("888",   FX(0.78f),  FY(0.150f), 0.013f, 0, bl, font);
		float sy = winY + (bootEl / 1700.0f) * winH;       // sweep bar travelling down the screen
		dc.rect(winX, sy, winW, 0.006f, withA(lcd, 120));
		dc.text("RALLY COMPUTER", FX(0.5f), winY + winH * 0.45f, 0.015f, 1, withA(lcd, 205), font);
		return;
	}
	char buf[48];
	std::snprintf(buf, sizeof buf, "%.2f", nav.tripM / 1000.0);
	dc.text(buf, FX(0.455f), FY(0.094f), 0.024f, 2, lcd, bold);                 // TOTAL (big, right-aligned)
	std::snprintf(buf, sizeof buf, "%d", std::max(0, static_cast<int>(std::lround(nav.speedKmh))));
	dc.text(buf, FX(0.94f), FY(0.094f), 0.024f, 2, lcd, bold);                  // SPEED (big, right-aligned)
	double toNext = (active + 1 < n) ? rb.boxes[active + 1].distTotalKm * 1000.0 - nav.tripM : 0.0;
	std::snprintf(buf, sizeof buf, "%d", std::max(0, static_cast<int>(toNext)));
	dc.text(buf, FX(0.17f), FY(0.150f), 0.013f, 0, lcd, font);                  // NEXT (small)
	int hdg = (static_cast<int>(std::lround(nav.curHeading)) % 360 + 360) % 360;
	std::snprintf(buf, sizeof buf, "%03d", hdg);
	dc.text(buf, FX(0.475f), FY(0.150f), 0.013f, 0, lcd, font);                 // HDG (small)
	if (active + 1 < n) { std::snprintf(buf, sizeof buf, "%03d", rb.boxes[active + 1].capDeg); dc.text(buf, FX(0.78f), FY(0.150f), 0.013f, 0, lcd, font); }
	if (nav.recording)
	{
		float p = 0.5f + 0.5f * std::sin(g_time * 4.0f);
		dc.rect(FX(0.92f), FY(0.013f), 0.006f, 0.009f, withA(abgr(230, 40, 40), 110 + static_cast<int>(140 * p)));
	}
}

void drawNext(DrawCache& dc, int font, const model::Roadbook& rb, const NavState& nav, int active)
{
	int ni = active + 1;
	if (ni >= static_cast<int>(rb.boxes.size())) return;
	const model::Box& b = rb.boxes[ni];
	const int bold = font + 1;
	const float w = 0.20f, h = 0.180f, x = 0.5f - w * 0.5f, y = 0.800f;
	const float lw = w * 0.30f, cw = w * 0.42f, rw = w - lw - cw;
	const float zx1 = x + lw, zx2 = x + lw + cw;
	char buf[48];
	double toNext = b.distTotalKm * 1000.0 - nav.tripM;

	dc.rect(x, y, w, h, boxBg(true));
	box4(dc, x, y, w, h, (b.dangerLevel > 0) ? g_style.danger : g_style.borderA, kLine * 1.6f);
	dc.line(zx1, y, zx1, y + h, kLine, g_style.inkDim);
	dc.line(zx2, y, zx2, y + h, kLine, g_style.inkDim);

	// left: NEXT + big countdown
	dc.text("NEXT", x + 0.008f, y + 0.022f, 0.013f, 0, g_style.inkDim, font);
	std::snprintf(buf, sizeof buf, "%dm", std::max(0, static_cast<int>(toNext)));
	dc.text(buf, x + 0.008f, y + 0.052f, 0.024f, 0, g_style.ink, bold);

	// centre: tulip + signs
	drawTulip(dc, b, zx1 + cw * 0.5f, y + h * 0.45f, h * 0.30f, g_style.line, g_style.ball);
	drawPictoRow(dc, b, zx1 + 0.004f, y + h - 0.028f, cw - 0.008f, 0.020f, 4, g_style.ink);

	// right: abbreviation + CAP
	float rcx = zx2 + rw * 0.5f;
	dc.text(turnAbbrev(b), rcx, y + 0.044f, 0.030f, 1, g_style.ink, bold);
	std::snprintf(buf, sizeof buf, "%03d", b.capDeg);
	dc.text(buf, rcx, y + 0.068f, 0.013f, 1, g_style.inkDim, font);
	drawDanger(dc, font, b.dangerLevel, rcx, y + h - 0.030f, 0.020f, g_style.danger);

	// approach bar (cyan) fills as you close in
	float partial = std::max(1.0f, static_cast<float>(b.distPartialM));
	float frac = clamp01(1.0f - static_cast<float>(toNext) / partial);
	dc.rect(x, y + h - 0.006f, w, 0.006f, hudBg());
	dc.rect(x, y + h - 0.006f, w * frac, 0.006f, g_style.routeHi);
}

void drawCompass(DrawCache& dc, int font, const model::Roadbook& rb, const NavState& nav, int active)
{
	const float L = 0.32f, R = 0.68f, T = 0.016f, H = 0.030f;
	const float W = R - L, C = (L + R) * 0.5f, ppd = W / 120.0f;
	dc.rect(L, T, W, H, hudBg());
	float cur = g_dHead;

	int base = static_cast<int>(std::floor((cur - 60.0f) / 15.0f)) * 15;
	for (int bear = base; bear <= static_cast<int>(cur + 60.0f); bear += 15)
	{
		float rel = wrap180(static_cast<float>(bear) - cur);
		if (std::fabs(rel) > 61.0f) continue;
		float xx = C + rel * ppd;
		dc.line(xx, T + H * 0.62f, xx, T + H, kLine, g_style.hudDim);
		int dd = ((bear % 360) + 360) % 360;
		if (dd % 30 == 0)
		{
			char lab[8];
			if (dd == 0) std::snprintf(lab, sizeof lab, "N");
			else if (dd == 90)  std::snprintf(lab, sizeof lab, "E");
			else if (dd == 180) std::snprintf(lab, sizeof lab, "S");
			else if (dd == 270) std::snprintf(lab, sizeof lab, "W");
			else std::snprintf(lab, sizeof lab, "%d", dd);
			dc.text(lab, xx, T + 0.004f, 0.012f, 1, g_style.hudInk, font);
		}
	}

	dc.line(C, T, C, T + H, kLine * 1.8f, g_style.hudAccent);
	float rel = wrap180(g_dTarget - cur);
	if (nav.matched)
	{
		if (std::fabs(rel) <= 60.0f) dc.line(C + rel * ppd, T, C + rel * ppd, T + H, kLine * 1.8f, g_style.hudGood);
		else dc.text(rel > 0 ? ">>" : "<<", rel > 0 ? R - 0.012f : L + 0.012f, T + H * 0.35f, 0.018f, 1, g_style.hudGood, font);
	}

	char buf[64];
	float turn = wrap180(nav.targetHeading - nav.curHeading);
	if (nav.matched && std::fabs(turn) < 5.0f) std::snprintf(buf, sizeof buf, "HDG %03d   ON LINE", static_cast<int>(wrap360(nav.curHeading)));
	else if (nav.matched) std::snprintf(buf, sizeof buf, "HDG %03d   turn %s %d", static_cast<int>(wrap360(nav.curHeading)), turn > 0 ? "R" : "L", static_cast<int>(std::fabs(turn)));
	else std::snprintf(buf, sizeof buf, "HDG %03d", static_cast<int>(wrap360(nav.curHeading)));
	dc.text(buf, C, T + H + 0.018f, 0.016f, 1, (nav.matched && std::fabs(turn) < 5.0f) ? g_style.hudGood : g_style.hudInk, font);

	if (active + 1 < static_cast<int>(rb.boxes.size()))
	{
		std::snprintf(buf, sizeof buf, "next CAP %03d", rb.boxes[active + 1].capDeg);
		dc.text(buf, C, T + H + 0.040f, 0.014f, 1, g_style.hudDim, font);
	}
}

void drawOffRoute(DrawCache& dc, int font, const NavState& nav)
{
	if (!nav.matched || nav.offRouteM < nav.offRouteTolM) return;
	float p = 0.5f + 0.5f * std::sin(g_time * 6.5f); // flash
	const float w = 0.22f, h = 0.085f, x = 0.5f - w * 0.5f, y = 0.40f;
	dc.rect(x, y, w, h, abgr(60, 0, 0, 150 + static_cast<int>(90 * p)));
	box4(dc, x, y, w, h, withA(g_style.hudWarn, 150 + static_cast<int>(105 * p)), kLine * 1.5f);
	char buf[48];
	std::snprintf(buf, sizeof buf, "OFF ROUTE  %dm", static_cast<int>(nav.offRouteM));
	dc.text(buf, x + w * 0.5f, y + 0.026f, 0.020f, 1, g_style.hudWarn, font);
	float rel = wrap180(nav.bearingToRoute - nav.curHeading);
	drawArrow(dc, x + w * 0.5f, y + h - 0.026f, 0.026f, rel, g_style.hudWarn);
}

void drawMinimap(DrawCache& dc, int font, const model::Roadbook& rb, const NavState& nav, int active)
{
	if (rb.route.size() < 2) return;
	const float X = 0.018f, Y = 0.655f, W = 0.205f, H = 0.320f, pad = 0.014f;
	dc.rect(X, Y, W, H, hudBg());
	box4(dc, X, Y, W, H, g_style.hudDim);
	dc.text("MAP", X + 0.008f, Y + 0.020f, 0.013f, 0, g_style.hudDim, font);

	float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f;
	for (const auto& r : rb.route) { minX = std::min(minX, r.x); maxX = std::max(maxX, r.x); minZ = std::min(minZ, r.z); maxZ = std::max(maxZ, r.z); }
	float spanX = std::max(1.0f, maxX - minX), spanZ = std::max(1.0f, maxZ - minZ);
	float sy = std::min((H - 2 * pad) / spanZ, (W - 2 * pad) / (spanX * kAspect));
	float sx = sy * kAspect;
	float cxw = (minX + maxX) * 0.5f, czw = (minZ + maxZ) * 0.5f;
	float bcx = X + W * 0.5f, bcy = Y + H * 0.5f;
	auto SX = [&](float wx) { return bcx + (wx - cxw) * sx; };
	auto SY = [&](float wz) { return bcy - (wz - czw) * sy; };

	int step = std::max(1, static_cast<int>(rb.route.size() / 160));
	bool have = false; float px = 0, py = 0;
	for (size_t i = 0; i < rb.route.size(); i += step)
	{
		float nx = SX(rb.route[i].x), ny = SY(rb.route[i].z);
		if (have) dc.line(px, py, nx, ny, kLine, g_style.hudDim);
		px = nx; py = ny; have = true;
	}
	for (const auto& b : rb.boxes)
	{
		if (b.type == model::BoxType::Start || b.type == model::BoxType::Finish) continue;
		geo::Vec2 wp = worldAtDist(rb.route, static_cast<float>(b.distTotalKm * 1000.0));
		float dw = 0.005f;
		dc.rect(SX(wp.x) - dw * kAspect, SY(wp.y) - dw, 2 * dw * kAspect, 2 * dw, b.dangerLevel > 0 ? g_style.hudWarn : g_style.hudAccent);
	}
	if (active >= 0 && active < static_cast<int>(rb.boxes.size()))
	{
		geo::Vec2 wp = worldAtDist(rb.route, static_cast<float>(rb.boxes[active].distTotalKm * 1000.0));
		float dw = 0.007f;
		dc.rect(SX(wp.x) - dw * kAspect, SY(wp.y) - dw, 2 * dw * kAspect, 2 * dw, g_style.hudGood);
	}
	(void)nav; // heading taken from the smoothed g_dHead (north-up map, 0 deg = +z = N)
	drawArrow(dc, SX(g_dPos.x), SY(g_dPos.y), 0.011f, g_dHead, g_style.hudInk);
}

} // namespace

// Defined in the render namespace (not the anonymous one) so the plugin can link
// to it; reads the boot timestamp held in the anonymous namespace above.
void triggerDeviceBoot() { g_deviceBootMs = GetTickCount64(); }

// Rally trip computer: twin amber-LCD readouts (TOTAL / NEXT) + heading + CAP,
// styled after an ERTF/F2R rally dash. Top-left module.
void drawDash(DrawCache& dc, int font, const model::Roadbook& rb, const NavState& nav, int active)
{
	const int bold = font + 1;
	const float x = 0.015f, y = 0.015f, w = 0.224f, h = 0.132f;
	const unsigned long bg    = abgr(10, 11, 13, std::min(255, g_opacity + 40));
	const unsigned long lcd   = abgr(255, 174, 32);  // amber LCD
	const unsigned long lcdD  = abgr(120, 84, 22);   // dim amber
	const unsigned long frame = abgr(64, 66, 72);
	char buf[48];

	dc.rect(x, y, w, h, bg);
	box4(dc, x, y, w, h, frame, kLine * 1.5f);
	float midx = x + w * 0.52f;
	dc.line(midx, y + 0.028f, midx, y + h - 0.030f, kLine, frame);

	dc.text("RALLY TRIP", x + 0.010f, y + 0.020f, 0.012f, 0, abgr(150, 150, 154), font);
	if (nav.recording)
	{
		float p = 0.5f + 0.5f * std::sin(g_time * 4.0f);
		dc.rect(x + w - 0.050f, y + 0.011f, 0.008f, 0.009f, withA(abgr(230, 40, 40), 110 + static_cast<int>(140 * p)));
		dc.text("REC", x + w - 0.039f, y + 0.020f, 0.011f, 0, abgr(235, 95, 95), font);
	}

	// progress: current box index + a thin bar showing distance through the stage
	int nbx = static_cast<int>(rb.boxes.size());
	std::snprintf(buf, sizeof buf, "%d/%d", std::min(active + 1, nbx), nbx);
	dc.text(buf, x + 0.088f, y + 0.020f, 0.011f, 0, abgr(150, 150, 154), font);
	{
		float prog = (rb.totalDistanceKm > 0.01) ? clamp01(static_cast<float>(nav.tripM / (rb.totalDistanceKm * 1000.0))) : 0.0f;
		dc.rect(x + 0.010f, y + h - 0.0045f, w - 0.020f, 0.0024f, abgr(45, 47, 53));
		dc.rect(x + 0.010f, y + h - 0.0045f, (w - 0.020f) * prog, 0.0024f, lcd);
	}

	// TOTAL (left LCD)
	dc.text("TOTAL km", x + 0.012f, y + 0.044f, 0.010f, 0, lcdD, font);
	std::snprintf(buf, sizeof buf, "%.2f", nav.tripM / 1000.0);
	dc.text(buf, x + 0.012f, y + 0.084f, 0.032f, 0, lcd, bold);

	// NEXT (right LCD): metres to the next box
	dc.text("NEXT m", midx + 0.012f, y + 0.044f, 0.010f, 0, lcdD, font);
	double toNext = (active + 1 < static_cast<int>(rb.boxes.size())) ? rb.boxes[active + 1].distTotalKm * 1000.0 - nav.tripM : 0.0;
	std::snprintf(buf, sizeof buf, "%d", std::max(0, static_cast<int>(toNext)));
	dc.text(buf, midx + 0.012f, y + 0.084f, 0.032f, 0, lcd, bold);

	// bottom: live heading + next CAP + menu hint
	int hdg = (static_cast<int>(std::lround(nav.curHeading)) % 360 + 360) % 360;
	std::snprintf(buf, sizeof buf, "HDG %03d", hdg);
	dc.text(buf, x + 0.012f, y + h - 0.013f, 0.014f, 0, lcd, font);
	if (active + 1 < static_cast<int>(rb.boxes.size()))
	{
		std::snprintf(buf, sizeof buf, "CAP %03d", rb.boxes[active + 1].capDeg);
		dc.text(buf, midx + 0.012f, y + h - 0.013f, 0.014f, 0, lcd, font);
	}
	dc.text("F4", x + w - 0.024f, y + h - 0.013f, 0.011f, 0, abgr(140, 140, 144), font);
}

// Elevation profile of the route with a 'you are here' marker + ascent/descent.
void drawProfile(DrawCache& dc, int font, const model::Roadbook& rb, const NavState& nav)
{
	const auto& route = rb.route;
	if (route.size() < 2) return;
	const float x = 0.018f, y = 0.82f, w = 0.20f, h = 0.072f;
	dc.rect(x, y, w, h, hudBg());
	box4(dc, x, y, w, h, g_style.hudDim);
	float lo = 1e9f, hi = -1e9f; const float tot = std::max(1.0f, route.back().distM);
	for (auto& r : route) { lo = std::min(lo, r.elev); hi = std::max(hi, r.elev); }
	const float span = std::max(1.0f, hi - lo);
	auto PX = [&](float d) { return x + 0.006f + (d / tot) * (w - 0.012f); };
	auto PY = [&](float e) { return y + h - 0.008f - ((e - lo) / span) * (h - 0.026f); };
	for (size_t i = 1; i < route.size(); ++i)
		dc.line(PX(route[i - 1].distM), PY(route[i - 1].elev), PX(route[i].distM), PY(route[i].elev), kLine, g_style.tripValue);
	float you = static_cast<float>(nav.tripM);
	dc.line(PX(you), y + 0.004f, PX(you), y + h - 0.004f, kLine, g_style.hudAccent);
	float asc = 0, desc = 0; for (size_t i = 1; i < route.size(); ++i) { float d = route[i].elev - route[i - 1].elev; if (d > 0) asc += d; else desc -= d; }
	char buf[48]; std::snprintf(buf, sizeof buf, "ELEV  +%d / -%d m", static_cast<int>(asc), static_cast<int>(desc));
	dc.text(buf, x + 0.006f, y + 0.014f, 0.010f, 0, g_style.hudDim, font);
}

// Co-driver pace note for the upcoming box, fading in as you approach it.
void drawDriver(DrawCache& dc, int font, const model::Roadbook& rb, const NavState& nav, int active)
{
	int ni = active + 1, n = static_cast<int>(rb.boxes.size());
	if (ni >= n) return;
	const model::Box& b = rb.boxes[ni];
	double toNext = b.distTotalKm * 1000.0 - nav.tripM;
	if (toNext <= 0.0 || toNext > 300.0) return;
	const int bold = font + 1;
	float fade = clamp01((300.0f - static_cast<float>(toNext)) / 130.0f);
	const float x = 0.5f, y = 0.13f, w = 0.26f, h = 0.072f;
	int al = std::min(g_opacity, static_cast<int>(g_opacity * (0.35f + 0.65f * fade)));
	dc.rect(x - w * 0.5f, y, w, h, abgr(12, 14, 18, std::min(235, al + 30)));
	box4(dc, x - w * 0.5f, y, w, h, (b.dangerLevel > 0) ? g_style.danger : g_style.borderA, kLine * 1.7f);
	const char* dir = (b.type == model::BoxType::Straight) ? "STRAIGHT" : (b.turnDir == model::TurnDir::Right) ? "RIGHT" : "LEFT";
	char buf[64];
	std::snprintf(buf, sizeof buf, "%dm   %s", std::max(0, static_cast<int>(toNext)), dir);
	dc.text(buf, x, y + 0.032f, 0.026f, 1, g_style.hudInk, bold);
	std::string sub;
	if (b.type == model::BoxType::Hairpin) sub = "HAIRPIN ";
	for (int i = 0; i < b.dangerLevel && i < 3; ++i) sub += "!";
	if (!b.pictograms.empty()) { if (!sub.empty()) sub += "  "; sub += b.pictograms[0]; }
	if (!sub.empty()) dc.text(sub.c_str(), x, y + 0.058f, 0.014f, 1, (b.dangerLevel > 0) ? g_style.danger : g_style.hudDim, font);
}

void RoadbookRenderer::draw(DrawCache& dc, int font, const model::Roadbook& rb, const NavState& nav)
{
	if (rb.boxes.empty())
	{
		// no roadbook for this track yet -> a slim, non-intrusive guidance banner
		g_opacity = std::max(0, std::min(255, nav.panelOpacity));
		g_style = getStyle(nav.styleIndex);
		const float bw = 0.42f, bx = 0.5f - bw * 0.5f, by = 0.035f, bh = 0.052f;
		dc.rect(bx, by, bw, bh, hudBg());
		box4(dc, bx, by, bw, bh, g_style.hudAccent);
		dc.text("NO ROADBOOK - ride the route, then F4 > Save roadbook now", 0.5f, by + 0.022f, 0.012f, 1, g_style.hudInk, font);
		dc.text("(F8 hides this overlay)", 0.5f, by + 0.041f, 0.0098f, 1, g_style.hudDim, font);
		return;
	}
	g_opacity = std::max(0, std::min(255, nav.panelOpacity));
	g_style = getStyle(nav.styleIndex);
	g_noCap = nav.noCap; g_distOnly = nav.distanceOnly; g_maskBox = nav.maskBox; g_french = nav.frenchNotation;

	unsigned long long now = GetTickCount64();
	g_dt = g_animInit ? std::min(0.1f, (now - g_lastMs) / 1000.0f) : 0.0f;
	g_time = static_cast<float>((now % 100000000ULL) / 1000.0);
	g_lastMs = now; g_animInit = true;

	g_dHead   = easeAngle(g_dHead, nav.curHeading, 12.0f);
	g_dTarget = easeAngle(g_dTarget, nav.targetHeading, 10.0f);
	if (!g_posInit) { g_dPos = nav.pos; g_posInit = true; }
	else { float k = clamp01(g_dt * 8.0f); g_dPos.x += (nav.pos.x - g_dPos.x) * k; g_dPos.y += (nav.pos.y - g_dPos.y) * k; }

	int active = activeBox(rb, nav);

	auto setMod = [&](const model::ModuleCfg& m, float px, float py) {
		g_opacity = std::max(0, std::min(255, m.opacity >= 0 ? m.opacity : nav.panelOpacity));
		dc.setTransform(nav.gScale * m.scale, nav.gOffX + m.x, nav.gOffY + m.y, px, py);
	};
	if (nav.deviceFrame)
	{
		// the rally computer replaces the bare strip + dash
		setMod(nav.mStrip, 0.903f, 0.497f); // pivot at device centre
		drawDevice(dc, font, rb, nav, active);
		dc.resetTransform();
	}
	else
	{
		if (nav.mDash.on)  { setMod(nav.mDash, 0.015f, 0.015f); drawDash(dc, font, rb, nav, active);          dc.resetTransform(); }
		if (nav.mStrip.on) { setMod(nav.mStrip, 0.988f, 0.030f); drawStrip(dc, font, rb, nav.tripM, active);  dc.resetTransform(); }
	}
	if (nav.mDriver.on)  { setMod(nav.mDriver, 0.5f, 0.13f);  drawDriver(dc, font, rb, nav, active);       dc.resetTransform(); }
	if (nav.mCompass.on) { setMod(nav.mCompass, 0.5f, 0.016f); drawCompass(dc, font, rb, nav, active);     dc.resetTransform(); }
	if (nav.mNext.on)    { setMod(nav.mNext, 0.5f, 0.980f);    drawNext(dc, font, rb, nav, active);        dc.resetTransform(); }
	if (nav.mOffroute.on && !nav.challenge) { setMod(nav.mOffroute, 0.5f, 0.40f); drawOffRoute(dc, font, nav); dc.resetTransform(); }
	if (nav.mMinimap.on  && !nav.challenge) { setMod(nav.mMinimap, 0.018f, 0.975f); drawMinimap(dc, font, rb, nav, active); dc.resetTransform(); }
	if (nav.mProfile.on) { setMod(nav.mProfile, 0.018f, 0.82f); drawProfile(dc, font, rb, nav); dc.resetTransform(); }
}

// Full-page roadbook review: a paged grid of every box, for study off-bike.
void RoadbookRenderer::drawPage(DrawCache& dc, int font, const model::Roadbook& rb, int page, int styleIndex)
{
	g_style = getStyle(styleIndex); g_opacity = 245; g_noCap = false; g_distOnly = false; g_maskBox = false;
	g_time = static_cast<float>((GetTickCount64() % 100000000ULL) / 1000.0);
	dc.rect(0.03f, 0.04f, 0.94f, 0.92f, abgr(10, 12, 16, 246));
	dc.rect(0.03f, 0.04f, 0.94f, 0.0018f, g_style.borderA);
	dc.rect(0.03f, 0.958f, 0.94f, 0.0018f, g_style.borderA);
	const int n = static_cast<int>(rb.boxes.size());
	const int cols = 3, rows = 5, per = cols * rows;
	const int pages = std::max(1, (n + per - 1) / per);
	page = std::max(0, std::min(page, pages - 1));
	char buf[96];
	std::snprintf(buf, sizeof buf, "ROADBOOK   %.20s   page %d/%d   F6/F7 page  -  F4 close",
	              rb.trackName.empty() ? "(track)" : rb.trackName.c_str(), page + 1, pages);
	dc.text(buf, 0.045f, 0.085f, 0.018f, 0, abgr(235, 235, 235), font);
	const float gx = 0.045f, gy = 0.10f, cw = 0.30f, gapx = 0.008f, gapy = 0.006f;
	for (int k = 0; k < per; ++k)
	{
		int bi = page * per + k; if (bi >= n) break;
		int c = k % cols, r = k / cols;
		drawCell(dc, font, rb.boxes[bi], bi, gx + c * (cw + gapx), gy + r * (kCellH + gapy), false, cw);
	}
}

} // namespace render
