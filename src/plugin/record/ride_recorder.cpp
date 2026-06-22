#include "ride_recorder.h"

#include <algorithm>
#include <cmath>
#include <ctime>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../util/log.h"

namespace record {
namespace {

constexpr float kMinMoveSpeed = 0.1f; // m/s
constexpr float kMaxDt        = 0.5f; // s

// Replace characters that are awkward in a filename.
std::string sanitize(const std::string& s)
{
	std::string out = s;
	for (char& c : out)
	{
		if (c == ' ' || c == '\\' || c == '/' || c == ':' || c == '*' ||
		    c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
			c = '_';
	}
	if (out.empty()) out = "unknown";
	return out;
}

std::string timestamp()
{
	std::time_t t = std::time(nullptr);
	std::tm tm{};
	localtime_s(&tm, &t);
	char buf[32];
	std::strftime(buf, sizeof buf, "%Y%m%d-%H%M%S", &tm);
	return buf;
}

} // namespace

void RideRecorder::setSavePath(const char* savePath)
{
	savePath_ = (savePath && *savePath) ? savePath : ".";
}

void RideRecorder::setEvent(const char* trackId, const char* trackName, float trackLength)
{
	trackId_     = (trackId   && *trackId)   ? trackId   : "unknown";
	trackName_   = (trackName && *trackName) ? trackName : "";
	trackLength_ = trackLength;
}

void RideRecorder::start()
{
	if (file_) return; // already recording

	std::string dir = savePath_;
	if (!dir.empty() && dir.back() != '\\' && dir.back() != '/') dir += '\\';
	dir += "Roadbook";
	CreateDirectoryA(dir.c_str(), nullptr);
	dir += "\\rides";
	CreateDirectoryA(dir.c_str(), nullptr);

	path_ = dir + "\\ride_" + sanitize(trackId_) + "_" + timestamp() + ".csv";
	fopen_s(&file_, path_.c_str(), "w");
	if (!file_)
	{
		rblog::line("RideRecorder: failed to open %s", path_.c_str());
		return;
	}

	rows_ = 0;
	dist_ = 0.0;
	haveLast_ = false;
	samples_.clear();

	// Header: a comment line with track metadata, then the column header.
	std::fprintf(file_, "# track=%s id=%s length=%.1f\n",
	             trackName_.c_str(), trackId_.c_str(), trackLength_);
	std::fprintf(file_, "t,x,y,z,yaw,vx,vy,vz,speed,pos,dist\n");
	std::fflush(file_);
	rblog::line("RideRecorder: recording to %s", path_.c_str());
}

void RideRecorder::onTelemetry(const SPluginsBikeData_t& b, float time, float pos)
{
	if (!file_) return;

	// Gated odometer using horizontal speed (ground plane = x,z).
	if (haveLast_)
	{
		float dt = time - lastTime_;
		if (dt > 0.0f && dt <= kMaxDt)
		{
			float horiz = std::sqrt(b.m_fVelocityX * b.m_fVelocityX + b.m_fVelocityZ * b.m_fVelocityZ);
			float v = std::max(b.m_fSpeedometer, horiz);
			if (v >= kMinMoveSpeed) dist_ += static_cast<double>(v) * dt;
		}
	}
	lastTime_ = time;
	haveLast_ = true;

	std::fprintf(file_, "%.3f,%.3f,%.3f,%.3f,%.2f,%.3f,%.3f,%.3f,%.3f,%.5f,%.3f\n",
	             time, b.m_fPosX, b.m_fPosY, b.m_fPosZ, b.m_fYaw,
	             b.m_fVelocityX, b.m_fVelocityY, b.m_fVelocityZ,
	             b.m_fSpeedometer, pos, dist_);

	if ((++rows_ % 256) == 0) std::fflush(file_);

	gen::RideSample s;
	s.x = b.m_fPosX; s.z = b.m_fPosZ; s.y = b.m_fPosY; s.yaw = b.m_fYaw;
	s.speed = b.m_fSpeedometer; s.dist = static_cast<float>(dist_);
	samples_.push_back(s);
}

void RideRecorder::flush()
{
	if (file_) std::fflush(file_);
}

void RideRecorder::stop()
{
	if (!file_) return;
	std::fflush(file_);
	std::fclose(file_);
	file_ = nullptr;
	rblog::line("RideRecorder: stopped, %lld rows, %.1f m -> %s", rows_, dist_, path_.c_str());
}

} // namespace record
