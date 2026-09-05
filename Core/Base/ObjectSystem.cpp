#include "PCH.h"
#include "ObjectSystem.h"

#include "FGuid.h"
#include "FObjectItem.h"
#include "UObject.h"

namespace
{
    struct FObjectRegistryState
    {
        std::vector<FObjectItem> ObjectItems;
        std::vector<std::uint32_t> FreeIndices;
        std::uint32_t ObjectCount = 0;
    };

    FObjectRegistryState& GetRegistryState()
    {
        static FObjectRegistryState State;
        return State;
    }
}

FObjectHandle ObjectSystem::Register(UObject* Object)
{
    if (Object == nullptr)
    {
        return {};
    }

    FObjectRegistryState& State = GetRegistryState();

    std::uint32_t Index;

    if (!State.FreeIndices.empty())
    {
        Index = State.FreeIndices.back();
        State.FreeIndices.pop_back();
    }
    else
    {
        Index = static_cast<std::uint32_t>(
            State.ObjectItems.size());

        State.ObjectItems.emplace_back();
    }

    FObjectItem& Item = State.ObjectItems[Index];
    Item.Object = Object;

    const FObjectHandle Handle
    {
        Index,
        Item.Generation
    };

    Object->SetHandle(Handle);

    ++State.ObjectCount;

    return Handle;
}

void ObjectSystem::Unregister(
    UObject* Object,
    FObjectHandle Handle)
{
    if (Object == nullptr)
    {
        return;
    }

    FObjectRegistryState& State = GetRegistryState();

    if (Handle.Index >= State.ObjectItems.size())
    {
        return;
    }

    FObjectItem& Item = State.ObjectItems[Handle.Index];

    if (Item.Object != Object)
    {
        return;
    }

    if (Item.Generation != Handle.Generation)
    {
        return;
    }

    Item.Object = nullptr;

    ++Item.Generation;

    // Generation 0은 Invalid Handle 용도로 비워 둔다.
    if (Item.Generation == 0)
    {
        ++Item.Generation;
    }

    State.FreeIndices.push_back(Handle.Index);

    --State.ObjectCount;
}

UObject* ObjectSystem::Resolve(FObjectHandle Handle)
{
    if (!Handle.IsValid())
    {
        return nullptr;
    }

    FObjectRegistryState& State = GetRegistryState();

    if (Handle.Index >= State.ObjectItems.size())
    {
        return nullptr;
    }

    FObjectItem& Item = State.ObjectItems[Handle.Index];

    if (Item.Object == nullptr)
    {
        return nullptr;
    }

    if (Item.Generation != Handle.Generation)
    {
        return nullptr;
    }

    return Item.Object;
}

FObjectHandle ObjectSystem::FindHandleByGuid(
    const FGuid& Guid)
{
    FObjectRegistryState& State = GetRegistryState();

    for (std::uint32_t Index = 0;
        Index < State.ObjectItems.size();
        ++Index)
    {
        const FObjectItem& Item = State.ObjectItems[Index];

        if (Item.Object == nullptr)
        {
            continue;
        }

        if (Item.Object->GetGuid() == Guid)
        {
            return FObjectHandle
            {
                Index,
                Item.Generation
            };
        }
    }

    return {};
}

FObjectHandle ObjectSystem::GetHandle(
    const UObject* Object)
{
    if (Object == nullptr)
    {
        return {};
    }

    return Object->GetHandle();
}

std::uint32_t ObjectSystem::GetObjectCount()
{
    return GetRegistryState().ObjectCount;
}