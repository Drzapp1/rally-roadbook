#include "input_manager.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace input {

void InputManager::watch(int vk)
{
	keys_.try_emplace(vk);
}

void InputManager::update()
{
	for (auto& [vk, k] : keys_)
	{
		k.prev = k.now;
		k.now  = (GetAsyncKeyState(vk) & 0x8000) != 0;
	}
}

bool InputManager::clicked(int vk) const
{
	auto it = keys_.find(vk);
	if (it == keys_.end()) return false;
	return it->second.now && !it->second.prev;
}

} // namespace input
