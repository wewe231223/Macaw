#include "pch.h"
#include "Serialization/FArchiveMemory.h"
#include <limits>

FArchiveMemory::FArchiveMemory(TArray<Uint8>& InBytes)
	: FArchive{EArchiveMode::Saving},
	  mWriteBytes{&InBytes} {
}

FArchiveMemory::FArchiveMemory(const TArray<Uint8>& InBytes)
	: FArchive{EArchiveMode::Loading},
	  mReadBytes{&InBytes} {
}

std::size_t FArchiveMemory::GetRemainingBytes() const {
    return mReadBytes != nullptr ? mReadBytes->size() - mReadOffset : 0;
}

void FArchiveMemory::SerializeBytes(void* Value, std::size_t Size) {
    if (HasError() || Size == 0) {
        return;
    }

    if (IsSaving()) {
        const std::size_t CurrentSize{mWriteBytes->size()};

        if (Size > mWriteBytes->max_size() - CurrentSize) {
            SetError();
            return;
        }

        mWriteBytes->resize(CurrentSize + Size);
        std::memcpy(mWriteBytes->data() + CurrentSize, Value, Size);
    } else if (IsLoading()) {
        if (Size > GetRemainingBytes()) {
            SetError();
            return;
        }

        std::memcpy(Value, mReadBytes->data() + mReadOffset, Size);
        mReadOffset += Size;
    }
}

#define IMPLEMENT_MEMORY_SERIALIZE(Type) \
    void FArchiveMemory::Serialize(std::string_view Name, Type& Value) { \
        SerializeBytes(&Value, sizeof(Type)); \
    }

IMPLEMENT_MEMORY_SERIALIZE(Uint8)
IMPLEMENT_MEMORY_SERIALIZE(Int32)
IMPLEMENT_MEMORY_SERIALIZE(Uint32)
IMPLEMENT_MEMORY_SERIALIZE(Int64)
IMPLEMENT_MEMORY_SERIALIZE(Uint64)
IMPLEMENT_MEMORY_SERIALIZE(Float32)
IMPLEMENT_MEMORY_SERIALIZE(Float64)
IMPLEMENT_MEMORY_SERIALIZE(FGuid)
IMPLEMENT_MEMORY_SERIALIZE(FVector2D)
IMPLEMENT_MEMORY_SERIALIZE(FVector3)
IMPLEMENT_MEMORY_SERIALIZE(FVector4)
IMPLEMENT_MEMORY_SERIALIZE(FQuat)
IMPLEMENT_MEMORY_SERIALIZE(FMatrix)

#undef IMPLEMENT_MEMORY_SERIALIZE

void FArchiveMemory::Serialize(std::string_view Name, bool& Value) {
    Uint8 Byte{static_cast<Uint8>(Value)};

    Serialize(Name, Byte);

    if (IsLoading() && !HasError()) {
        if (Byte > 1) {
            SetError();
            return;
        }

        Value = Byte != 0;
    }
}

void FArchiveMemory::Serialize(std::string_view Name, FString& Value) {
    if (HasError()) {
        return;
    }

    if (IsSaving()) {
        if (Value.size() > std::numeric_limits<Uint32>::max()) {
            SetError();
            return;
        }

        Uint32 StringLength{static_cast<Uint32>(Value.size())};

        Serialize(Name, StringLength);
        SerializeBytes(Value.data(), StringLength);
    } else if (IsLoading()) {
        Uint32 StringLength{};

        Serialize(Name, StringLength);

        if (HasError() || StringLength > GetRemainingBytes()) {
            SetError();
            return;
        }

        Value.resize(StringLength);
        SerializeBytes(Value.data(), StringLength);
    }
}

void FArchiveMemory::BeginObjectScope(std::string_view Name) {
}

void FArchiveMemory::EndObjectScope() {
}

void FArchiveMemory::BeginArrayScope(std::string_view Name, std::size_t& ArraySize) {
    if (IsSaving() && ArraySize > std::numeric_limits<Uint32>::max()) {
        SetError();
        return;
    }

    Uint32 Size{IsSaving() ? static_cast<Uint32>(ArraySize) : 0};

    Serialize(Name, Size);

    if (IsLoading()) {
        if (HasError() || Size > GetRemainingBytes()) {
            SetError();
            ArraySize = 0;
            return;
        }

        ArraySize = Size;
    }
}

void FArchiveMemory::EndArrayScope() {
}
