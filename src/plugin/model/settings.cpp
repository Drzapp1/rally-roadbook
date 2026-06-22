#include "settings.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace model {
namespace {
json modToJson(const ModuleCfg& m) { return json{ { "on", m.on }, { "scale", m.scale }, { "x", m.x }, { "y", m.y }, { "opacity", m.opacity } }; }
void modFromJson(const json& j, ModuleCfg& m)
{
	if (!j.is_object()) return;
	m.on      = j.value("on", m.on);
	m.scale   = j.value("scale", m.scale);
	m.x       = j.value("x", m.x);
	m.y       = j.value("y", m.y);
	m.opacity = j.value("opacity", m.opacity);
}
} // namespace

bool loadSettings(Settings& s, const std::string& path)
{
	std::ifstream f(path);
	if (!f) return false;
	json j;
	try { f >> j; } catch (...) { return false; }

	s.scale     = j.value("scale", s.scale);
	s.opacity   = j.value("opacity", s.opacity);
	s.offX      = j.value("offX", s.offX);
	s.offY      = j.value("offY", s.offY);
	s.challenge = j.value("challenge", s.challenge);
	s.audio = j.value("audio", s.audio);
	s.voice = j.value("voice", s.voice);
	s.paceSentences = j.value("paceSentences", s.paceSentences);
	s.deviceFrame = j.value("deviceFrame", s.deviceFrame);
	s.deviceModel = j.value("deviceModel", s.deviceModel);
	s.lcdColor = j.value("lcdColor", s.lcdColor);
	s.layoutPreset = j.value("layoutPreset", s.layoutPreset);
	s.maskBox = j.value("maskBox", s.maskBox);
	s.noCap = j.value("noCap", s.noCap);
	s.distanceOnly = j.value("distanceOnly", s.distanceOnly);
	s.detailDeg = j.value("detailDeg", s.detailDeg);
	s.offRouteToleranceM = j.value("offRouteToleranceM", s.offRouteToleranceM);
	s.styleIndex   = j.value("styleIndex", s.styleIndex);
	s.abbrevStyle  = j.value("abbrevStyle", s.abbrevStyle);
	s.advanceLookM = j.value("advanceLookM", s.advanceLookM);

	if (j.contains("mDriver"))   modFromJson(j["mDriver"], s.mDriver);
	if (j.contains("mDash"))     modFromJson(j["mDash"], s.mDash);
	if (j.contains("mStatus"))   modFromJson(j["mStatus"], s.mStatus);
	if (j.contains("mStrip"))    modFromJson(j["mStrip"], s.mStrip);
	if (j.contains("mCompass"))  modFromJson(j["mCompass"], s.mCompass);
	if (j.contains("mNext"))     modFromJson(j["mNext"], s.mNext);
	if (j.contains("mOffroute")) modFromJson(j["mOffroute"], s.mOffroute);
	if (j.contains("mMinimap"))  modFromJson(j["mMinimap"], s.mMinimap);
	if (j.contains("mProfile"))  modFromJson(j["mProfile"], s.mProfile);

	// validate / clamp so a hand-edited or stale settings file can't break the UI
	auto ci = [](int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); };
	auto cf = [](float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); };
	s.deviceModel  = ci(s.deviceModel, 0, 15);
	s.lcdColor     = ci(s.lcdColor, 0, 5);
	s.layoutPreset = ci(s.layoutPreset, 0, 2);
	s.styleIndex   = ((s.styleIndex % 4) + 4) % 4;
	s.opacity      = ci(s.opacity, 30, 255);
	s.scale        = cf(s.scale, 0.2f, 2.5f);
	s.offRouteToleranceM = ci(s.offRouteToleranceM, 5, 200);
	s.advanceLookM = ci(s.advanceLookM, 0, 60);
	return true;
}

bool saveSettings(const Settings& s, const std::string& path)
{
	json j = {
		{ "scale", s.scale }, { "opacity", s.opacity },
		{ "offX", s.offX }, { "offY", s.offY },
		{ "challenge", s.challenge }, { "audio", s.audio }, { "voice", s.voice }, { "paceSentences", s.paceSentences }, { "deviceFrame", s.deviceFrame },
		{ "deviceModel", s.deviceModel }, { "lcdColor", s.lcdColor }, { "layoutPreset", s.layoutPreset },
		{ "maskBox", s.maskBox }, { "noCap", s.noCap }, { "distanceOnly", s.distanceOnly },
		{ "detailDeg", s.detailDeg },
		{ "offRouteToleranceM", s.offRouteToleranceM },
		{ "styleIndex", s.styleIndex }, { "abbrevStyle", s.abbrevStyle }, { "advanceLookM", s.advanceLookM },
		{ "mDriver", modToJson(s.mDriver) }, { "mDash", modToJson(s.mDash) },
		{ "mStatus", modToJson(s.mStatus) }, { "mStrip", modToJson(s.mStrip) },
		{ "mCompass", modToJson(s.mCompass) }, { "mNext", modToJson(s.mNext) },
		{ "mOffroute", modToJson(s.mOffroute) }, { "mMinimap", modToJson(s.mMinimap) }, { "mProfile", modToJson(s.mProfile) },
	};
	std::ofstream f(path);
	if (!f) return false;
	f << j.dump(2);
	return true;
}

} // namespace model
