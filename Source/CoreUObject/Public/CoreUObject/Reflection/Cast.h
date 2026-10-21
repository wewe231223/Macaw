#pragma once

#include "CoreUObject/UObject.h"

#include <type_traits>

class UObject;

template <typename T>
T* Cast(UObject* Object);

template <typename T>
const T* Cast(const UObject* Object);

#include "CoreUObject/Reflection/CastTemplates.h"
