#include "menu.h"

#include <algorithm>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "primitives.h"

namespace render {

void Menu::handleInput(input::InputManager& in, std::vector<MenuItem>& items)
{
	const int n = static_cast<int>(items.size());
	if (n == 0) return;
	auto selectable = [&](int i) { return items[i].type != MenuItem::Header; };

	if (sel < 0) sel = 0;
	if (sel >= n) sel = n - 1;
	for (int g = 0; g < n && !selectable(sel); ++g) sel = (sel + 1) % n; // bounded: never hang

	if (in.clicked(VK_DOWN)) { int i = sel; do { i = (i + 1) % n; } while (!selectable(i) && i != sel); sel = i; }
	if (in.clicked(VK_UP))   { int i = sel; do { i = (i - 1 + n) % n; } while (!selectable(i) && i != sel); sel = i; }

	MenuItem& it = items[sel];
	bool left = in.clicked(VK_LEFT), right = in.clicked(VK_RIGHT), enter = in.clicked(VK_RETURN);

	switch (it.type)
	{
	case MenuItem::Toggle:
		if ((left || right || enter) && it.boolVal) { *it.boolVal = !*it.boolVal; if (it.action) it.action(); }
		break;
	case MenuItem::SliderF:
		if (it.fval && (left || right))
		{
			*it.fval = std::max(it.fmin, std::min(it.fmax, *it.fval + (right ? it.fstep : -it.fstep)));
			if (it.action) it.action();
		}
		break;
	case MenuItem::SliderI:
		if (it.ival && (left || right))
		{
			*it.ival = std::max(it.imin, std::min(it.imax, *it.ival + (right ? it.istep : -it.istep)));
			if (it.action) it.action();
		}
		break;
	case MenuItem::Action:
		if (enter && it.action) it.action();
		break;
	default: break;
	}
}

void Menu::draw(DrawCache& dc, int font, const char* title, const std::string& footer,
                const std::vector<MenuItem>& items)
{
	const float x = 0.345f, w = 0.31f, y = 0.14f, rowH = 0.0335f;
	const int n = static_cast<int>(items.size());
	const int MAXVIS = 15;
	int start = (n > MAXVIS) ? std::max(0, std::min(sel - MAXVIS / 2, n - MAXVIS)) : 0;
	int end = std::min(n, start + MAXVIS);
	const float h = 0.072f + (end - start) * rowH + 0.045f;
	dc.rect(x, y, w, h, abgr(10, 12, 16, 240));
	dc.line(x, y, x + w, y, 0.0016f, col::accent);
	dc.line(x, y + h, x + w, y + h, 0.0016f, col::accent);
	dc.line(x, y, x, y + h, 0.0016f, col::accent);
	dc.line(x + w, y, x + w, y + h, 0.0016f, col::accent);

	dc.text(title, x + w * 0.5f, y + 0.030f, 0.023f, 1, col::accent, font);

	float yy = y + 0.066f;
	char buf[96];
	for (int i = start; i < end; ++i)
	{
		const MenuItem& it = items[i];
		bool selrow = (i == sel);

		if (it.type == MenuItem::Header)
		{
			dc.text(it.label.c_str(), x + 0.012f, yy + 0.020f, 0.014f, 0, col::dim, font);
			yy += rowH; continue;
		}
		if (selrow) dc.rect(x + 0.006f, yy + 0.016f, w - 0.012f, 0.027f, abgr(42, 50, 64, 210));

		unsigned long c = selrow ? col::accent : col::white;
		std::string lab = (selrow ? "> " : "  ") + it.label;
		dc.text(lab.c_str(), x + 0.012f, yy + 0.022f, 0.0165f, 0, c, font);

		if (it.type == MenuItem::Toggle && it.boolVal)
			dc.text(*it.boolVal ? "ON" : "OFF", x + w - 0.014f, yy + 0.022f, 0.0165f, 2, *it.boolVal ? col::good : col::dim, font);
		else if (it.type == MenuItem::SliderF && it.fval)
		{ std::snprintf(buf, sizeof buf, it.fmt ? it.fmt : "%.2f", *it.fval); dc.text(buf, x + w - 0.014f, yy + 0.022f, 0.0165f, 2, col::good, font); }
		else if (it.type == MenuItem::SliderI && it.ival)
		{ std::snprintf(buf, sizeof buf, it.fmt ? it.fmt : "%d", *it.ival); dc.text(buf, x + w - 0.014f, yy + 0.022f, 0.0165f, 2, col::good, font); }

		yy += rowH;
	}

	if (start > 0) dc.text("^ more", x + w * 0.5f, y + 0.052f, 0.012f, 1, col::dim, font);
	if (end < n)   dc.text("v more", x + w * 0.5f, y + h - 0.030f, 0.012f, 1, col::dim, font);
	dc.text(footer.c_str(), x + w * 0.5f, y + h - 0.016f, 0.0125f, 1, col::dim, font);
}

} // namespace render
