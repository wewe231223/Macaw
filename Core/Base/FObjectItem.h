#pragma once

#include <cstdint>

class UObject;

struct FObjectItem
{
	UObject* Object = nullptr;
	std::uint32_t Generation = 1;
};
