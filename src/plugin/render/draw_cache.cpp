#include "draw_cache.h"

#include <cmath>
#include <cstring>

namespace render {

void DrawCache::begin()
{
	quads_.resize(0);   // keep capacity — no per-frame reallocation
	strings_.resize(0);
}

void DrawCache::quad(const float pos[4][2], unsigned long color, int sprite)
{
	SPluginQuad_t q;
	for (int i = 0; i < 4; ++i)
	{
		q.m_aafPos[i][0] = (pos[i][0] - txPX_) * txS_ + txPX_ + txX_;
		q.m_aafPos[i][1] = (pos[i][1] - txPY_) * txS_ + txPY_ + txY_;
	}
	q.m_iSprite = sprite;
	q.m_ulColor = color;
	quads_.push_back(q);
}

void DrawCache::sprite(float x, float y, float w, float h, unsigned long color, int spriteIndex)
{
	// TL, BL, BR, TR — matches the engine's texture-corner mapping.
	const float p[4][2] = {
		{ x,     y     },
		{ x,     y + h },
		{ x + w, y + h },
		{ x + w, y     },
	};
	quad(p, color, spriteIndex);
}

void DrawCache::rect(float x, float y, float w, float h, unsigned long color, int sprite)
{
	// Counter-clockwise winding (in screen space, y-down). The game back-face
	// culls quads, so a clockwise rect is invisible — this order matches the
	// thin-quad line() below, which is known to render.
	const float p[4][2] = {
		{ x,     y + h },
		{ x + w, y + h },
		{ x + w, y     },
		{ x,     y     },
	};
	quad(p, color, sprite);
}

void DrawCache::line(float x1, float y1, float x2, float y2, float halfWidth, unsigned long color)
{
	float dx = x2 - x1, dy = y2 - y1;
	float len = std::sqrt(dx * dx + dy * dy);
	if (len < 1e-6f)
	{
		// Degenerate segment: draw a small square so it is still visible.
		rect(x1 - halfWidth, y1 - halfWidth, 2 * halfWidth, 2 * halfWidth, color);
		return;
	}
	// Perpendicular unit vector scaled to halfWidth.
	float px = -dy / len * halfWidth;
	float py =  dx / len * halfWidth;
	const float p[4][2] = {
		{ x1 + px, y1 + py },
		{ x2 + px, y2 + py },
		{ x2 - px, y2 - py },
		{ x1 - px, y1 - py },
	};
	quad(p, color);
}

void DrawCache::text(const char* s, float x, float y, float size, int justify, unsigned long color, int font)
{
	SPluginString_t str;
	std::memset(str.m_szString, 0, sizeof str.m_szString);
	if (s)
	{
		std::strncpy(str.m_szString, s, sizeof(str.m_szString) - 1);
		str.m_szString[sizeof(str.m_szString) - 1] = '\0';
	}
	str.m_afPos[0] = (x - txPX_) * txS_ + txPX_ + txX_;
	str.m_afPos[1] = (y - txPY_) * txS_ + txPY_ + txY_;
	str.m_iFont    = font;
	str.m_fSize    = size * txS_;
	str.m_iJustify = justify;
	str.m_ulColor  = color;
	strings_.push_back(str);
}

void DrawCache::commit(int* numQuads, void** quads, int* numStrings, void** strings)
{
	if (numQuads)   *numQuads   = static_cast<int>(quads_.size());
	if (quads)      *quads      = quads_.empty()   ? nullptr : quads_.data();
	if (numStrings) *numStrings = static_cast<int>(strings_.size());
	if (strings)    *strings    = strings_.empty() ? nullptr : strings_.data();
}

} // namespace render
