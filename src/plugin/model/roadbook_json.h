// ============================================================================
// roadbook_json.h  —  read/write a Roadbook as JSON (schemaVersion 1)
//
// One on-disk format for both auto-generated and hand-authored roadbooks.
// Tolerant reader: missing optional fields fall back to defaults.
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include "roadbook.h"

namespace model {

bool writeJson(const Roadbook& rb, const std::string& path);
bool readJson(const std::string& path, Roadbook& out);

// One completed run, persisted to history.json for the time-trial / history page.
struct RunRecord { std::string track, date, grade; int timeSec = 0; double distKm = 0.0; int score = 0; };
void appendRunRecord(const std::string& path, const RunRecord& r);
std::vector<RunRecord> loadRunHistory(const std::string& path);

// A saved "personal best" run for the cross-session ghost replay.
struct BestRun { int score = -1; int timeSec = 0; std::vector<float> traceX, traceZ; std::vector<unsigned char> off; std::vector<double> boxTimeMs; };
void saveBestRun(const std::string& path, const BestRun& b);
bool loadBestRun(const std::string& path, BestRun& out);

} // namespace model
