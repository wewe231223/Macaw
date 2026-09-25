#include "pch.h"
#include "FArchiveMemory.h"

FArchiveMemory::FArchiveMemory(TArray<Uint8>& InBytes)
    : FArchive(EArchiveMode::Saving),
      mWriteBytes(&InBytes),
      mReadBytes(nullptr),
      mReadOffset(0) {
}

FArchiveMemory::FArchiveMemory(const TArray<Uint8>& InBytes)
    : FArchive(EArchiveMode::Loading),
      mWriteBytes(nullptr),
      mReadBytes(&InBytes),
      mReadOffset(0) {
}

#define IMPLEMENT_MEMORY_SERIALIZE(Type)                                     \
    void FArchiveMemory::Serialize(std::string_view Name, Type& Value) {     \
        if (IsSaving()) {                                                    \
            std::size_t Size{sizeof(Type)};                                  \
            std::size_t CurrentSize{mWriteBytes->size()};                    \
            mWriteBytes->resize(CurrentSize + Size);                         \
            std::memcpy(mWriteBytes->data() + CurrentSize, &Value, Size);    \
        } else if (IsLoading()) {                                            \
            std::size_t Size{sizeof(Type)};                                  \
            if (mReadOffset + Size <= mReadBytes->size()) {                  \
                std::memcpy(&Value, mReadBytes->data() + mReadOffset, Size); \
                mReadOffset += Size;                                         \
            }                                                                \
        }                                                                    \
    }

// Primitives
IMPLEMENT_MEMORY_SERIALIZE(bool)
IMPLEMENT_MEMORY_SERIALIZE(Uint8)
IMPLEMENT_MEMORY_SERIALIZE(Int32)
IMPLEMENT_MEMORY_SERIALIZE(Uint32)
IMPLEMENT_MEMORY_SERIALIZE(Int64)
IMPLEMENT_MEMORY_SERIALIZE(Uint64)
IMPLEMENT_MEMORY_SERIALIZE(Float32)
IMPLEMENT_MEMORY_SERIALIZE(Float64)

// Math Types & Guid
IMPLEMENT_MEMORY_SERIALIZE(FGuid)
IMPLEMENT_MEMORY_SERIALIZE(FVector2D)
IMPLEMENT_MEMORY_SERIALIZE(FVector3)
IMPLEMENT_MEMORY_SERIALIZE(FVector4)
IMPLEMENT_MEMORY_SERIALIZE(FQuat)
IMPLEMENT_MEMORY_SERIALIZE(FMatrix)

#undef IMPLEMENT_MEMORY_SERIALIZE

// Engine Core
void FArchiveMemory::Serialize(std::string_view Name, FString& Value) {
    if (IsSaving()) {
        Uint32 StringLen{static_cast<Uint32>(Value.size())};
        Serialize(Name, StringLen);

        if (StringLen > 0) {
            std::size_t CurrentSize{mWriteBytes->size()};
            mWriteBytes->resize(CurrentSize + StringLen);
            std::memcpy(mWriteBytes->data() + CurrentSize, Value.data(), StringLen);
        }
    } else if (IsLoading()) {
        Uint32 StringLen{0};
        Serialize(Name, StringLen);

        if (StringLen > 0 && (mReadOffset + StringLen) <= mReadBytes->size()) {
            Value.resize(StringLen);
            std::memcpy(Value.data(), mReadBytes->data() + mReadOffset, StringLen);
            mReadOffset += StringLen;
        }
    }
}

// Scope
void FArchiveMemory::BeginObjectScope(std::string_view Name) {
    // 바이너리 아카이브에서는 Object Scope 단위 구분을 하지 않음
}

void FArchiveMemory::EndObjectScope() {
    // 바이너리 아카이브에서는 Object Scope 단위 구분을 하지 않음
}

void FArchiveMemory::BeginArrayScope(std::string_view Name, std::size_t& ArraySize) {
    // 배열은 크기정보를 저장해야 함
    Uint32 Size32{static_cast<Uint32>(ArraySize)};

    // Uint32 재사용
    Serialize(Name, Size32);

    if (IsLoading()) {
        ArraySize = Size32;
    }
}

void FArchiveMemory::EndArrayScope() {
}
