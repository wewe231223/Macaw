#pragma once

#include <cstddef>
#include <limits>
#include "World/Tick/ETickType.h"

class UWorld;

class FTickFunction {
public:
    FTickFunction() = default;
    virtual ~FTickFunction() = default;
    FTickFunction(const FTickFunction&) = delete;
    FTickFunction& operator=(const FTickFunction&) = delete;
    FTickFunction(FTickFunction&&) = delete;
    FTickFunction& operator=(FTickFunction&&) = delete;

public:
    bool CanEverTick() const;
    void SetCanEverTick(bool CanEverTick);
    bool IsTickEnabled() const;
    void SetTickEnabled(bool TickEnabled);
    bool IsTickInEditor() const;
    void SetTickInEditor(bool TickInEditor);
    bool IsTickWhenPaused() const;
    void SetTickWhenPaused(bool TickWhenPaused);
    bool ShouldTick(ETickType TickType, bool HasBegunPlay) const;
    virtual bool CanTick(ETickType TickType) const = 0;
    virtual void ExecuteTick(float DeltaTime) = 0;

private:
    friend class UWorld;

    bool mCanEverTick{};
    bool mTickEnabled{};
    bool mTickInEditor{};
    bool mTickWhenPaused{};
    std::size_t mTickIndex{std::numeric_limits<std::size_t>::max()};
};
