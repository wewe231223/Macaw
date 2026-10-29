#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"

#include <array>
#include <optional>

struct FShadowAtlasBlock {
public:
    ReflectTypes(FShadowAtlasBlock);

public:
    ReflectStruct(FShadowAtlasBlock);

private:
    ReflectMembers();

public:
    Uint32 mX{};
    ReflectProperty(mX, "X", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mY{};
    ReflectProperty(mY, "Y", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FShadowAtlasAllocator {
public:
    ReflectTypes(FShadowAtlasAllocator);

private:
    struct FNode {
    public:
        ReflectTypes(FNode);

    public:
        ReflectStruct(FNode);

    private:
        ReflectMembers();

    public:
        FShadowAtlasBlock mBlock{};
        ReflectProperty(mBlock, "Block", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        std::array<Uint32, 4> mChildren{};
        ReflectProperty(mChildren, "Children", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        bool mSplit{};
        ReflectProperty(mSplit, "Split", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        bool mOccupied{};
        ReflectProperty(mOccupied, "Occupied", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    };

public:
    explicit FShadowAtlasAllocator(Uint32 Size = 16384);

public:
    ReflectStruct(FShadowAtlasAllocator);

    void Reset();
    std::optional<FShadowAtlasBlock> Allocate(Uint32 Size);

private:
    ReflectMembers();

    std::optional<FShadowAtlasBlock> AllocateNode(Uint32 Index, Uint32 Size);

private:
    Uint32 mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FNode> mNodes{};
    ReflectProperty(mNodes, "Nodes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
