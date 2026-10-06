#include "pch.h"
#include "CoreUObject/UObjectSystem.h"
#include "FObjectItem.h"
#include <limits>

namespace {
    struct FObjectNameKey {
        const UObject* mOuter{};
        FName mName{};

        bool operator==(const FObjectNameKey& Other) const;
    };

    struct FObjectNameKeyHash {
        std::size_t operator()(const FObjectNameKey& Key) const;
    };

    struct FObjectRegistryState {
        std::vector<FObjectItem> mObjectItems{};
        TMap<FGuid, Uint32> mGuidToIndexMap{};
        std::unordered_map<FObjectNameKey, FObjectHandle, FObjectNameKeyHash> mNames{};
        std::vector<Uint32> mFreeIndices{};
        Uint32 mObjectCount{};
    };

    bool FObjectNameKey::operator==(const FObjectNameKey& Other) const {
        return mOuter == Other.mOuter && mName == Other.mName;
    }

    std::size_t FObjectNameKeyHash::operator()(const FObjectNameKey& Key) const {
        const std::size_t OuterHash{std::hash<const UObject*>{}(Key.mOuter)};
        const std::size_t NameHash{std::hash<FName>{}(Key.mName)};

        return OuterHash ^ (NameHash + 0x9e3779b97f4a7c15ULL + (OuterHash << 6) + (OuterHash >> 2));
    }

    FObjectRegistryState& GetRegistryState() {
        static FObjectRegistryState State{};

        return State;
    }
}

FObjectHandle UObjectSystem::Register(UObject* Object) {
    if (Object == nullptr || Object->mDestroying) {
        return {};
    }

    if (Resolve(Object->GetHandle()) == Object) {
        return Object->GetHandle();
    }

    if (FindHandleByGuid(Object->GetGuid()).IsValid()) {
        return {};
    }

    const FName Name{Object->GetName().IsNone() ? MakeUniqueObjectName(Object->GetOuter(), Object->GetTypeInfo()->mTypeName) : Object->GetName()};

    if (!IsValidObjectName(Name) || FindObject(Object->GetOuter(), Name) != nullptr) {
        return {};
    }

    FObjectRegistryState& State{GetRegistryState()};
    Uint32 Index{};

    if (!State.mFreeIndices.empty()) {
        Index = State.mFreeIndices.back();
        State.mFreeIndices.pop_back();
    } else {
        Index = static_cast<Uint32>(State.mObjectItems.size());
        State.mObjectItems.emplace_back();
    }

    FObjectItem& Item{State.mObjectItems[Index]};
    const FObjectHandle Handle{Index, Item.mGeneration};

    Item.mObject = Object;
    Object->mName = Name;
    Object->SetHandle(Handle);
    State.mGuidToIndexMap.emplace(Object->GetGuid(), Index);
    State.mNames.emplace(FObjectNameKey{Object->GetOuter(), Name}, Handle);
    ++State.mObjectCount;

    return Handle;
}

FObjectHandle UObjectSystem::RegisterWithGuid(UObject* Object, const FGuid& InGuid) {
    if (Object == nullptr) {
        return {};
    }

    const FGuid PreviousGuid{Object->GetGuid()};

    if (!Object->RestoreGuid(InGuid)) {
        return {};
    }

    const FObjectHandle Handle{Register(Object)};

    if (!Handle.IsValid()) {
        Object->RestoreGuid(PreviousGuid);
    }

    return Handle;
}

bool UObjectSystem::TryGet(Uint32 Index, FObjectHandle& Out) {
    FObjectRegistryState& State{GetRegistryState()};

    if (Index >= State.mObjectItems.size() || State.mObjectItems[Index].mObject == nullptr) {
        Out = {};
        return false;
    }

    Out = FObjectHandle{Index, State.mObjectItems[Index].mGeneration};

    return true;
}

void UObjectSystem::Unregister(UObject* Object, FObjectHandle Handle) {
    if (Object == nullptr || Resolve(Handle) != Object) {
        return;
    }

    FObjectRegistryState& State{GetRegistryState()};
    FObjectItem& Item{State.mObjectItems[Handle.mIndex]};

    State.mNames.erase(FObjectNameKey{Object->GetOuter(), Object->GetName()});
    State.mGuidToIndexMap.erase(Object->GetGuid());
    Item.mObject = nullptr;
    ++Item.mGeneration;

    if (Item.mGeneration == 0) {
        ++Item.mGeneration;
    }

    Object->SetHandle({});
    State.mFreeIndices.push_back(Handle.mIndex);
    --State.mObjectCount;
}

UObject* UObjectSystem::Resolve(FObjectHandle Handle) {
    if (!Handle.IsValid()) {
        return nullptr;
    }

    FObjectRegistryState& State{GetRegistryState()};

    if (Handle.mIndex >= State.mObjectItems.size()) {
        return nullptr;
    }

    const FObjectItem& Item{State.mObjectItems[Handle.mIndex]};

    return Item.mGeneration == Handle.mGeneration ? Item.mObject : nullptr;
}

FObjectHandle UObjectSystem::FindHandleByGuid(const FGuid& Guid) {
    FObjectRegistryState& State{GetRegistryState()};
    const auto Iterator{State.mGuidToIndexMap.find(Guid)};

    if (Iterator == State.mGuidToIndexMap.end()) {
        return {};
    }

    const Uint32 Index{Iterator->second};

    return FObjectHandle{Index, State.mObjectItems[Index].mGeneration};
}

FObjectHandle UObjectSystem::GetHandle(const UObject* Object) {
    return Object != nullptr ? Object->GetHandle() : FObjectHandle{};
}

UObject* UObjectSystem::FindObject(const UObject* Outer, FName Name) {
    const FObjectRegistryState& State{GetRegistryState()};
    const auto Iterator{State.mNames.find(FObjectNameKey{Outer, Name})};

    return Iterator != State.mNames.end() ? Resolve(Iterator->second) : nullptr;
}

UObject* UObjectSystem::FindObjectByPath(std::string_view Path, const UObject* Outer) {
    UObject* Object{};

    while (!Path.empty()) {
        const std::size_t Separator{Path.find('.')};
        const std::string_view Segment{Path.substr(0, Separator)};

        if (Segment.empty() || Segment.size() >= NameSize) {
            return nullptr;
        }

        Object = FindObject(Outer, FName{Segment});

        if (Object == nullptr || Separator == std::string_view::npos) {
            return Object;
        }

        Outer = Object;
        Path.remove_prefix(Separator + 1);
    }

    return nullptr;
}

TArray<UObject*> UObjectSystem::GetObjectsWithOuter(const UObject* Outer, bool IncludeNested) {
    TArray<UObject*> Objects{};

    for (const FObjectItem& Item : GetRegistryState().mObjectItems) {
        UObject* Object{Item.mObject};

        if (Object != nullptr && (Object->GetOuter() == Outer || (IncludeNested && Outer != nullptr && Object->IsIn(Outer)))) {
            Objects.push_back(Object);
        }
    }

    return Objects;
}

FName UObjectSystem::MakeUniqueObjectName(const UObject* Outer, FName BaseName, const UObject* IgnoreObject) {
    if (BaseName.IsNone()) {
        return {};
    }

    UObject* Existing{FindObject(Outer, BaseName)};

    if (Existing == nullptr || Existing == IgnoreObject) {
        return BaseName;
    }

    const Int32 FirstNumber{BaseName.GetNumber() > 0 ? BaseName.GetNumber() : 1};

    for (Int32 Number{FirstNumber}; Number < std::numeric_limits<Int32>::max(); ++Number) {
        const FName Candidate{BaseName.WithNumber(Number)};

        Existing = FindObject(Outer, Candidate);

        if (Existing == nullptr || Existing == IgnoreObject) {
            return Candidate;
        }
    }

    return {};
}

bool UObjectSystem::IsValidObjectName(FName Name) {
    const FString String{Name.ToString()};

    return !String.empty() && String.find_first_of(".:/\\") == FString::npos && String.find('\0') == FString::npos;
}

bool UObjectSystem::Rename(UObject* Object, FName NewName, UObject* NewOuter) {
    if (Object == nullptr || Object->mDestroying || (NewOuter != nullptr && (NewOuter->mDestroying || NewOuter == Object || NewOuter->IsIn(Object))) || !Object->CanChangeOuter(NewOuter)) {
        return false;
    }

    const bool Registered{Resolve(Object->GetHandle()) == Object};

    if (NewName.IsNone() && Registered) {
        NewName = MakeUniqueObjectName(NewOuter, Object->GetTypeInfo()->mTypeName, Object);
    }

    if (!NewName.IsNone() && !IsValidObjectName(NewName)) {
        return false;
    }

    UObject* Existing{FindObject(NewOuter, NewName)};

    if (Registered && Existing != nullptr && Existing != Object) {
        return false;
    }

    const bool Changed{Object->mOuter != NewOuter || Object->mName.GetDisplayId() != NewName.GetDisplayId() || Object->mName.GetNumber() != NewName.GetNumber()};

    if (!Changed) {
        return true;
    }

    if (Object->mOuter != NewOuter && NewOuter != nullptr) {
        NewOuter->mInnerObjects.reserve(NewOuter->mInnerObjects.size() + 1);
    }

    if (Registered && (Object->mOuter != NewOuter || Object->mName != NewName)) {
        GetRegistryState().mNames.emplace(FObjectNameKey{NewOuter, NewName}, Object->GetHandle());
        GetRegistryState().mNames.erase(FObjectNameKey{Object->mOuter, Object->mName});
    }

    if (Object->mOuter != NewOuter) {
        if (Object->mOuter != nullptr) {
            std::erase(Object->mOuter->mInnerObjects, Object);
        }

        if (NewOuter != nullptr) {
            NewOuter->mInnerObjects.push_back(Object);
        }
    }

    Object->mName = NewName;
    Object->mOuter = NewOuter;
    Object->OnIdentityChanged();

    return true;
}

bool UObjectSystem::RestoreGuid(UObject* Object, const FGuid& Guid) {
    if (Object == nullptr || !Guid.IsValid()) {
        return false;
    }

    const FObjectHandle Existing{FindHandleByGuid(Guid)};

    if (Resolve(Object->GetHandle()) == Object && Existing.IsValid() && Resolve(Existing) != Object) {
        return false;
    }

    if (Resolve(Object->GetHandle()) == Object) {
        FObjectRegistryState& State{GetRegistryState()};

        State.mGuidToIndexMap.erase(Object->mGuid);
        State.mGuidToIndexMap.emplace(Guid, Object->GetHandle().mIndex);
    }

    Object->mGuid = Guid;

    return true;
}

void UObjectSystem::DetachInners(UObject* Object) {
    FObjectRegistryState& State{GetRegistryState()};

    while (!Object->mInnerObjects.empty()) {
        UObject* Inner{Object->mInnerObjects.back()};

        Object->mInnerObjects.pop_back();

        const bool Registered{Resolve(Inner->GetHandle()) == Inner};
        const FName Name{Registered ? MakeUniqueObjectName(nullptr, Inner->GetName(), Inner) : Inner->GetName()};

        if (Registered) {
            State.mNames.erase(FObjectNameKey{Object, Inner->mName});
            State.mNames.emplace(FObjectNameKey{nullptr, Name}, Inner->GetHandle());
        }

        Inner->mName = Name;
        Inner->mOuter = nullptr;
    }

    if (Object->mOuter != nullptr) {
        std::erase(Object->mOuter->mInnerObjects, Object);
        Object->mOuter = nullptr;
    }
}

Uint32 UObjectSystem::GetItemCount() {
    return static_cast<Uint32>(GetRegistryState().mObjectItems.size());
}

Uint32 UObjectSystem::GetObjectCount() {
    return GetRegistryState().mObjectCount;
}
