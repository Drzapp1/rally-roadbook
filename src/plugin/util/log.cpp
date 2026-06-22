#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace rblog {
namespace {
std::FILE* g_file = nullptr;
std::mutex g_mutex;

void timestamp(char* buf, size_t n)
{
	std::time_t t = std::time(nullptr);
	std::tm tm{};
	localtime_s(&tm, &t);
	std::strftime(buf, n, "%H:%M:%S", &tm);
}
} // namespace

void init(const char* savePath)
{
	std::lock_guard<std::mutex> lk(g_mutex);
	if (g_file) { std::fclose(g_file); g_file = nullptr; }

	std::string dir = (savePath && *savePath) ? savePath : ".";
	dir += "\\Roadbook";
	CreateDirectoryA(dir.c_str(), nullptr); // ok if it already exists

	std::string path = dir + "\\log.txt";
	fopen_s(&g_file, path.c_str(), "w");
	if (g_file)
	{
		char ts[16]; timestamp(ts, sizeof ts);
		std::fprintf(g_file, "[%s] log opened: %s\n", ts, path.c_str());
		std::fflush(g_file);
	}
}

void line(const char* fmt, ...)
{
	std::lock_guard<std::mutex> lk(g_mutex);
	if (!g_file) return;

	char ts[16]; timestamp(ts, sizeof ts);
	std::fprintf(g_file, "[%s] ", ts);

	va_list args;
	va_start(args, fmt);
	std::vfprintf(g_file, fmt, args);
	va_end(args);

	std::fputc('\n', g_file);
	std::fflush(g_file);
}

void shutdown()
{
	std::lock_guard<std::mutex> lk(g_mutex);
	if (g_file) { std::fclose(g_file); g_file = nullptr; }
}

} // namespace rblog
