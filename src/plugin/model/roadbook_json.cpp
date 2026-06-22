#include "roadbook_json.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace model {
namespace {

BoxType boxTypeFrom(const std::string& s)
{
	if (s == "straight") return BoxType::Straight;
	if (s == "hairpin")  return BoxType::Hairpin;
	if (s == "start")    return BoxType::Start;
	if (s == "finish")   return BoxType::Finish;
	return BoxType::Turn;
}
TurnDir turnDirFrom(const std::string& s)
{
	if (s == "left")  return TurnDir::Left;
	if (s == "right") return TurnDir::Right;
	return TurnDir::None;
}
} // namespace

bool writeJson(const Roadbook& rb, const std::string& path)
{
	json j;
	j["schemaVersion"] = rb.schemaVersion;
	j["meta"] = {
		{ "trackId", rb.trackId }, { "trackName", rb.trackName },
		{ "generatedBy", rb.generatedBy }, { "sourceRide", rb.sourceRide },
		{ "totalDistanceKm", rb.totalDistanceKm },
	};
	json boxes = json::array();
	for (const auto& b : rb.boxes)
	{
		json pts = json::array();
		for (const auto& p : b.tulip) pts.push_back({ p.x, p.y });
		json jb = {
			{ "index", b.index },
			{ "distTotalKm", b.distTotalKm },
			{ "distPartialM", b.distPartialM },
			{ "capDeg", b.capDeg },
			{ "type", toString(b.type) },
			{ "turnDir", toString(b.turnDir) },
			{ "dangerLevel", b.dangerLevel },
			{ "pictograms", b.pictograms },
			{ "note", b.note },
			{ "crossDeg", b.crossDeg },
			{ "branchDeg", b.branchDeg },
			{ "tulip", {
				{ "points", pts },
				{ "entry", { b.entry.x, b.entry.y } },
				{ "exit", { { "at", { b.exitAt.x, b.exitAt.y } }, { "headingDeg", b.exitHeadingDeg } } },
			} },
		};
		boxes.push_back(jb);
	}
	j["boxes"] = boxes;

	json route = json::array();
	for (const auto& r : rb.route) route.push_back({ r.x, r.z, r.distM, r.elev });
	j["route"] = route;

	std::ofstream f(path);
	if (!f) return false;
	f << j.dump(2);
	return true;
}

bool readJson(const std::string& path, Roadbook& out)
{
	std::ifstream f(path);
	if (!f) return false;
	json j;
	try { f >> j; } catch (...) { return false; }

	Roadbook rb;
	rb.schemaVersion = j.value("schemaVersion", 1);
	if (j.contains("meta") && j["meta"].is_object())
	{
		const auto& m = j["meta"];
		rb.trackId         = m.value("trackId", "");
		rb.trackName       = m.value("trackName", "");
		rb.generatedBy     = m.value("generatedBy", "");
		rb.sourceRide      = m.value("sourceRide", "");
		rb.totalDistanceKm = m.value("totalDistanceKm", 0.0);
	}
	if (j.contains("boxes") && j["boxes"].is_array())
	{
		for (const auto& jb : j["boxes"])
		{
			Box b;
			b.index        = jb.value("index", 0);
			b.distTotalKm  = jb.value("distTotalKm", 0.0);
			b.distPartialM = jb.value("distPartialM", 0);
			b.capDeg       = jb.value("capDeg", 0);
			b.type         = boxTypeFrom(jb.value("type", "turn"));
			b.turnDir      = turnDirFrom(jb.value("turnDir", "none"));
			if (jb.contains("dangerLevel")) b.dangerLevel = jb.value("dangerLevel", 0);
			else { std::string d = jb.value("danger", "none"); b.dangerLevel = (d == "hairpin") ? 2 : (d == "caution") ? 1 : 0; }
			if (jb.contains("pictograms") && jb["pictograms"].is_array())
				b.pictograms = jb["pictograms"].get<std::vector<std::string>>();
			b.note = jb.value("note", "");
				b.crossDeg = jb.value("crossDeg", -999.0f);
			if (jb.contains("branchDeg") && jb["branchDeg"].is_array())
				for (const auto& v : jb["branchDeg"]) b.branchDeg.push_back(v.get<float>());
			if (jb.contains("tulip") && jb["tulip"].is_object())
			{
				const auto& t = jb["tulip"];
				if (t.contains("points") && t["points"].is_array())
					for (const auto& p : t["points"])
						if (p.is_array() && p.size() >= 2)
							b.tulip.push_back({ p[0].get<float>(), p[1].get<float>() });
				if (t.contains("entry") && t["entry"].is_array() && t["entry"].size() >= 2)
					b.entry = { t["entry"][0].get<float>(), t["entry"][1].get<float>() };
				if (t.contains("exit") && t["exit"].is_object())
				{
					const auto& e = t["exit"];
					if (e.contains("at") && e["at"].is_array() && e["at"].size() >= 2)
						b.exitAt = { e["at"][0].get<float>(), e["at"][1].get<float>() };
					b.exitHeadingDeg = e.value("headingDeg", 0.0f);
				}
			}
			rb.boxes.push_back(b);
		}
	}
	if (j.contains("route") && j["route"].is_array())
	{
		for (const auto& r : j["route"])
			if (r.is_array() && r.size() >= 3)
				rb.route.push_back({ r[0].get<float>(), r[1].get<float>(), r[2].get<float>(), r.size() >= 4 ? r[3].get<float>() : 0.0f });
	}
	out = rb;
	return true;
}

void appendRunRecord(const std::string& path, const RunRecord& r)
{
	json arr = json::array();
	{ std::ifstream in(path); if (in) { try { in >> arr; } catch (...) {} } }
	if (!arr.is_array()) arr = json::array();
	arr.push_back({ { "track", r.track }, { "date", r.date }, { "timeSec", r.timeSec },
	                { "distKm", r.distKm }, { "score", r.score }, { "grade", r.grade } });
	if (arr.size() > 200) arr.erase(arr.begin(), arr.begin() + (arr.size() - 200));
	std::ofstream out(path); if (out) out << arr.dump(1);
}

void saveBestRun(const std::string& path, const BestRun& b)
{
	json j;
	j["score"] = b.score; j["timeSec"] = b.timeSec;
	j["traceX"] = b.traceX; j["traceZ"] = b.traceZ; j["off"] = b.off; j["boxTimeMs"] = b.boxTimeMs;
	std::ofstream o(path); if (o) o << j.dump();
}

bool loadBestRun(const std::string& path, BestRun& out)
{
	std::ifstream in(path); if (!in) return false;
	json j; try { in >> j; } catch (...) { return false; }
	if (!j.is_object()) return false;
	out.score = j.value("score", -1); out.timeSec = j.value("timeSec", 0);
	out.traceX = j.value("traceX", std::vector<float>{});
	out.traceZ = j.value("traceZ", std::vector<float>{});
	out.off = j.value("off", std::vector<unsigned char>{});
	out.boxTimeMs = j.value("boxTimeMs", std::vector<double>{});
	return out.score >= 0 && out.traceX.size() == out.traceZ.size() && !out.traceX.empty();
}

std::vector<RunRecord> loadRunHistory(const std::string& path)
{
	std::vector<RunRecord> v; json arr;
	std::ifstream in(path); if (!in) return v;
	try { in >> arr; } catch (...) { return v; }
	if (!arr.is_array()) return v;
	for (const auto& j : arr)
	{
		RunRecord r;
		r.track = j.value("track", std::string()); r.date = j.value("date", std::string());
		r.timeSec = j.value("timeSec", 0); r.distKm = j.value("distKm", 0.0);
		r.score = j.value("score", 0); r.grade = j.value("grade", std::string());
		v.push_back(r);
	}
	return v;
}

} // namespace model
