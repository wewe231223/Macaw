#include "pch.h"
#include "World/ULevel.h"
#include "World/AActor.h"

ULevel::ULevel(UWorld& World)
	: mWorld{World} {
}

ULevel::~ULevel() = default;

UWorld& ULevel::GetWorld() const {
    return mWorld;
}

const TArray<std::unique_ptr<AActor>>& ULevel::GetActors() const {
    return mActors;
}

const FTypeInfo* ULevel::StaticTypeInfo() noexcept {
    static const FTypeInfo Information{"ULevel", UObject::StaticTypeInfo(), nullptr};
    return &Information;
}

const FTypeInfo* ULevel::GetTypeInfo() const noexcept {
    return StaticTypeInfo();
}
