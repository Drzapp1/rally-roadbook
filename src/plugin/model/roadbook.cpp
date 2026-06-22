#include "roadbook.h"

#include <algorithm>

namespace model {

const char* toString(BoxType t)
{
	switch (t)
	{
	case BoxType::Straight: return "straight";
	case BoxType::Turn:     return "turn";
	case BoxType::Hairpin:  return "hairpin";
	case BoxType::Start:    return "start";
	case BoxType::Finish:   return "finish";
	}
	return "turn";
}

const char* toString(TurnDir d)
{
	switch (d)
	{
	case TurnDir::None:  return "none";
	case TurnDir::Left:  return "left";
	case TurnDir::Right: return "right";
	}
	return "none";
}

bool hasPictogram(const Box& b, const char* code)
{
	for (const auto& p : b.pictograms) if (p == code) return true;
	return false;
}

void togglePictogram(Box& b, const char* code)
{
	auto it = std::find(b.pictograms.begin(), b.pictograms.end(), std::string(code));
	if (it != b.pictograms.end()) b.pictograms.erase(it);
	else b.pictograms.push_back(code);
}

} // namespace model
