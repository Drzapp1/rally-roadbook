// ============================================================================
// api_sizes.h  —  runtime struct-size validation helpers
//
// The game passes raw buffers with a byte size. Struct layouts can drift
// across MX Bikes versions, so before reinterpreting any buffer we require
// `size >= sizeof(struct we compiled against)`. The engine may send a NEWER,
// LARGER struct — that is fine, we read the known prefix — so we check `>=`,
// never `==`.
// ============================================================================
#pragma once

#include "mxb_api.h"

namespace apisz {

inline constexpr int kBikeEvent   = static_cast<int>(sizeof(SPluginsBikeEvent_t));
inline constexpr int kBikeSession = static_cast<int>(sizeof(SPluginsBikeSession_t));
inline constexpr int kBikeData    = static_cast<int>(sizeof(SPluginsBikeData_t));
inline constexpr int kBikeLap     = static_cast<int>(sizeof(SPluginsBikeLap_t));
inline constexpr int kBikeSplit   = static_cast<int>(sizeof(SPluginsBikeSplit_t));

// True when a buffer of `size` bytes is at least as large as `expected`.
inline bool ok(int size, int expected) { return size >= expected; }

} // namespace apisz
