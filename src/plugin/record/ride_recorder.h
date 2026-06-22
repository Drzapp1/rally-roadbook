// ============================================================================
// ride_recorder.h  —  logs a ride to CSV for offline tulip generation
//
// Writes one row per telemetry tick to
//   <savePath>\Roadbook\rides\ride_<trackId>_<timestamp>.csv
// Columns: t,x,y,z,yaw,vx,vy,vz,speed,pos,dist
// where (x,z) is the horizontal ground plane, y is elevation, yaw is the
// compass heading (0=N,90=E), and dist is the gated speed-integrated odometer.
// ============================================================================
#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include "../api/mxb_api.h"
#include "../generate/tulip_generator.h"

namespace record {

class RideRecorder
{
public:
	void setSavePath(const char* savePath);
	void setEvent(const char* trackId, const char* trackName, float trackLength);

	// Open a new ride file (no-op if already recording). Resets the odometer.
	void start();
	// Append one telemetry sample.
	void onTelemetry(const SPluginsBikeData_t& b, float time, float pos);
	// Flush to disk without closing (call on pause).
	void flush();
	// Flush and close.
	void stop();

	bool        recording() const { return file_ != nullptr; }
	long long   rows()      const { return rows_; }
	double      distance()  const { return dist_; }
	const std::string& path() const { return path_; }

	// In-memory samples for in-plugin roadbook generation.
	const std::vector<gen::RideSample>& samples() const { return samples_; }

private:
	std::string savePath_;
	std::string trackId_   = "unknown";
	std::string trackName_;
	float       trackLength_ = 0.0f;

	std::FILE*  file_ = nullptr;
	std::string path_;
	long long   rows_ = 0;
	std::vector<gen::RideSample> samples_;

	// odometer state
	bool   haveLast_ = false;
	float  lastTime_ = 0.0f;
	double dist_     = 0.0;
};

} // namespace record
