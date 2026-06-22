// ============================================================================
// log.h  —  minimal file logger
//
// The plugin runs inside the game with no console, so a flushed-per-line file
// log is the primary debugging instrument. Writes to
// <savePath>\Roadbook\log.txt. Every export should log enough to reconstruct
// what happened before a crash.
// ============================================================================
#pragma once

namespace rblog {

// Open <savePath>\Roadbook\log.txt for writing (truncates). Safe to call once
// from Startup; creates the Roadbook subfolder if needed.
void init(const char* savePath);

// printf-style line logger. A newline is appended automatically and the file
// is flushed, so the last line survives a hard crash.
void line(const char* fmt, ...);

// Close the log file.
void shutdown();

} // namespace rblog
