/**
 * @file MemoryArchive.cpp
 * @brief 바이트 배열을 순차적으로 읽고 쓰는 바이너리 아카이브에 대한 구현입니다.
 * @ingroup Serialization
 */

#include "Serialization/MemoryArchive.h"

#include <cmath>
#include <cstring>
#include <limits>

FMemoryArchive::FMemoryArchive(TArray<Uint8>& Bytes)
	: FArchive{EArchiveMode::Saving},
	  mWriteBytes{&Bytes} {
}

FMemoryArchive::FMemoryArchive(const TArray<Uint8>& Bytes)
	: FArchive{EArchiveMode::Loading},
	  mReadBytes{&Bytes} {
}

std::size_t FMemoryArchive::GetRemainingBytes() const {
    return mReadBytes != nullptr ? mReadBytes->size() - mReadOffset : 0;
}

bool FMemoryArchive::HasField(std::string_view) const {
    return !HasError();
}

bool FMemoryArchive::BeginObject(std::string_view) {
    return !HasError();
}

void FMemoryArchive::EndObject() {
}

bool FMemoryArchive::BeginArray(std::string_view Name, Uint32& Count) {
    Serialize(Name, Count);

    if (!HasError() && Count > 100000) {
        SetError("Memory archive array exceeds the element limit");
    }

    return !HasError();
}

void FMemoryArchive::EndArray() {
}

void FMemoryArchive::Serialize(std::string_view, bool& Value) {
    Uint8 Byte{static_cast<Uint8>(Value)};
    SerializeBytes(&Byte, sizeof(Byte));

    if (IsLoading() && !HasError()) {
        if (Byte > 1) {
            SetError("Invalid boolean in memory archive");
            return;
        }

        Value = Byte != 0;
    }
}

void FMemoryArchive::Serialize(std::string_view, Int32& Value) {
    SerializeBytes(&Value, sizeof(Value));
}

void FMemoryArchive::Serialize(std::string_view, Uint32& Value) {
    SerializeBytes(&Value, sizeof(Value));
}

void FMemoryArchive::Serialize(std::string_view, Uint64& Value) {
    SerializeBytes(&Value, sizeof(Value));
}

void FMemoryArchive::Serialize(std::string_view, float& Value) {
    if (!IsLoading() && !std::isfinite(Value)) {
        SetError("Cannot serialize a non-finite float");
        return;
    }

    SerializeBytes(&Value, sizeof(Value));

    if (!HasError() && !std::isfinite(Value)) {
        SetError("Invalid float in memory archive");
    }
}

void FMemoryArchive::Serialize(std::string_view Name, FString& Value) {
    if (HasError()) {
        return;
    }

    if (!IsLoading() && Value.size() > std::numeric_limits<Uint32>::max()) {
        SetError("Memory archive string is too large");
        return;
    }

    Uint32 Length{IsLoading() ? 0 : static_cast<Uint32>(Value.size())};
    Serialize(Name, Length);

    if (HasError()) {
        return;
    }

    if (IsLoading()) {
        if (Length > GetRemainingBytes()) {
            SetError("Truncated string in memory archive");
            return;
        }

        Value.resize(Length);
    }

    SerializeBytes(Value.data(), Length);
}

void FMemoryArchive::SerializeBytes(void* Value, std::size_t Size) {
    if (HasError() || Size == 0) {
        return;
    }

    if (IsLoading()) {
        if (Size > GetRemainingBytes()) {
            SetError("Truncated memory archive");
            return;
        }

        std::memcpy(Value, mReadBytes->data() + mReadOffset, Size);
        mReadOffset += Size;
    } else {
        const std::size_t Offset{mWriteBytes->size()};

        if (Size > mWriteBytes->max_size() - Offset) {
            SetError("Memory archive buffer is too large");
            return;
        }

        mWriteBytes->resize(Offset + Size);
        std::memcpy(mWriteBytes->data() + Offset, Value, Size);
    }
}
