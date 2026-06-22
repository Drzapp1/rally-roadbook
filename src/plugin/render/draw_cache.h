// ============================================================================
// draw_cache.h  —  owns the quad/string buffers returned from Draw
//
// Draw is a hot path (called every frame, up to 240 fps). The buffers are
// member vectors that we clear with resize(0) (keeping capacity) and refill,
// so steady-state frames perform zero heap allocations. commit() hands the
// game stable pointers that stay valid until the next begin().
//
// Coordinate space throughout is normalized screen: (0,0) top-left,
// (1,1) bottom-right.
// ============================================================================
#pragma once

#include <vector>
#include "../api/mxb_api.h"

namespace render {

class DrawCache
{
public:
	// Start a fresh frame (clears counts, keeps capacity).
	void begin();

	// Axis-aligned rectangle. sprite 0 = solid fill; >0 = textured.
	void rect(float x, float y, float w, float h, unsigned long color, int sprite = 0);

	// Arbitrary quad with 4 explicit corners.
	void quad(const float pos[4][2], unsigned long color, int sprite = 0);

	// Textured sprite quad (corner order TL,BL,BR,TR to match the engine's UV
	// mapping). spriteIndex is 1-based (from DrawInit); color tints the sprite.
	void sprite(float x, float y, float w, float h, unsigned long color, int spriteIndex);

	// A line segment drawn as a thin quad of total width 2*halfWidth.
	void line(float x1, float y1, float x2, float y2, float halfWidth, unsigned long color);

	// A text string (truncated to fit the API's 100-char field, NUL-terminated).
	void text(const char* s, float x, float y, float size, int justify, unsigned long color, int font);

	// Publish the buffers to the game's out-params.
	void commit(int* numQuads, void** quads, int* numStrings, void** strings);

	// Apply a scale (about a pivot) + offset to everything added until reset.
	// out = (base - pivot)*scale + pivot + offset; text size *= scale.
	// A per-module pivot lets each panel scale in place instead of toward 0,0.
	void setTransform(float scale, float offX, float offY, float pivotX = 0.0f, float pivotY = 0.0f)
	{ txS_ = scale; txX_ = offX; txY_ = offY; txPX_ = pivotX; txPY_ = pivotY; }
	void resetTransform() { txS_ = 1.0f; txX_ = 0.0f; txY_ = 0.0f; txPX_ = 0.0f; txPY_ = 0.0f; }

private:
	std::vector<SPluginQuad_t>   quads_;
	std::vector<SPluginString_t> strings_;
	float txS_ = 1.0f, txX_ = 0.0f, txY_ = 0.0f, txPX_ = 0.0f, txPY_ = 0.0f;
};

} // namespace render
