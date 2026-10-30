/**
 * @file MemoryArchive.cpp
 * @brief 바이트 배열을 순차적으로 읽고 쓰는 바이너리 아카이브에 대한 구현입니다.
 * @ingroup Serialization
 */

#include "Serialization/MemoryArchive.h"

#include <cmath>
#include <cstring>
#include <limits>

namespace Serialization {

    Serialization::FMemoryArchive::FMemoryArchive(Core::TArray<Core::Uint8>& Bytes)
		: Core::FArchive{Core::EArchiveMode::Saving},
		  mWriteBytes{&Bytes} {
    }

    Serialization::FMemoryArchive::FMemoryArchive(const Core::TArray<Core::Uint8>& Bytes)
		: Core::FArchive{Core::EArchiveMode::Loading},
		  mReadBytes{&Bytes} {
    }

    std::size_t Serialization::FMemoryArchive::GetRemainingBytes() const {
        return mReadBytes != nullptr ? mReadBytes->size() - mReadOffset : 0;
    }

    bool Serialization::FMemoryArchive::HasField(std::string_view) const {
        return !HasError();
    }

    bool Serialization::FMemoryArchive::BeginObject(std::string_view) {
        return !HasError();
    }

    void Serialization::FMemoryArchive::EndObject() {
    }

    bool Serialization::FMemoryArchive::BeginArray(std::string_view Name, Core::Uint32& Count) {
        Serialize(Name, Count);

        if (!HasError() && Count > 100000) {
            SetError("Memory archive array exceeds the element limit");
        }

        return !HasError();
    }

    void Serialization::FMemoryArchive::EndArray() {
    }

    void Serialization::FMemoryArchive::Serialize(std::string_view, bool& Value) {
        Core::Uint8 Byte{static_cast<Core::Uint8>(Value)};
        SerializeBytes(&Byte, sizeof(Byte));

        if (IsLoading() && !HasError()) {
            if (Byte > 1) {
                SetError("Invalid boolean in memory archive");
                return;
            }

            Value = Byte != 0;
        }
    }

    void Serialization::FMemoryArchive::Serialize(std::string_view, Core::Int32& Value) {
        SerializeBytes(&Value, sizeof(Value));
    }

    void Serialization::FMemoryArchive::Serialize(std::string_view, Core::Uint32& Value) {
        SerializeBytes(&Value, sizeof(Value));
    }

    void Serialization::FMemoryArchive::Serialize(std::string_view, Core::Uint64& Value) {
        SerializeBytes(&Value, sizeof(Value));
    }

    void Serialization::FMemoryArchive::Serialize(std::string_view, float& Value) {
        if (!IsLoading() && !std::isfinite(Value)) {
            SetError("Cannot serialize a non-finite float");
            return;
        }

        SerializeBytes(&Value, sizeof(Value));

        if (!HasError() && !std::isfinite(Value)) {
            SetError("Invalid float in memory archive");
        }
    }

    void Serialization::FMemoryArchive::Serialize(std::string_view Name, Core::FString& Value) {
        if (HasError()) {
            return;
        }

        if (!IsLoading() && Value.size() > std::numeric_limits<Core::Uint32>::max()) {
            SetError("Memory archive string is too large");
            return;
        }

        Core::Uint32 Length{IsLoading() ? 0 : static_cast<Core::Uint32>(Value.size())};
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

    void Serialization::FMemoryArchive::SerializeBytes(void* Value, std::size_t Size) {
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

}
