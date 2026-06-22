// ============================================================================
// dllmain.cpp  —  exported PiBoSo plugin API (Stage 0 smoke test)
//
// This is the single try/catch boundary between the game and the plugin: no
// C++ exception may cross back into the host or it terminates the game. Each
// export wraps its body in GUARD_CATCH.
//
// Stage 0 goal: prove build + load + telemetry + draw + input before any
// roadbook logic. It draws a small panel showing live telemetry (so we can
// read off which world axis is "up", confirm the draw coordinate space, and
// verify the distance integrator), with F8 toggling the overlay.
// ============================================================================
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include "api/mxb_api.h"
#include "api/api_sizes.h"
#include "util/log.h"
#include "render/draw_cache.h"
#include "render/primitives.h"
#include "input/input_manager.h"
#include "record/ride_recorder.h"
#include "generate/tulip_generator.h"
#include "model/roadbook.h"
#include "model/roadbook_json.h"
#include "render/roadbook_renderer.h"
#include "render/menu.h"
#include "model/settings.h"

#include <string>
#include <vector>

using namespace render;

// Module handle, captured at load, for resolving the plugin's own folder.
static HMODULE g_hModule = nullptr;
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID) { if (reason == DLL_PROCESS_ATTACH) g_hModule = inst; return TRUE; }

// ---- exception barrier ----------------------------------------------------
// Usage:  void Export(...) { try { ...body... } GUARD_CATCH("Export") }
#define GUARD_CATCH(name)                                                      \
	catch (const std::exception& e) { rblog::line("EXC %s: %s", name, e.what()); } \
	catch (...)                     { rblog::line("EXC %s: unknown", name); }

// ---- API identity ---------------------------------------------------------
// Values current for the live MX Bikes build (match the actively-maintained
// reference plugins). Bump if a game update changes the data/interface version.
namespace {
constexpr int kTelemetryHz     = 100;
constexpr int kModDataVersion  = 8;
constexpr int kInterfaceVersion = 9;
constexpr float kMinMoveSpeed  = 0.1f;  // m/s; below this we count no distance
constexpr float kMaxDt         = 0.5f;  // s; reject gaps larger than this (pause/teleport)

// ---- plugin state ---------------------------------------------------------
struct App
{
	bool   overlayVisible = true;

	// latest telemetry snapshot
	long long ticks   = 0;
	bool   haveLastTime = false;
	float  lastTime  = 0.0f;
	float  pos = 0, x = 0, y = 0, z = 0, yaw = 0, speed = 0, vx = 0, vy = 0;
	double distM = 0.0;

	// event info
	char   trackId[100]   = "";
	char   trackName[100] = "";
	float  trackLength    = 0.0f;

	std::string          savePath;

	DrawCache            draw;
	input::InputManager  in;
	record::RideRecorder rec;
	model::Roadbook        roadbook;
	std::vector<geo::Vec2> routeVec;     // cached route (x,z) for map-matching
	int                    matchHint = -1;
	RoadbookRenderer       renderer;
	NavState               nav;
	model::Settings        settings;
	Menu                   menu;
	int                    menuPage = 0; // 0 main, 1 settings, 2 roadbooks, 3 stats, 4 modules, 5 edit
	int                    editBox = 0;
	std::string            statusMsg;

	// ride stats / challenge scoring
	float     rideTime = 0.0f;
	float     topSpeed = 0.0f;
	double    sumSpeed = 0.0;
	long long speedCount = 0;
	long long onRouteCount = 0;
	long long sampleCount = 0;
	int       offRouteEvents = 0;
	bool      wasOffRoute = false;

	static App& get() { static App s; return s; }
};

// Resolve the plugin's own folder and play a bundled WAV (async, best-effort).
std::string pluginDir()
{
	char buf[MAX_PATH] = "";
	GetModuleFileNameA(g_hModule, buf, MAX_PATH);
	std::string p = buf;
	size_t s = p.find_last_of("\\/");
	return (s == std::string::npos) ? "." : p.substr(0, s);
}
void playSound(const char* file)
{
	static const std::string dir = pluginDir();
	std::string path = dir + "\\Roadbook_data\\audio\\" + file;
	PlaySoundA(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

// --- spoken-sentence pace notes: concatenate clip WAVs into one in-memory WAV ---
static bool readWavPcm(const std::string& path, std::vector<unsigned char>& pcm, int& rate, int& bits, int& ch)
{
	FILE* f = std::fopen(path.c_str(), "rb");
	if (!f) return false;
	std::fseek(f, 0, SEEK_END); long sz = std::ftell(f); std::fseek(f, 0, SEEK_SET);
	if (sz < 44) { std::fclose(f); return false; }
	std::vector<unsigned char> buf(static_cast<size_t>(sz));
	size_t rd = std::fread(buf.data(), 1, static_cast<size_t>(sz), f); std::fclose(f);
	if (rd < 44 || std::memcmp(buf.data(), "RIFF", 4) || std::memcmp(buf.data() + 8, "WAVE", 4)) return false;
	size_t off = 12;
	while (off + 8 <= buf.size())
	{
		unsigned int csz = buf[off + 4] | (buf[off + 5] << 8) | (buf[off + 6] << 16) | (static_cast<unsigned>(buf[off + 7]) << 24);
		if (std::memcmp(&buf[off], "fmt ", 4) == 0 && off + 24 <= buf.size())
		{
			ch   = buf[off + 10] | (buf[off + 11] << 8);
			rate = buf[off + 12] | (buf[off + 13] << 8) | (buf[off + 14] << 16) | (static_cast<unsigned>(buf[off + 15]) << 24);
			bits = buf[off + 22] | (buf[off + 23] << 8);
		}
		else if (std::memcmp(&buf[off], "data", 4) == 0)
		{
			size_t n = (std::min)(static_cast<size_t>(csz), buf.size() - off - 8);
			pcm.assign(buf.begin() + off + 8, buf.begin() + off + 8 + n);
			return true;
		}
		off += 8 + csz + (csz & 1);
	}
	return false;
}

static std::vector<unsigned char> g_sentenceBuf; // kept alive while SND_ASYNC plays

void playSentence(const std::vector<std::string>& clips)
{
	static const std::string base = pluginDir() + "\\Roadbook_data\\audio\\voice\\";
	std::vector<unsigned char> pcm; int rate = 22050, bits = 16, ch = 1;
	for (const auto& c : clips) { std::vector<unsigned char> p; int r, b, n; if (readWavPcm(base + c + ".wav", p, r, b, n)) { rate = r; bits = b; ch = n; pcm.insert(pcm.end(), p.begin(), p.end()); } }
	if (pcm.empty()) return;
	g_sentenceBuf.clear(); g_sentenceBuf.reserve(pcm.size() + 44);
	auto p32 = [&](unsigned int v) { for (int i = 0; i < 4; ++i) g_sentenceBuf.push_back((v >> (8 * i)) & 0xff); };
	auto p16 = [&](unsigned short v) { g_sentenceBuf.push_back(v & 0xff); g_sentenceBuf.push_back((v >> 8) & 0xff); };
	auto ps  = [&](const char* s) { for (int i = 0; i < 4; ++i) g_sentenceBuf.push_back(static_cast<unsigned char>(s[i])); };
	unsigned int byteRate = static_cast<unsigned int>(rate) * ch * bits / 8; unsigned short blockAlign = static_cast<unsigned short>(ch * bits / 8);
	ps("RIFF"); p32(36 + static_cast<unsigned int>(pcm.size())); ps("WAVE"); ps("fmt "); p32(16); p16(1); p16(static_cast<unsigned short>(ch)); p32(static_cast<unsigned int>(rate)); p32(byteRate); p16(blockAlign); p16(static_cast<unsigned short>(bits)); ps("data"); p32(static_cast<unsigned int>(pcm.size()));
	g_sentenceBuf.insert(g_sentenceBuf.end(), pcm.begin(), pcm.end());
	PlaySoundA(reinterpret_cast<LPCSTR>(g_sentenceBuf.data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

static void numberClips(int v, std::vector<std::string>& out)
{
	static const char* ones[] = { "zero","one","two","three","four","five","six","seven","eight","nine","ten","eleven","twelve","thirteen","fourteen","fifteen","sixteen","seventeen","eighteen","nineteen" };
	static const char* tens[] = { "","","twenty","thirty","forty","fifty","sixty","seventy","eighty","ninety" };
	if (v >= 1000) { numberClips(v / 1000, out); out.push_back("thousand"); v %= 1000; if (v == 0) return; }
	if (v >= 100) { out.push_back(ones[v / 100]); out.push_back("hundred"); v %= 100; if (v == 0) return; }
	if (v >= 20) { out.push_back(tens[v / 10]); if (v % 10) out.push_back(ones[v % 10]); }
	else out.push_back(ones[v < 0 ? 0 : v]);
}

// map a pictogram code to a voice clip key (only those that have a spoken word)
static const char* signClip(const std::string& pg)
{
	if (pg == "bump") return "crest";  if (pg == "jump") return "jump";  if (pg == "dip") return "dip";   if (pg == "water") return "water";
	if (pg == "bumpy" || pg == "rough") return "bumpy"; if (pg == "ruts") return "ruts"; if (pg == "compression") return "compression";
	if (pg == "downhill") return "downhill";
	if (pg == "dune" || pg == "dunes" || pg == "sandpit") return "sand";
	if (pg == "hole") return "hole";   if (pg == "ditch") return "ditch"; if (pg == "rocks" || pg == "rocky") return "rocks";
	if (pg == "bridge") return "bridge"; if (pg == "gate") return "gate"; if (pg == "narrow") return "narrows";
	if (pg == "tree") return "tree";   if (pg == "village") return "village"; if (pg == "cliff") return "cliff"; if (pg == "fence") return "fence";
	return nullptr;
}

// Assemble a full pace-note clip sequence: "<danger> <direction> <distance> into <feature>".
std::vector<std::string> paceNoteClips(const model::Box& b, int distM)
{
	using model::BoxType; using model::TurnDir;
	std::vector<std::string> s;
	if (b.dangerLevel == 1) s.push_back("caution");
	else if (b.dangerLevel == 2) { s.push_back("danger"); s.push_back("two"); }
	else if (b.dangerLevel >= 3) { s.push_back("danger"); s.push_back("three"); }
	if (b.type == BoxType::Hairpin) { s.push_back("hairpin"); s.push_back(b.turnDir == TurnDir::Right ? "right" : "left"); }
	else if (b.type == BoxType::Turn) s.push_back(b.turnDir == TurnDir::Right ? "right" : "left");
	else if (b.type == BoxType::Finish) s.push_back("finish");
	else if (b.type == BoxType::Straight) s.push_back("keep_straight");
	if (distM > 0 && b.type != BoxType::Start && b.type != BoxType::Finish)
	{
		int d = ((distM + 5) / 10) * 10; if (d < 10) d = 10; // round to nearest 10 m
		numberClips(d, s);
	}
	for (const auto& pg : b.pictograms) { const char* f = signClip(pg); if (f) { s.push_back("into"); s.push_back(f); break; } }
	return s;
}

int  g_lastChimeBox = -1;        // audio throttle: last box we chimed for
bool g_offRouteAudioLatch = false;

void resetRide()
{
	App& a = App::get();
	a.distM = 0.0;
	a.haveLastTime = false;
	a.ticks = 0;
	a.rideTime = 0.0f; a.topSpeed = 0.0f; a.sumSpeed = 0.0; a.speedCount = 0;
	a.onRouteCount = 0; a.sampleCount = 0; a.offRouteEvents = 0; a.wasOffRoute = false;
}

std::string roadbookPath(const std::string& savePath, const std::string& trackId)
{
	std::string id = trackId;
	for (char& c : id)
		if (c == ' ' || c == '\\' || c == '/' || c == ':' || c == '*' ||
		    c == '?' || c == '"' || c == '<' || c == '>' || c == '|') c = '_';
	if (id.empty()) id = "unknown";
	std::string dir = savePath.empty() ? "." : savePath;
	if (dir.back() != '\\' && dir.back() != '/') dir += '\\';
	return dir + "Roadbook\\roadbook_" + id + ".json";
}

std::string historyPath(const std::string& savePath)
{
	std::string dir = savePath.empty() ? "." : savePath;
	if (dir.back() != '\\' && dir.back() != '/') dir += '\\';
	return dir + "Roadbook\\history.json";
}

std::string bestRunPath(const std::string& savePath, const std::string& trackId)
{
	std::string id = trackId;
	for (char& c : id)
		if (c == ' ' || c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') c = '_';
	if (id.empty()) id = "unknown";
	std::string dir = savePath.empty() ? "." : savePath;
	if (dir.back() != '\\' && dir.back() != '/') dir += '\\';
	return dir + "Roadbook\\best_" + id + ".json";
}

void rebuildRouteCache()
{
	App& a = App::get();
	a.routeVec.clear();
	a.routeVec.reserve(a.roadbook.route.size());
	for (const auto& r : a.roadbook.route) a.routeVec.push_back({ r.x, r.z });
	a.matchHint = -1;
}

void reloadRoadbook()
{
	App& a = App::get();
	std::string rbPath = roadbookPath(a.savePath, a.trackId);
	model::Roadbook loaded;
	if (model::readJson(rbPath, loaded) && !loaded.boxes.empty())
	{
		a.roadbook = loaded;
		rebuildRouteCache();
		render::triggerDeviceBoot();
		rblog::line("reload: %zu boxes from %s", a.roadbook.boxes.size(), rbPath.c_str());
	}
	else rblog::line("reload: failed %s", rbPath.c_str());
}

void saveCurrentBook()
{
	App& a = App::get();
	if (!a.roadbook.boxes.empty())
		model::writeJson(a.roadbook, roadbookPath(a.savePath, a.trackId));
}

void generateRoadbook()
{
	App& a = App::get();
	const auto& s = a.rec.samples();
	if (s.size() < 10) { rblog::line("generateRoadbook: too few samples (%zu)", s.size()); return; }
	a.roadbook = gen::generate(s, gen::Params(), a.trackId, a.trackName);
	rblog::line("generateRoadbook: %zu boxes, %.3f km from %zu samples",
	            a.roadbook.boxes.size(), a.roadbook.totalDistanceKm, s.size());
	for (const auto& b : a.roadbook.boxes)
		rblog::line("  box %d %-8s tot=%.3fkm part=%dm cap=%d %s dl=%d",
		            b.index, model::toString(b.type), b.distTotalKm, b.distPartialM, b.capDeg,
		            model::toString(b.turnDir), b.dangerLevel);

	a.roadbook.sourceRide = a.rec.path();
	std::string rbPath = roadbookPath(a.savePath, a.trackId);
	bool wrote = model::writeJson(a.roadbook, rbPath);
	rblog::line("generateRoadbook: %s %s", wrote ? "wrote" : "FAILED to write", rbPath.c_str());
	rebuildRouteCache();
}

void computeNav()
{
	App& a = App::get();
	NavState& nv = a.nav;
	nv.curHeading = geo::wrap360(a.yaw);
	nv.pos = { a.x, a.z };
	const auto& route = a.roadbook.route;
	if (a.routeVec.size() >= 2 && route.size() == a.routeVec.size())
	{
		geo::Projection pr = geo::projectToPolyline(a.routeVec, { a.x, a.z }, a.matchHint, 40, std::max(40.0f, a.nav.offRouteTolM));
		if (pr.seg >= 0 && pr.seg + 1 < static_cast<int>(route.size()))
		{
			a.matchHint = pr.seg;
			float segD = route[pr.seg].distM + pr.t * (route[pr.seg + 1].distM - route[pr.seg].distM);
			geo::Vec2 matched = { route[pr.seg].x + (route[pr.seg + 1].x - route[pr.seg].x) * pr.t,
			                      route[pr.seg].z + (route[pr.seg + 1].z - route[pr.seg].z) * pr.t };
			nv.tripM = segD;
			nv.offRouteM = pr.dist;
			nv.matched = true;
			geo::Vec2 ahead = matched;
			float aheadD = segD + 15.0f;
			for (size_t i = static_cast<size_t>(pr.seg) + 1; i < route.size(); ++i)
			{
				ahead = { route[i].x, route[i].z };
				if (route[i].distM >= aheadD) break;
			}
			nv.targetHeading  = geo::headingDeg(ahead - matched);
			nv.bearingToRoute = geo::headingDeg(matched - geo::Vec2{ a.x, a.z });
			return;
		}
	}
	nv.matched = false;
	nv.tripM = a.distM;
	nv.offRouteM = 0.0f;
}

void applySettings()
{
	App& a = App::get();
	a.nav.challenge    = a.settings.challenge;
	a.nav.panelOpacity = a.settings.opacity;
	a.nav.offRouteTolM = static_cast<float>(a.settings.offRouteToleranceM);
	a.nav.styleIndex   = a.settings.styleIndex;
	a.nav.frenchNotation = (a.settings.abbrevStyle == 1);
	a.nav.advanceLookM = static_cast<float>(a.settings.advanceLookM);
	a.nav.gScale = a.settings.scale;
	a.nav.gOffX  = a.settings.offX;
	a.nav.gOffY  = a.settings.offY;
	a.nav.mStrip    = a.settings.mStrip;
	a.nav.mCompass  = a.settings.mCompass;
	a.nav.mNext     = a.settings.mNext;
	a.nav.mOffroute = a.settings.mOffroute;
	a.nav.mMinimap  = a.settings.mMinimap;
	a.nav.mDash     = a.settings.mDash;
	a.nav.mDriver   = a.settings.mDriver;
	a.nav.mProfile  = a.settings.mProfile;
	a.nav.deviceFrame = a.settings.deviceFrame;
	a.nav.deviceModel = a.settings.deviceModel;
	a.nav.lcdColor = a.settings.lcdColor;
	a.nav.maskBox = a.settings.maskBox;
	a.nav.noCap = a.settings.noCap;
	a.nav.distanceOnly = a.settings.distanceOnly;
}

// Position presets for the rally-computer device (moves the strip transform).
// 0 = chase (right edge), 1 = onboard/cockpit (smaller, low-centre near the bars).
void applyPreset(int p)
{
	App& a = App::get();
	switch (((p % 3) + 3) % 3)
	{
	case 1:  a.settings.mStrip.scale = 0.62f; a.settings.mStrip.x = -0.403f; a.settings.mStrip.y = 0.193f; break; // bottom-centre
	case 2:  a.settings.mStrip.scale = 0.48f; a.settings.mStrip.x = -0.403f; a.settings.mStrip.y = 0.243f; break; // onboard / cockpit
	default: a.settings.mStrip.scale = 1.00f; a.settings.mStrip.x =  0.000f; a.settings.mStrip.y = 0.000f; break; // chase (right)
	}
	a.settings.layoutPreset = ((p % 3) + 3) % 3;
}

std::string settingsPath(const std::string& savePath)
{
	std::string dir = savePath.empty() ? "." : savePath;
	if (dir.back() != '\\' && dir.back() != '/') dir += '\\';
	return dir + "Roadbook\\settings.json";
}

void saveSettingsNow() { App& a = App::get(); model::saveSettings(a.settings, settingsPath(a.savePath)); }

// Overall rally score (0..100): on-route %, minus 5 per off-route event.
int rallyScore()
{
	App& a = App::get();
	double pct = a.sampleCount ? 100.0 * a.onRouteCount / a.sampleCount : 100.0;
	int s = static_cast<int>(std::lround(pct)) - a.offRouteEvents * 5;
	return std::max(0, std::min(100, s));
}
const char* gradeLetter(int s) { return s >= 90 ? "A" : s >= 75 ? "B" : s >= 60 ? "C" : s >= 40 ? "D" : "F"; }

// --- blind-run replay: ridden line + per-box pass, drawn top-down ----------
std::vector<geo::Vec2>     g_runTrace;   // downsampled ridden positions (x, z)
std::vector<unsigned char> g_runOff;     // 1 if off-route at that sample
std::vector<int>           g_boxPass;    // per box: 1 if ever off-route while active
geo::Vec2                  g_lastTrace;
bool                       g_haveLastTrace = false;
std::vector<geo::Vec2>     g_prevTrace;  // previous run (ghost) for comparison
std::vector<unsigned char> g_prevOff;
int                        g_prevScore = -1;
unsigned long long         g_runStartMs = 0;        // wall-clock (ms) at run start
int                        g_runTimeSec = 0;         // elapsed run time, seconds
bool                       g_runSaved = false;       // history record written for this run
std::vector<unsigned long long> g_boxTime;           // ms elapsed when each box was reached (0 = not yet)
std::vector<unsigned long long> g_prevBoxTime;       // ghost split times
int                        g_prevTimeSec = -1;
bool                       g_showReplay = false;
bool                       g_showPage = false;
int                        g_pageIdx = 0;
int rallyScore();
void saveRunRecord();
void saveBestRun_();
void loadBestRun_();
void resetRun()
{
	if (g_runTrace.size() > 5)
	{
		int sc = rallyScore();
		// the ghost is the personal best: keep this run only if it beats the loaded best
		bool better = (g_prevScore < 0) || (sc > g_prevScore) || (sc == g_prevScore && g_prevTimeSec > 0 && g_runTimeSec < g_prevTimeSec);
		if (better) { g_prevTrace = g_runTrace; g_prevOff = g_runOff; g_prevScore = sc; g_prevBoxTime = g_boxTime; g_prevTimeSec = g_runTimeSec; saveBestRun_(); }
	}
	g_runTrace.clear(); g_runOff.clear(); g_boxPass.clear(); g_boxTime.clear(); g_haveLastTrace = false;
	g_runStartMs = 0; g_runTimeSec = 0; g_runSaved = false;
}

// Persist the just-finished run to history.json (time-trial + run history).
void saveRunRecord()
{
	App& a = App::get();
	int sc = rallyScore();
	SYSTEMTIME st; GetLocalTime(&st);
	char date[40];
	std::snprintf(date, sizeof date, "%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
	model::RunRecord r;
	r.track = (a.trackName[0] ? a.trackName : a.trackId);
	r.date = date; r.timeSec = g_runTimeSec; r.distKm = a.distM / 1000.0; r.score = sc; r.grade = gradeLetter(sc);
	model::appendRunRecord(historyPath(a.savePath), r);
}

// Persist the current ghost (= personal best) so it survives across sessions.
void saveBestRun_()
{
	App& a = App::get();
	model::BestRun b;
	b.score = g_prevScore; b.timeSec = g_prevTimeSec;
	b.traceX.reserve(g_prevTrace.size()); b.traceZ.reserve(g_prevTrace.size());
	for (const auto& p : g_prevTrace) { b.traceX.push_back(p.x); b.traceZ.push_back(p.y); }
	b.off.assign(g_prevOff.begin(), g_prevOff.end());
	for (auto t : g_prevBoxTime) b.boxTimeMs.push_back(static_cast<double>(t));
	model::saveBestRun(bestRunPath(a.savePath, a.trackId), b);
}

// Load the personal best for the current track into the ghost slot.
void loadBestRun_()
{
	App& a = App::get();
	model::BestRun b;
	if (!model::loadBestRun(bestRunPath(a.savePath, a.trackId), b))
	{ g_prevTrace.clear(); g_prevOff.clear(); g_prevBoxTime.clear(); g_prevScore = -1; g_prevTimeSec = -1; return; } // no best yet -> clear ghost (don't leak another track's)
	g_prevTrace.clear(); g_prevOff.clear(); g_prevBoxTime.clear();
	for (size_t i = 0; i < b.traceX.size(); ++i) g_prevTrace.push_back({ b.traceX[i], b.traceZ[i] });
	g_prevOff.assign(b.off.begin(), b.off.end());
	for (double t : b.boxTimeMs) g_prevBoxTime.push_back(static_cast<unsigned long long>(t));
	g_prevScore = b.score; g_prevTimeSec = b.timeSec;
}

void drawReplay(DrawCache& dc, int font)
{
	App& a = App::get();
	const auto& route = a.roadbook.route;
	const float px = 0.05f, py = 0.09f, pw = 0.74f, ph = 0.82f;
	dc.rect(px, py, pw, ph, abgr(14, 16, 20, 238));
	dc.rect(px, py, pw, 0.0016f, col::accent);
	dc.rect(px, py + ph, pw, 0.0016f, col::accent);
	dc.text("RUN REPLAY", px + 0.014f, py + 0.036f, 0.024f, 0, col::accent, font);
	int sc = rallyScore();
	double pct = a.sampleCount ? 100.0 * a.onRouteCount / a.sampleCount : 100.0;
	char b[96];
	int mm = g_runTimeSec / 60, ss = g_runTimeSec % 60;
	if (g_prevTimeSec >= 0)
		std::snprintf(b, sizeof b, "SCORE %d/100  grade %s  on-route %.0f%%  off %d  TIME %d:%02d (%+ds)", sc, gradeLetter(sc), pct, a.offRouteEvents, mm, ss, g_runTimeSec - g_prevTimeSec);
	else
		std::snprintf(b, sizeof b, "SCORE %d/100  grade %s  on-route %.0f%%  off %d  TIME %d:%02d", sc, gradeLetter(sc), pct, a.offRouteEvents, mm, ss);
	dc.text(b, px + 0.014f, py + 0.064f, 0.015f, 0, col::white, font);
	dc.text("F4 to close", px + pw - 0.10f, py + 0.036f, 0.012f, 0, abgr(150, 150, 150), font);
	if (route.size() < 2) { dc.text("No run recorded yet - ride a roadbook first.", px + 0.014f, py + 0.13f, 0.016f, 0, col::warn, font); return; }

	float minx = 1e9f, maxx = -1e9f, minz = 1e9f, maxz = -1e9f;
	for (auto& r : route)      { minx = std::min(minx, r.x); maxx = std::max(maxx, r.x); minz = std::min(minz, r.z); maxz = std::max(maxz, r.z); }
	for (auto& p : g_runTrace) { minx = std::min(minx, p.x); maxx = std::max(maxx, p.x); minz = std::min(minz, p.y); maxz = std::max(maxz, p.y); }
	for (auto& p : g_prevTrace) { minx = std::min(minx, p.x); maxx = std::max(maxx, p.x); minz = std::min(minz, p.y); maxz = std::max(maxz, p.y); }
	float spanx = std::max(1.0f, maxx - minx), spanz = std::max(1.0f, maxz - minz);
	const float mapx = px + 0.02f, mapy = py + 0.09f, mapw = (pw - 0.04f) * 0.64f, maph = ph - 0.16f;
	const float kAspect = 9.0f / 16.0f;
	float scaleZ = std::min(maph / spanz, (mapw / kAspect) / spanx), scaleX = scaleZ * kAspect;
	float cxw = (minx + maxx) * 0.5f, czw = (minz + maxz) * 0.5f;
	auto MX = [&](float x) { return mapx + mapw * 0.5f + (x - cxw) * scaleX; };
	auto MY = [&](float z) { return mapy + maph * 0.5f - (z - czw) * scaleZ; };

	for (size_t i = 1; i < route.size(); ++i)
		dc.line(MX(route[i - 1].x), MY(route[i - 1].z), MX(route[i].x), MY(route[i].z), 0.0016f, abgr(110, 116, 126, 210));
	for (size_t i = 1; i < g_prevTrace.size(); ++i) // ghost: previous run
		dc.line(MX(g_prevTrace[i - 1].x), MY(g_prevTrace[i - 1].y), MX(g_prevTrace[i].x), MY(g_prevTrace[i].y), 0.0013f, abgr(90, 150, 240, 150));
	for (size_t i = 1; i < g_runTrace.size(); ++i)
		dc.line(MX(g_runTrace[i - 1].x), MY(g_runTrace[i - 1].y), MX(g_runTrace[i].x), MY(g_runTrace[i].y), 0.0019f, g_runOff[i] ? abgr(240, 80, 60) : abgr(90, 220, 120));
	for (size_t bi = 0; bi < a.roadbook.boxes.size(); ++bi)
	{
		double bs = a.roadbook.boxes[bi].distTotalKm * 1000.0;
		float bx = route.back().x, bz = route.back().z;
		for (size_t i = 1; i < route.size(); ++i)
			if (route[i].distM >= bs) { float seg = std::max(1e-3f, route[i].distM - route[i - 1].distM); float t = static_cast<float>((bs - route[i - 1].distM) / seg); bx = route[i - 1].x + (route[i].x - route[i - 1].x) * t; bz = route[i - 1].z + (route[i].z - route[i - 1].z) * t; break; }
		unsigned long mc = (bi < g_boxPass.size() && g_boxPass[bi]) ? abgr(240, 80, 60) : abgr(255, 200, 60);
		dc.rect(MX(bx) - 0.003f, MY(bz) - 0.005f, 0.006f, 0.010f, mc);
	}
	// box timeline (right column)
	float tlx = mapx + mapw + 0.025f, tly = mapy;
	dc.text(g_prevBoxTime.empty() ? "BOX TIMELINE" : "BOX TIMELINE   (+/-s vs ghost)", tlx, tly, 0.013f, 0, col::accent, font);
	int shown = std::min<int>(20, static_cast<int>(a.roadbook.boxes.size()));
	for (int i = 0; i < shown; ++i)
	{
		const auto& bx = a.roadbook.boxes[i];
		bool off = (i < static_cast<int>(g_boxPass.size()) && g_boxPass[i]);
		const char* dir = bx.turnDir == model::TurnDir::Right ? "R" : bx.turnDir == model::TurnDir::Left ? "L" : "-";
		unsigned long long bt = (i < static_cast<int>(g_boxTime.size())) ? g_boxTime[i] : 0;
		int bsec = static_cast<int>(bt / 1000);
		char tb[56];
		if (bt > 0) std::snprintf(tb, sizeof tb, "%2d %s %.2fkm  %d:%02d %s", i, dir, bx.distTotalKm, bsec / 60, bsec % 60, off ? "OFF" : "");
		else        std::snprintf(tb, sizeof tb, "%2d %s %.2fkm   --  %s", i, dir, bx.distTotalKm, off ? "OFF" : "");
		dc.text(tb, tlx, tly + 0.022f + i * 0.014f, 0.0108f, 0, off ? abgr(240, 90, 70) : abgr(120, 210, 130), font);
		if (bt > 0 && i < static_cast<int>(g_prevBoxTime.size()) && g_prevBoxTime[i] > 0)
		{
			int dsec = static_cast<int>((static_cast<long long>(bt) - static_cast<long long>(g_prevBoxTime[i])) / 1000);
			char db[16]; std::snprintf(db, sizeof db, "%+ds", dsec);
			dc.text(db, tlx + 0.150f, tly + 0.022f + i * 0.014f, 0.0108f, 0, dsec <= 0 ? abgr(120, 220, 130) : abgr(245, 150, 70), font);
		}
	}
	if (g_prevScore >= 0) { std::snprintf(b, sizeof b, "ghost (prev run, blue): score %d", g_prevScore); dc.text(b, px + pw - 0.20f, py + 0.064f, 0.012f, 0, abgr(110, 160, 240), font); }
	dc.text("route grey   line green=on / red=off   ghost blue   boxes amber, missed red", px + 0.014f, py + ph - 0.022f, 0.011f, 0, abgr(150, 150, 150), font);
}

std::string roadbookDir(const std::string& sp)
{
	std::string d = sp.empty() ? "." : sp;
	if (d.back() != '\\' && d.back() != '/') d += '\\';
	return d + "Roadbook";
}

std::vector<std::string> listFiles(const std::string& dir, const std::string& pattern)
{
	std::vector<std::string> out;
	WIN32_FIND_DATAA fd;
	std::string search = dir + "\\" + pattern;
	HANDLE h = FindFirstFileA(search.c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return out;
	do { if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) out.push_back(fd.cFileName); }
	while (FindNextFileA(h, &fd));
	FindClose(h);
	return out;
}

std::vector<gen::RideSample> loadRideCsv(const std::string& path)
{
	std::vector<gen::RideSample> out;
	std::FILE* f = nullptr; fopen_s(&f, path.c_str(), "r");
	if (!f) return out;
	char line[512];
	while (std::fgets(line, sizeof line, f))
	{
		if (line[0] == '#' || line[0] == 't' || line[0] == '\n') continue;
		float t, x, y, z, yaw, vx, vy, vz, sp, pos, dist;
		int got = std::sscanf(line, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f", &t, &x, &y, &z, &yaw, &vx, &vy, &vz, &sp, &pos, &dist);
		if (got >= 4) { gen::RideSample s; s.x = x; s.z = z; s.y = y; s.yaw = (got >= 5 ? yaw : 0); s.speed = (got >= 9 ? sp : 0); s.dist = (got >= 11 ? dist : 0); out.push_back(s); }
	}
	std::fclose(f);
	return out;
}

std::string prettyName(std::string f)
{
	size_t sl = f.find_last_of("\\/"); if (sl != std::string::npos) f = f.substr(sl + 1);
	size_t dot = f.rfind('.'); if (dot != std::string::npos) f = f.substr(0, dot);
	if (f.rfind("roadbook_", 0) == 0) f = f.substr(9);
	for (char& c : f) if (c == '_') c = ' ';
	if (f.size() > 26) f = f.substr(0, 26);
	return f;
}

void loadRoadbookFile(const std::string& path, bool saveAsCurrent)
{
	App& a = App::get();
	model::Roadbook rb;
	if (model::readJson(path, rb) && !rb.boxes.empty())
	{
		a.roadbook = rb;
		rebuildRouteCache();
		render::triggerDeviceBoot();
		if (saveAsCurrent) model::writeJson(a.roadbook, roadbookPath(a.savePath, a.trackId));
		a.statusMsg = "loaded " + prettyName(path);
		rblog::line("menu: loaded %zu boxes from %s", a.roadbook.boxes.size(), path.c_str());
	}
	else { a.statusMsg = "load failed"; rblog::line("menu: load failed %s", path.c_str()); }
}

void generateFromRide(const std::string& path)
{
	App& a = App::get();
	auto samples = loadRideCsv(path);
	if (samples.size() < 10) { a.statusMsg = "ride too short"; return; }
	gen::Params p; p.turnThresholdDeg = static_cast<float>(a.settings.detailDeg);
	a.roadbook = gen::generate(samples, p, a.trackId, a.trackName);
	rebuildRouteCache();
	model::writeJson(a.roadbook, roadbookPath(a.savePath, a.trackId));
	a.statusMsg = "generated " + std::to_string(a.roadbook.boxes.size()) + " boxes";
	rblog::line("menu: generated %zu boxes from %s (detail %d)", a.roadbook.boxes.size(), path.c_str(), a.settings.detailDeg);
}

void buildMenuItems(std::vector<MenuItem>& items, const char*& title, std::string& footer)
{
	App& a = App::get();
	items.clear();
	auto apply = []() { applySettings(); saveSettingsNow(); };

	if (a.menuPage == 1) // settings
	{
		title = "SETTINGS";
		footer = "Left/Right change   F4 close";
		MenuItem s1; s1.type = MenuItem::SliderF; s1.label = "Global scale"; s1.fval = &a.settings.scale; s1.fmin = 0.6f; s1.fmax = 1.6f; s1.fstep = 0.05f; s1.fmt = "%.2f"; s1.action = apply; items.push_back(s1);
		MenuItem s2; s2.type = MenuItem::SliderI; s2.label = "Opacity";   s2.ival = &a.settings.opacity;  s2.imin = 40; s2.imax = 255; s2.istep = 15; s2.action = apply; items.push_back(s2);
		MenuItem s3; s3.type = MenuItem::SliderI; s3.label = "Detail";    s3.ival = &a.settings.detailDeg; s3.imin = 15; s3.imax = 70; s3.istep = 5; s3.fmt = "%d deg"; s3.action = apply; items.push_back(s3);
		MenuItem s4; s4.type = MenuItem::SliderI; s4.label = "Off-route tol"; s4.ival = &a.settings.offRouteToleranceM; s4.imin = 5; s4.imax = 120; s4.istep = 5; s4.fmt = "%d m"; s4.action = apply; items.push_back(s4);
		MenuItem s5; s5.type = MenuItem::SliderI; s5.label = "Advance look"; s5.ival = &a.settings.advanceLookM; s5.imin = 0; s5.imax = 40; s5.istep = 5; s5.fmt = "%d m"; s5.action = apply; items.push_back(s5);
		MenuItem b; b.type = MenuItem::Action; b.label = "Back"; b.action = []() { App::get().menuPage = 0; App::get().menu.sel = 0; }; items.push_back(b);
	}
	else if (a.menuPage == 2) // roadbooks
	{
		title = "ROADBOOKS";
		footer = a.statusMsg.empty() ? "Enter = load / generate    F4 close" : a.statusMsg;
		std::string rbdir = roadbookDir(a.savePath);
		MenuItem h; h.type = MenuItem::Header;

		MenuItem ex; ex.type = MenuItem::Action; ex.label = "Export current -> Roadbook\\shared";
		ex.action = []() {
			App& a = App::get();
			if (a.roadbook.boxes.size() < 2) { a.statusMsg = "No roadbook to export"; return; }
			std::string shared = roadbookDir(a.savePath) + "\\shared"; CreateDirectoryA(shared.c_str(), nullptr);
			std::string dst = shared + "\\roadbook_" + a.trackId + ".json";
			a.statusMsg = CopyFileA(roadbookPath(a.savePath, a.trackId).c_str(), dst.c_str(), FALSE) ? "Exported to Roadbook\\shared" : "Export failed";
		};
		items.push_back(ex);

		h.label = "Saved roadbooks"; items.push_back(h);
		int cnt = 0;
		for (const auto& f : listFiles(rbdir, "roadbook_*.json"))
		{
			if (cnt++ >= 6) break;
			MenuItem m; m.type = MenuItem::Action; m.label = prettyName(f);
			std::string full = rbdir + "\\" + f;
			m.action = [full]() { loadRoadbookFile(full, false); };
			items.push_back(m);
		}

		h.label = "Import (drop .json in Roadbook\\import)"; items.push_back(h);
		cnt = 0;
		for (const auto& f : listFiles(rbdir + "\\import", "*.json"))
		{
			if (cnt++ >= 4) break;
			MenuItem m; m.type = MenuItem::Action; m.label = "import " + prettyName(f);
			std::string full = rbdir + "\\import\\" + f;
			m.action = [full]() { loadRoadbookFile(full, true); };
			items.push_back(m);
		}

		h.label = "Generate from ride (newest first)"; items.push_back(h);
		auto rides = listFiles(rbdir + "\\rides", "*.csv");
		int shown = 0;
		for (size_t k = 0; k < rides.size() && shown < 5; ++k, ++shown)
		{
			const std::string& fn = rides[rides.size() - 1 - k];
			MenuItem m; m.type = MenuItem::Action; m.label = "gen " + prettyName(fn);
			std::string full = rbdir + "\\rides\\" + fn;
			m.action = [full]() { generateFromRide(full); };
			items.push_back(m);
		}

		MenuItem b; b.type = MenuItem::Action; b.label = "Back"; b.action = []() { App::get().menuPage = 0; App::get().menu.sel = 0; }; items.push_back(b);
	}
	else if (a.menuPage == 3) // stats
	{
		title = "RIDE STATS";
		footer = "Up/Down move   F4 close";
		double avg = a.speedCount ? a.sumSpeed / a.speedCount : 0.0;
		double pct = a.sampleCount ? 100.0 * a.onRouteCount / a.sampleCount : 100.0;
		char b[64];
		MenuItem h; h.type = MenuItem::Header;
		std::snprintf(b, sizeof b, "Distance: %.2f km", a.distM / 1000.0);   h.label = b; items.push_back(h);
		std::snprintf(b, sizeof b, "Time: %.0f s", a.rideTime);              h.label = b; items.push_back(h);
		std::snprintf(b, sizeof b, "Top speed: %.0f km/h", a.topSpeed * 3.6f); h.label = b; items.push_back(h);
		std::snprintf(b, sizeof b, "Avg speed: %.0f km/h", avg * 3.6f);      h.label = b; items.push_back(h);
		std::snprintf(b, sizeof b, "On-route: %.0f%%", pct);                 h.label = b; items.push_back(h);
		std::snprintf(b, sizeof b, "Off-route events: %d", a.offRouteEvents); h.label = b; items.push_back(h);
		{ int sc = rallyScore(); std::snprintf(b, sizeof b, "SCORE  %d / 100   grade %s", sc, gradeLetter(sc)); h.label = b; items.push_back(h); }
		MenuItem r;  r.type  = MenuItem::Action; r.label  = "Reset stats"; r.action  = []() { resetRide(); }; items.push_back(r);
		MenuItem bk; bk.type = MenuItem::Action; bk.label = "Back";        bk.action = []() { App::get().menuPage = 0; App::get().menu.sel = 0; }; items.push_back(bk);
	}
	else if (a.menuPage == 4) // modules
	{
		title = "MODULES";
		footer = "show / size / position    F4 close";
		struct M { const char* name; model::ModuleCfg* cfg; };
		M mods[] = {
			{ "Co-driver", &a.settings.mDriver },   { "Dash",    &a.settings.mDash },
			{ "Status",    &a.settings.mStatus },
			{ "Strip",     &a.settings.mStrip },    { "Compass", &a.settings.mCompass },
			{ "Next",      &a.settings.mNext },     { "Off-route", &a.settings.mOffroute },
			{ "Minimap",   &a.settings.mMinimap }, { "Elevation", &a.settings.mProfile },
		};
		for (auto& md : mods)
		{
			MenuItem h;  h.type  = MenuItem::Header;  h.label  = md.name; items.push_back(h);
			MenuItem on; on.type = MenuItem::Toggle;  on.label = "  show"; on.boolVal = &md.cfg->on; on.action = apply; items.push_back(on);
			MenuItem sc; sc.type = MenuItem::SliderF; sc.label = "  size"; sc.fval = &md.cfg->scale; sc.fmin = 0.5f; sc.fmax = 2.0f; sc.fstep = 0.05f; sc.fmt = "%.2f"; sc.action = apply; items.push_back(sc);
			MenuItem mx; mx.type = MenuItem::SliderF; mx.label = "  x";    mx.fval = &md.cfg->x; mx.fmin = -0.4f; mx.fmax = 0.4f; mx.fstep = 0.01f; mx.fmt = "%+.2f"; mx.action = apply; items.push_back(mx);
			MenuItem my; my.type = MenuItem::SliderF; my.label = "  y";    my.fval = &md.cfg->y; my.fmin = -0.4f; my.fmax = 0.4f; my.fstep = 0.01f; my.fmt = "%+.2f"; my.action = apply; items.push_back(my);
			MenuItem op; op.type = MenuItem::SliderI; op.label = "  opacity (-1=glob)"; op.ival = &md.cfg->opacity; op.imin = -1; op.imax = 255; op.istep = 15; op.action = apply; items.push_back(op);
		}
		MenuItem b; b.type = MenuItem::Action; b.label = "Back"; b.action = []() { App::get().menuPage = 0; App::get().menu.sel = 0; }; items.push_back(b);
	}
	else if (a.menuPage == 5) // edit / review
	{
		title = "EDIT / REVIEW";
		int n = static_cast<int>(a.roadbook.boxes.size());
		if (n == 0) { footer = "No roadbook loaded"; }
		else
		{
			if (a.editBox >= n) a.editBox = n - 1;
			if (a.editBox < 0)  a.editBox = 0;
			a.nav.manualBox = a.editBox; // highlight this box in the strip
			model::Box& eb = a.roadbook.boxes[a.editBox];
			footer = "box " + std::to_string(a.editBox) + " / " + std::to_string(n - 1) + "   Enter toggles signs";
			MenuItem bx; bx.type = MenuItem::SliderI; bx.label = "Box";          bx.ival = &a.editBox;      bx.imin = 0; bx.imax = n - 1; bx.istep = 1; items.push_back(bx);
			MenuItem dg; dg.type = MenuItem::SliderI; dg.label = "Danger level"; dg.ival = &eb.dangerLevel; dg.imin = 0; dg.imax = 3;     dg.istep = 1; dg.action = []() { saveCurrentBook(); }; items.push_back(dg);
			MenuItem hh; hh.type = MenuItem::Header; hh.label = "Signs (Enter = add/remove)"; items.push_back(hh);
			for (int i = 0; i < model::picto::kCount; ++i)
			{
				for (int c = 0; c < model::picto::kCatCount; ++c)
					if (model::picto::kCats[c].start == i)
					{ MenuItem ch; ch.type = MenuItem::Header; ch.label = model::picto::kCats[c].name; items.push_back(ch); }
				const char* code = model::picto::kAll[i];
				MenuItem pg; pg.type = MenuItem::Action;
				pg.label = std::string(model::hasPictogram(eb, code) ? "[x] " : "[ ] ") + code;
				pg.action = [code]() { App& aa = App::get(); if (aa.editBox < static_cast<int>(aa.roadbook.boxes.size())) { model::togglePictogram(aa.roadbook.boxes[aa.editBox], code); saveCurrentBook(); } };
				items.push_back(pg);
			}
			MenuItem sv; sv.type = MenuItem::Action; sv.label = "Save book"; sv.action = []() { saveCurrentBook(); App::get().statusMsg = "saved"; }; items.push_back(sv);
		}
		MenuItem b; b.type = MenuItem::Action; b.label = "Back"; b.action = []() { App::get().menuPage = 0; App::get().menu.sel = 0; App::get().nav.manualBox = -1; }; items.push_back(b);
	}
	else if (a.menuPage == 6) // rally computer
	{
		title = "RALLY COMPUTER";
		footer = "Enter cycles   Left/Right sliders   F4 close";
		static const char* mn[] = { "Carbon", "Tablet", "Classic", "Stealth", "Desert", "Night", "Retro", "Enduro",
		                            "Titanium", "Forest", "Crimson", "Ocean", "Sandstone", "Mono", "Gold", "Ranger",
		                            "Ice", "Copper", "Lime", "Navy", "RaidNav", "Tripmaster", "RallyeF2" };
		static const char* cn[] = { "Default", "Amber", "Green", "Red", "Blue", "White" };
		static const char* vn[] = { "Chase (right)", "Bottom-centre", "Onboard" };
		char b[48];
		MenuItem fr; fr.type = MenuItem::Toggle; fr.label = "Show device frame"; fr.boolVal = &a.settings.deviceFrame; fr.action = apply; items.push_back(fr);
		MenuItem md; md.type = MenuItem::Action; std::snprintf(b, sizeof b, "Model: %s", mn[((a.settings.deviceModel % 23) + 23) % 23]); md.label = b;
		md.action = []() { auto& s = App::get().settings; s.deviceModel = (s.deviceModel + 1) % 23; applySettings(); saveSettingsNow(); }; items.push_back(md);
		MenuItem lc; lc.type = MenuItem::Action; std::snprintf(b, sizeof b, "LCD colour: %s", cn[((a.settings.lcdColor % 6) + 6) % 6]); lc.label = b;
		lc.action = []() { auto& s = App::get().settings; s.lcdColor = (s.lcdColor + 1) % 6; applySettings(); saveSettingsNow(); }; items.push_back(lc);
		MenuItem vw; vw.type = MenuItem::Action; std::snprintf(b, sizeof b, "View: %s", vn[((a.settings.layoutPreset % 3) + 3) % 3]); vw.label = b;
		vw.action = []() { App& a = App::get(); applyPreset(a.settings.layoutPreset + 1); applySettings(); saveSettingsNow(); }; items.push_back(vw);
		MenuItem sz; sz.type = MenuItem::SliderF; sz.label = "Size"; sz.fval = &a.settings.mStrip.scale; sz.fmin = 0.3f; sz.fmax = 1.6f; sz.fstep = 0.05f; sz.fmt = "%.2f"; sz.action = apply; items.push_back(sz);
		MenuItem op; op.type = MenuItem::SliderI; op.label = "Opacity"; op.ival = &a.settings.opacity; op.imin = 40; op.imax = 255; op.istep = 15; op.action = apply; items.push_back(op);
		MenuItem bk; bk.type = MenuItem::Action; bk.label = "Back"; bk.action = []() { App::get().menuPage = 0; App::get().menu.sel = 0; }; items.push_back(bk);
	}
	else if (a.menuPage == 7) // run history
	{
		title = "RUN HISTORY";
		footer = "Up/Down move   F4 close";
		auto hist = model::loadRunHistory(historyPath(a.savePath));
		char b[112];
		if (hist.empty()) { MenuItem h; h.type = MenuItem::Header; h.label = "No runs yet - finish a roadbook to record one."; items.push_back(h); }
		else
		{
			int bestSec = 1 << 30; std::string bestTrack;
			for (const auto& r : hist) if (r.timeSec > 0 && r.timeSec < bestSec) { bestSec = r.timeSec; bestTrack = r.track; }
			if (bestSec < (1 << 30)) { std::snprintf(b, sizeof b, "Best time: %d:%02d  (%s)", bestSec / 60, bestSec % 60, bestTrack.c_str()); MenuItem hb; hb.type = MenuItem::Header; hb.label = b; items.push_back(hb); }
			MenuItem hh; hh.type = MenuItem::Header; hh.label = "Recent (newest first)"; items.push_back(hh);
			int shown = 0;
			for (int k = static_cast<int>(hist.size()) - 1; k >= 0 && shown < 12; --k, ++shown)
			{
				const auto& r = hist[k];
				std::snprintf(b, sizeof b, "%s  %d:%02d  %d%% %s  %.1fkm  %s", r.date.c_str(), r.timeSec / 60, r.timeSec % 60, r.score, r.grade.c_str(), r.distKm, r.track.c_str());
				MenuItem m; m.type = MenuItem::Header; m.label = b; items.push_back(m);
			}
		}
		MenuItem bk; bk.type = MenuItem::Action; bk.label = "Back"; bk.action = []() { App::get().menuPage = 0; App::get().menu.sel = 0; }; items.push_back(bk);
	}
	else // main
	{
		title = "ROADBOOK MENU";
		footer = a.statusMsg.empty() ? "Up/Down move   Enter select   F4 close" : a.statusMsg;
		MenuItem st; st.type = MenuItem::Action;
		{ static const char* nm[] = { "Paper", "Dakar", "Carbon", "Blueprint" }; char b[32]; std::snprintf(b, sizeof b, "Style: %s", nm[((a.settings.styleIndex % 4) + 4) % 4]); st.label = b; }
		st.action = []() { auto& s = App::get().settings; s.styleIndex = (s.styleIndex + 1) % 4; applySettings(); saveSettingsNow(); };
		items.push_back(st);
		MenuItem ab; ab.type = MenuItem::Action; ab.label = a.settings.abbrevStyle == 1 ? "Notation: French (D / G / TD)" : "Notation: International (R / L / kpS)";
		ab.action = []() { auto& s = App::get().settings; s.abbrevStyle = 1 - s.abbrevStyle; applySettings(); saveSettingsNow(); }; items.push_back(ab);
		MenuItem dv; dv.type = MenuItem::Action; dv.label = "Rally computer (model / view)"; dv.action = []() { App::get().menuPage = 6; App::get().menu.sel = 0; }; items.push_back(dv);
		MenuItem m1; m1.type = MenuItem::Action; m1.label = "Settings";              m1.action = []() { App::get().menuPage = 1; App::get().menu.sel = 0; }; items.push_back(m1);
		MenuItem m1b; m1b.type = MenuItem::Action; m1b.label = "Modules (toggle/size)"; m1b.action = []() { App::get().menuPage = 4; App::get().menu.sel = 0; }; items.push_back(m1b);
		MenuItem m2; m2.type = MenuItem::Action; m2.label = "Roadbooks (load / gen)"; m2.action = []() { App::get().menuPage = 2; App::get().menu.sel = 0; }; items.push_back(m2);
		MenuItem m3; m3.type = MenuItem::Action; m3.label = "Ride stats";            m3.action = []() { App::get().menuPage = 3; App::get().menu.sel = 0; }; items.push_back(m3);
		MenuItem rh; rh.type = MenuItem::Action; rh.label = "Run history";           rh.action = []() { App::get().menuPage = 7; App::get().menu.sel = 0; }; items.push_back(rh);
		MenuItem rp; rp.type = MenuItem::Action; rp.label = "Run replay (last run)";  rp.action = []() { g_showReplay = true; App::get().menu.open = false; }; items.push_back(rp);
		MenuItem pg; pg.type = MenuItem::Action; pg.label = "Roadbook page (review all)"; pg.action = []() { g_showPage = true; g_pageIdx = 0; App::get().menu.open = false; }; items.push_back(pg);
		MenuItem me; me.type = MenuItem::Action; me.label = "Edit / review book";    me.action = []() { App::get().menuPage = 5; App::get().menu.sel = 0; }; items.push_back(me);

		MenuItem rsv; rsv.type = MenuItem::Action;
		rsv.label = a.rec.recording() ? "Save roadbook now (recording)" : "Save roadbook now (from ride)";
		rsv.action = []() {
			App& a = App::get();
			a.rec.flush();
			if (a.rec.samples().size() < 10) { a.statusMsg = "Ride a bit further, then save"; return; }
			generateRoadbook();
			char b[64]; std::snprintf(b, sizeof b, "Saved: %zu boxes, %.2f km", a.roadbook.boxes.size(), a.roadbook.totalDistanceKm);
			a.statusMsg = b;
		};
		items.push_back(rsv);
		MenuItem rst; rst.type = MenuItem::Action; rst.label = "Restart recording (fresh ride)";
		rst.action = []() { App& a = App::get(); a.rec.start(); a.distM = 0.0; resetRun(); a.statusMsg = "Recording restarted from here"; };
		items.push_back(rst);

		MenuItem m4; m4.type = MenuItem::Toggle; m4.label = "Challenge mode";         m4.boolVal = &a.settings.challenge; m4.action = apply; items.push_back(m4);
		MenuItem t1; t1.type = MenuItem::Toggle; t1.label = "Train: masked (1 box)";  t1.boolVal = &a.settings.maskBox;      t1.action = apply; items.push_back(t1);
		MenuItem t2; t2.type = MenuItem::Toggle; t2.label = "Train: no CAP";           t2.boolVal = &a.settings.noCap;        t2.action = apply; items.push_back(t2);
		MenuItem t3; t3.type = MenuItem::Toggle; t3.label = "Train: distance only";    t3.boolVal = &a.settings.distanceOnly; t3.action = apply; items.push_back(t3);
		MenuItem au; au.type = MenuItem::Toggle; au.label = "Audio (warning tones)";   au.boolVal = &a.settings.audio;        au.action = apply; items.push_back(au);
		MenuItem vo; vo.type = MenuItem::Toggle; vo.label = "Voice co-driver";          vo.boolVal = &a.settings.voice;        vo.action = apply; items.push_back(vo);
		MenuItem vs; vs.type = MenuItem::Toggle; vs.label = "  full pace-note sentences"; vs.boolVal = &a.settings.paceSentences; vs.action = apply; items.push_back(vs);
		MenuItem m5; m5.type = MenuItem::Action; m5.label = "Close";                  m5.action = []() { App::get().menu.open = false; saveSettingsNow(); }; items.push_back(m5);
	}
}
} // namespace

// ===========================================================================
// Identity
// ===========================================================================
char* GetModID()           { static char id[] = "mxbikes"; return id; }
int   GetModDataVersion()  { return kModDataVersion; }
int   GetInterfaceVersion(){ return kInterfaceVersion; }

// ===========================================================================
// Lifecycle
// ===========================================================================
int Startup(char* _szSavePath)
{
	try
	{
		rblog::init(_szSavePath ? _szSavePath : "");
		rblog::line("Startup savePath='%s'  modDataVer=%d interfaceVer=%d hz=%d",
		          _szSavePath ? _szSavePath : "(null)", kModDataVersion, kInterfaceVersion, kTelemetryHz);
		rblog::line("struct sizes: BikeData=%d BikeEvent=%d", apisz::kBikeData, apisz::kBikeEvent);

		App& a = App::get();
		a.savePath = _szSavePath ? _szSavePath : "";
		a.rec.setSavePath(a.savePath.c_str());
		a.in.watch(VK_F8);  // toggle overlay
		a.in.watch(VK_F5);  // reload roadbook.json
		a.in.watch(VK_F6);  // scroll review back
		a.in.watch(VK_F7);  // scroll review forward
		a.in.watch(VK_F9);  // back to auto + reset odometer
		a.in.watch(VK_F10); // toggle minimap
		a.in.watch(VK_F12); // toggle off-route alert
		a.in.watch(VK_F4);  // open/close menu
		a.in.watch(VK_UP); a.in.watch(VK_DOWN);
		a.in.watch(VK_LEFT); a.in.watch(VK_RIGHT); a.in.watch(VK_RETURN);
		model::loadSettings(a.settings, settingsPath(a.savePath));
		applySettings();
		CreateDirectoryA((roadbookDir(a.savePath) + "\\import").c_str(), nullptr);
		return kTelemetryHz;
	}
	GUARD_CATCH("Startup")
	return -1; // unloads the plugin
}

void Shutdown()
{
	try
	{
		App::get().rec.stop();
		rblog::line("Shutdown");
		rblog::shutdown();
	}
	GUARD_CATCH("Shutdown")
}

void EventInit(void* _pData, int _iDataSize)
{
	try
	{
		App& a = App::get();
		// Robust copy: tolerate older/newer struct sizes by copying the prefix
		// we share with the game into a zero-initialized local.
		SPluginsBikeEvent_t ev{};
		size_t copy = (_iDataSize > 0)
			? std::min(static_cast<size_t>(_iDataSize), sizeof(ev)) : 0;
		if (_pData && copy > 0) std::memcpy(&ev, _pData, copy);

		std::strncpy(a.trackId,   ev.m_szTrackID,   sizeof(a.trackId)   - 1);
		std::strncpy(a.trackName, ev.m_szTrackName, sizeof(a.trackName) - 1);
		a.trackId[sizeof(a.trackId) - 1]     = '\0';
		a.trackName[sizeof(a.trackName) - 1] = '\0';
		a.trackLength = ev.m_fTrackLength;
		a.rec.setEvent(a.trackId, a.trackName, a.trackLength);
		rblog::line("EventInit track='%s' id='%s' len=%.1fm type=%d (size=%d)",
		          a.trackName, a.trackId, a.trackLength, ev.m_iType, _iDataSize);

		// Load a saved roadbook for this track, if one exists.
		a.roadbook = model::Roadbook{};
		std::string rbPath = roadbookPath(a.savePath, a.trackId);
		model::Roadbook loaded;
		if (model::readJson(rbPath, loaded) && !loaded.boxes.empty())
		{
			a.roadbook = loaded;
			rblog::line("EventInit: loaded roadbook with %zu boxes from %s",
			            a.roadbook.boxes.size(), rbPath.c_str());
		}
		else rblog::line("EventInit: no saved roadbook at %s", rbPath.c_str());
		rebuildRouteCache();
		loadBestRun_(); // restore the personal-best ghost for this track
	}
	GUARD_CATCH("EventInit")
}

void EventDeinit() { try { rblog::line("EventDeinit"); } GUARD_CATCH("EventDeinit") }

void RunInit(void* /*_pData*/, int /*_iDataSize*/)
{
	try { rblog::line("RunInit"); resetRide(); render::triggerDeviceBoot(); } GUARD_CATCH("RunInit")
}

void RunDeinit() { try { rblog::line("RunDeinit"); App& a = App::get(); if (a.roadbook.boxes.size() < 2) generateRoadbook(); a.rec.stop(); } GUARD_CATCH("RunDeinit") }

void RunStart()
{
	try { rblog::line("RunStart"); App::get().rec.start(); } GUARD_CATCH("RunStart")
}

void RunStop() { try { rblog::line("RunStop dist=%.1fm ticks=%lld", App::get().distM, App::get().ticks); App::get().rec.flush(); } GUARD_CATCH("RunStop") }

void RunLap(void* /*_pData*/, int /*_iDataSize*/)   { try { rblog::line("RunLap"); }   GUARD_CATCH("RunLap") }
void RunSplit(void* /*_pData*/, int /*_iDataSize*/) { try { /* ignored in stage 0 */ } GUARD_CATCH("RunSplit") }

void RunTelemetry(void* _pData, int _iDataSize, float _fTime, float _fPos)
{
	try
	{
		if (!_pData || !apisz::ok(_iDataSize, apisz::kBikeData))
		{
			static bool warned = false;
			if (!warned) { warned = true; rblog::line("RunTelemetry size %d < %d, ignoring", _iDataSize, apisz::kBikeData); }
			return;
		}
		const auto* b = static_cast<const SPluginsBikeData_t*>(_pData);
		App& a = App::get();

		a.ticks++;
		a.pos   = _fPos;
		a.x     = b->m_fPosX; a.y = b->m_fPosY; a.z = b->m_fPosZ;
		a.yaw   = b->m_fYaw;
		a.speed = b->m_fSpeedometer;
		a.vx    = b->m_fVelocityX; a.vy = b->m_fVelocityY;

		a.rec.onTelemetry(*b, _fTime, _fPos);

		// Gated distance integration (the in-game odometer).
		if (a.haveLastTime)
		{
			float dt = _fTime - a.lastTime;
			if (dt > 0.0f && dt <= kMaxDt)
			{
				float v = std::max(a.speed, std::sqrt(a.vx * a.vx + a.vy * a.vy));
				if (v >= kMinMoveSpeed) a.distM += static_cast<double>(v) * dt;
			}
		}
		a.lastTime = _fTime;
		a.haveLastTime = true;

		a.rideTime = _fTime;
		if (a.speed > a.topSpeed) a.topSpeed = a.speed;
		if (a.speed > 0.5f) { a.sumSpeed += a.speed; a.speedCount++; }

		// Periodic sample (~1/s at 100 Hz) so we can determine the world up-axis
		// and verify the distance integrator from the log. Stage 0 diagnostic only.
		if ((a.ticks % 100) == 0)
		{
			rblog::line("tlm t=%.2f pos=%.4f x=%.1f y=%.1f z=%.1f yaw=%.1f spd=%.2f dist=%.1f",
			            _fTime, _fPos, a.x, a.y, a.z, a.yaw, a.speed, a.distM);
		}
	}
	GUARD_CATCH("RunTelemetry")
}

// ===========================================================================
// Drawing
// ===========================================================================
int DrawInit(int* _piNumSprites, char** _pszSpriteName, int* _piNumFonts, char** _pszFontName)
{
	if (_piNumSprites) *_piNumSprites = 0;
	if (_piNumFonts)   *_piNumFonts   = 0;
	try
	{
		// Font files: paths are relative to the plugins folder. We ship our own
		// data folder so the mod is self-contained.
		// Path is relative to the plugins folder and MUST use backslashes — the
		// game's font loader rejects forward slashes (matches mxbmrp3's
		// AssetManager::getFontPath format: "<data>\\fonts\\<name>.fnt").
		static char fontNames[] = "Roadbook_data\\fonts\\RobotoMono-Regular.fnt\0Roadbook_data\\fonts\\RobotoMono-Bold.fnt";
		// Register the pictogram sprites (paths relative to plugins folder,
		// backslashes). Order matches model::picto::kAll so sprite index = i+1.
		static std::string spriteBuf;
		static int spriteCount = 0;
		if (spriteBuf.empty())
		{
			for (int i = 0; i < model::picto::kCount; ++i)
			{
				spriteBuf += "Roadbook_data\\icons\\rb_";
				spriteBuf += model::picto::kAll[i];
				spriteBuf += ".tga";
				spriteBuf += '\0';
				++spriteCount;
			}
			// extra sprite after the pictograms: the official tulip arrowhead
			// (index = kCount+1), used rotated by the renderer.
			spriteBuf += "Roadbook_data\\icons\\rb_arrow.tga";
			spriteBuf += '\0';
			++spriteCount;
			// rally-computer device models (indices kCount+2 .. kCount+21).
			for (int mdl = 0; mdl < 23; ++mdl)
			{
				spriteBuf += "Roadbook_data\\icons\\rb_device";
				spriteBuf += std::to_string(mdl);
				spriteBuf += ".tga";
				spriteBuf += '\0';
				++spriteCount;
			}
		}
		if (_pszFontName)   *_pszFontName   = fontNames;
		if (_piNumFonts)    *_piNumFonts    = 2;
		if (_pszSpriteName) *_pszSpriteName = spriteBuf.data();
		if (_piNumSprites)  *_piNumSprites  = spriteCount;
		rblog::line("DrawInit: 2 fonts, %d sprites", spriteCount);
		return 0; // 0 = success for this API (mxbmrp3 returns 0; returning 1 made the game ignore registration)
	}
	GUARD_CATCH("DrawInit")
	return 0;
}

void Draw(int _iState, int* _piNumQuads, void** _ppQuad, int* _piNumString, void** _ppString)
{
	// Zero outputs first so a throw leaves the game reading "nothing to draw".
	if (_piNumQuads)  *_piNumQuads  = 0;
	if (_ppQuad)      *_ppQuad      = nullptr;
	if (_piNumString) *_piNumString = 0;
	if (_ppString)    *_ppString    = nullptr;
	try
	{
		App& a = App::get();
		a.in.update();
		if (a.in.clicked(VK_F4)) { if (g_showReplay) g_showReplay = false; else if (g_showPage) g_showPage = false; else { a.menu.toggleOpen(); if (!a.menu.open) { saveSettingsNow(); a.nav.manualBox = -1; } } }
		if (a.in.clicked(VK_F8))  a.overlayVisible = !a.overlayVisible;
		if (a.in.clicked(VK_F10)) { a.settings.mMinimap.on  = !a.settings.mMinimap.on;  applySettings(); saveSettingsNow(); }
		if (a.in.clicked(VK_F12)) { a.settings.mOffroute.on = !a.settings.mOffroute.on; applySettings(); saveSettingsNow(); }
		if (a.in.clicked(VK_F5))  reloadRoadbook();

		computeNav();

		if (a.speed > 1.0f && !a.menu.open)
		{
			float tol = a.nav.offRouteTolM;
			a.sampleCount++;
			if (a.nav.matched && a.nav.offRouteM < tol) a.onRouteCount++;
			if (a.nav.matched && a.nav.offRouteM > tol * 1.6f && !a.wasOffRoute) { a.offRouteEvents++; a.wasOffRoute = true; }
			if (a.nav.offRouteM < tol * 0.7f) a.wasOffRoute = false;

				// record the run trace + per-box pass for the replay
				if (a.nav.matched && !a.roadbook.boxes.empty())
				{
					if (static_cast<int>(g_boxPass.size()) != static_cast<int>(a.roadbook.boxes.size())) g_boxPass.assign(a.roadbook.boxes.size(), 0);
					if (g_boxTime.size() != a.roadbook.boxes.size()) g_boxTime.assign(a.roadbook.boxes.size(), 0);
					unsigned long long nowMs = GetTickCount64();
					if (g_runStartMs == 0) g_runStartMs = nowMs;
					g_runTimeSec = static_cast<int>((nowMs - g_runStartMs) / 1000);
					geo::Vec2 p = a.nav.pos;
					if (!g_haveLastTrace || std::hypot(p.x - g_lastTrace.x, p.y - g_lastTrace.y) > 4.0f)
					{
						if (g_runTrace.size() < 6000) { g_runTrace.push_back(p); g_runOff.push_back(a.nav.offRouteM > tol ? 1 : 0); }
						g_lastTrace = p; g_haveLastTrace = true;
					}
					int act = 0;
					for (int i = 0; i < static_cast<int>(a.roadbook.boxes.size()); ++i) if (a.roadbook.boxes[i].distTotalKm * 1000.0 <= a.nav.tripM + 15.0) act = i; else break;
					if (act < static_cast<int>(g_boxPass.size()) && a.nav.offRouteM > tol) g_boxPass[act] = 1;
					if (act < static_cast<int>(g_boxTime.size()) && g_boxTime[act] == 0) g_boxTime[act] = nowMs - g_runStartMs;
					int nbx = static_cast<int>(a.roadbook.boxes.size());
					if (!g_runSaved && nbx > 2 && act >= nbx - 1 && g_runTimeSec > 5) { saveRunRecord(); g_runSaved = true; a.statusMsg = "Run saved to history"; }
				}
		}

		if (!a.roadbook.boxes.empty())
		{
			// look further ahead at higher speed so the instruction arrives with enough
			// reaction time (base advance-look + ~1.2 s of travel)
			const double lookM = a.nav.advanceLookM + a.speed * 1.2;
			auto autoActive = [&]() {
				int A = 0;
				for (int i = 0; i < static_cast<int>(a.roadbook.boxes.size()); ++i)
					if (a.roadbook.boxes[i].distTotalKm * 1000.0 <= a.nav.tripM + lookM) A = i; else break;
				return A;
			};
			if (g_showPage) { if (a.in.clicked(VK_F6)) g_pageIdx = std::max(0, g_pageIdx - 1); if (a.in.clicked(VK_F7)) ++g_pageIdx; }
				if (a.in.clicked(VK_F6)) a.nav.manualBox = std::max(0, (a.nav.manualBox < 0 ? autoActive() : a.nav.manualBox) - 1);
			if (a.in.clicked(VK_F7)) a.nav.manualBox = std::min(static_cast<int>(a.roadbook.boxes.size()) - 1, (a.nav.manualBox < 0 ? autoActive() : a.nav.manualBox) + 1);
			if (a.in.clicked(VK_F9)) { a.nav.manualBox = -1; a.distM = 0.0; resetRun(); }

				// --- warning tones (best-effort; silent if WAVs missing) ---
				if ((a.settings.audio || a.settings.voice) && a.speed > 1.0f && !a.menu.open)
				{
					int active = autoActive(), ni = active + 1, nb = static_cast<int>(a.roadbook.boxes.size());
					if (ni < nb)
					{
						double toNext = a.roadbook.boxes[ni].distTotalKm * 1000.0 - a.nav.tripM;
						// scale the call lead with speed so the warning time is consistent
						double lead = (a.settings.voice && a.settings.paceSentences)
						              ? std::min(250.0, std::max(80.0, a.speed * 6.0))   // ~6 s for a full sentence
						              : std::min(120.0, std::max(40.0, a.speed * 4.0));  // ~4 s for a word / tone
						if (toNext > 0.0 && toNext < lead && g_lastChimeBox != ni)
						{
							const model::Box& nb2 = a.roadbook.boxes[ni];
							if (a.settings.voice && a.settings.paceSentences)
								playSentence(paceNoteClips(nb2, static_cast<int>(toNext)));
							else if (a.settings.voice)
								playSound(nb2.dangerLevel >= 2 ? "v_danger.wav"
								        : nb2.type == model::BoxType::Hairpin ? "v_hairpin.wav"
								        : nb2.type == model::BoxType::Straight ? "v_straight.wav"
								        : nb2.turnDir == model::TurnDir::Right ? "v_right.wav" : "v_left.wav");
							else if (a.settings.audio)
								playSound(nb2.dangerLevel >= 2 ? "rb_danger.wav" : "rb_chime.wav");
							g_lastChimeBox = ni;
						}
					}
					float tol = a.nav.offRouteTolM;
					if (a.nav.matched && a.nav.offRouteM > tol * 1.4f && !g_offRouteAudioLatch) { playSound("rb_offroute.wav"); g_offRouteAudioLatch = true; }
					if (a.nav.offRouteM < tol * 0.8f) g_offRouteAudioLatch = false;
				}
		}

		std::vector<MenuItem> menuItems;
		const char* menuTitle = "";
		std::string menuFooter;
		if (a.menu.open)
		{
			buildMenuItems(menuItems, menuTitle, menuFooter);
			a.menu.handleInput(a.in, menuItems);
		}

		a.draw.begin();
		if (a.overlayVisible)
		{
			const int   font = 1;
			const float ts   = 0.018f;
			char buf[160];
			a.nav.recording = a.rec.recording();
			a.nav.speedKmh  = a.speed * 3.6f;

			if (a.settings.mStatus.on)
			{
				a.draw.setTransform(a.settings.scale * a.settings.mStatus.scale,
				                    a.settings.offX + a.settings.mStatus.x,
				                    a.settings.offY + a.settings.mStatus.y, 0.008f, 0.162f);
				a.draw.rect(0.008f, 0.162f, 0.30f, 0.066f, col::panel);
				std::snprintf(buf, sizeof buf, "ROADBOOK  %.20s", a.trackName[0] ? a.trackName : "(no track)");
				a.draw.text(buf, 0.016f, 0.182f, 0.019f, 0, col::accent, font);
				std::snprintf(buf, sizeof buf, "REC %s %lld  boxes %zu  %s",
				              a.rec.recording() ? "ON" : "off", a.rec.rows(), a.roadbook.boxes.size(),
				              a.nav.matched ? "matched" : "no-match");
				a.draw.text(buf, 0.016f, 0.210f, ts, 0, col::white, font);
				if (a.settings.challenge)
				{
					double pct = a.sampleCount ? 100.0 * a.onRouteCount / a.sampleCount : 100.0;
					{ int sc = rallyScore(); std::snprintf(buf, sizeof buf, "CHALLENGE  %.0f%%  off %d   SCORE %d %s", pct, a.offRouteEvents, sc, gradeLetter(sc)); }
					a.draw.rect(0.008f, 0.232f, 0.30f, 0.030f, abgr(50, 0, 0, 190));
					a.draw.text(buf, 0.016f, 0.253f, 0.016f, 0, col::warn, font);
				}
				a.draw.resetTransform();
			}

			if (a.roadbook.boxes.size() >= 2)
				a.renderer.draw(a.draw, font, a.roadbook, a.nav);
			else
			{
				a.draw.rect(0.35f, 0.45f, 0.32f, 0.06f, col::panel);
				a.draw.text("Ride a lap, then pause (ESC) to build the roadbook",
				            0.36f, 0.487f, 0.016f, 0, col::white, font);
			}
		}

		if (a.menu.open)
		{
			buildMenuItems(menuItems, menuTitle, menuFooter);
			a.menu.draw(a.draw, 1, menuTitle, menuFooter, menuItems);
		}

		if (g_showReplay) drawReplay(a.draw, 1);
		if (g_showPage) a.renderer.drawPage(a.draw, 1, a.roadbook, g_pageIdx, a.settings.styleIndex);
		a.draw.commit(_piNumQuads, _ppQuad, _piNumString, _ppString);
	}
	GUARD_CATCH("Draw")
}

void TrackCenterline(int /*_iNumSegments*/, SPluginsTrackSegment_t* /*_pasSegment*/, void* /*_pRaceData*/)
{
	try { /* stage 0: unused */ } GUARD_CATCH("TrackCenterline")
}
