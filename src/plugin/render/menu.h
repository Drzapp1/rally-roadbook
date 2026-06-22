// ============================================================================
// menu.h  —  a small keyboard-driven immediate-mode menu
//
// The caller rebuilds a flat list of MenuItem each frame (cheap), then calls
// handleInput() and draw(). Items bind directly to bool/float/int values or an
// action closure, so pages are just different item lists.
// ============================================================================
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "draw_cache.h"
#include "../input/input_manager.h"

namespace render {

struct MenuItem
{
	enum Type { Toggle, SliderF, SliderI, Action, Header };
	Type type = Action;
	std::string label;

	bool*  boolVal = nullptr;                                  // Toggle
	float* fval = nullptr; float fmin = 0, fmax = 1, fstep = 0.1f; // SliderF
	int*   ival = nullptr; int imin = 0, imax = 100, istep = 1;    // SliderI
	const char* fmt = nullptr;                                 // value format override
	std::function<void()> action;                              // Action / on-change
};

class Menu
{
public:
	bool open = false;
	int  sel  = 0;

	void toggleOpen() { open = !open; sel = 0; }

	// Process navigation (Up/Down move, Left/Right adjust, Enter activate).
	void handleInput(input::InputManager& in, std::vector<MenuItem>& items);

	void draw(DrawCache& dc, int font, const char* title, const std::string& footer,
	          const std::vector<MenuItem>& items);
};

} // namespace render
