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

    /**
     * @brief 바이트를 추가할 배열로 저장 아카이브를 구성합니다.
     */
    Serialization::FMemoryArchive::FMemoryArchive(Core::TArray<Core::Uint8>& Bytes)
		: Core::FArchive{Core::EArchiveMode::Saving},
		  mWriteBytes{&Bytes} {
    }

    /**
     * @brief 읽기 전용 바이트 배열로 로드 아카이브를 구성합니다.
     */
    Serialization::FMemoryArchive::FMemoryArchive(const Core::TArray<Core::Uint8>& Bytes)
		: Core::FArchive{Core::EArchiveMode::Loading},
		  mReadBytes{&Bytes} {
    }

    /**
     * @brief 로드 아카이브에서 아직 읽지 않은 바이트 수를 반환합니다.
     */
    std::size_t Serialization::FMemoryArchive::GetRemainingBytes() const {
        return mReadBytes != nullptr ? mReadBytes->size() - mReadOffset : 0;
    }

    /**
     * @brief 현재 컨테이너에 지정한 이름의 필드가 있는지 검사합니다.
     */
    bool Serialization::FMemoryArchive::HasField(std::string_view) const {
        return !HasError();
    }

    /**
     * @brief 이름으로 지정한 객체 컨테이너에 진입합니다.
     */
    bool Serialization::FMemoryArchive::BeginObject(std::string_view) {
        return !HasError();
    }

    /**
     * @brief 현재 객체 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
     */
    void Serialization::FMemoryArchive::EndObject() {
    }

    /**
     * @brief 배열 컨테이너에 진입하고 로드 시 원소 개수를 읽습니다.
     */
    bool Serialization::FMemoryArchive::BeginArray(std::string_view Name, Core::Uint32& Count) {
        Serialize(Name, Count);

        if (!HasError() && Count > 100000) {
            SetError("Memory archive array exceeds the element limit");
        }

        return !HasError();
    }

    /**
     * @brief 현재 배열 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
     */
    void Serialization::FMemoryArchive::EndArray() {
    }

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
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

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
    void Serialization::FMemoryArchive::Serialize(std::string_view, Core::Int32& Value) {
        SerializeBytes(&Value, sizeof(Value));
    }

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
    void Serialization::FMemoryArchive::Serialize(std::string_view, Core::Uint32& Value) {
        SerializeBytes(&Value, sizeof(Value));
    }

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
    void Serialization::FMemoryArchive::Serialize(std::string_view, Core::Uint64& Value) {
        SerializeBytes(&Value, sizeof(Value));
    }

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
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

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
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

    /**
     * @brief 아카이브 모드에 따라 바이트를 추가하거나 범위를 검사하여 읽습니다.
     */
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
