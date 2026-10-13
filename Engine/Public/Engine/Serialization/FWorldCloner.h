#pragma once

#include "World/EWorldType.h"
#include <memory>

class UWorld;

class FWorldCloner final {
public:
    static std::unique_ptr<UWorld> Clone(UWorld& SourceWorld, EWorldType WorldType = EWorldType::Game);
};
