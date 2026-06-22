// ============================================================================
// input_manager.h  —  global hotkey edge detection
//
// The PiBoSo API has no input callback, so plugins poll Win32 GetAsyncKeyState
// once per frame (from Draw) and edge-detect presses. `clicked(vk)` is true on
// the frame a key transitions from up to down — fire-once semantics for
// toggles/resets.
// ============================================================================
#pragma once

#include <unordered_map>

namespace input {

class InputManager
{
public:
	// Register a virtual-key code to track (e.g. VK_F8). Idempotent.
	void watch(int vk);

	// Poll all watched keys. Call once at the top of Draw.
	void update();

	// True only on the frame `vk` went from released to pressed.
	bool clicked(int vk) const;

private:
	struct Key { bool now = false, prev = false; };
	std::unordered_map<int, Key> keys_;
};

} // namespace input
