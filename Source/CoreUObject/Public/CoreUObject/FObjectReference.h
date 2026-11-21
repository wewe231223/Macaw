/**
 * @file FObjectReference.h
 * @brief GUID로 저장되고 세대 핸들로 해석되는 UObject 약한 참조입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/GuidHash.h"
#include "Core/Types.h"
#include "CoreUObject/TObjectRef.h"
#include "CoreUObject/UObject.h"

#include <optional>

namespace CoreUObject {

    /**
     * @brief 한 장면 안에서 GUID와 객체를 대응시키는 비소유 대응표입니다.
     */
    using FObjectGuidMap = Core::TMap<Core::FGuid, CoreUObject::UObject*, Core::FGuidHash>;

    /**
     * @brief GUID로 저장되고 세대 핸들로 해석되는 UObject 약한 참조입니다.
     * @ingroup CoreUObject
     * @note 로드 직후에는 GUID만 복원됩니다. 핸들은 장면의 모든 객체가 만들어진 뒤 Bind()로 연결합니다.
     */
    struct FObjectReference {
    public:
        /**
         * @brief 아무 객체도 가리키지 않는 참조를 구성합니다.
         */
        FObjectReference() = default;

        /**
         * @brief 아직 핸들이 연결되지 않은 참조를 구성합니다.
         * @param[in] Guid 대상 객체의 영속 GUID입니다.
         */
        explicit FObjectReference(const Core::FGuid& Guid);

        /**
         * @brief 살아 있는 객체를 가리키는 참조를 구성합니다.
         * @param[in] Object 대상 객체이며 nullptr이면 빈 참조입니다.
         */
        explicit FObjectReference(const CoreUObject::UObject* Object);

        /**
         * @brief 형식 지정 객체 참조의 핸들과 GUID를 저장 가능한 객체 참조로 복사합니다.
         * @tparam T 참조 대상의 형식입니다.
         * @param[in] Reference 변환하거나 해시할 객체 참조입니다.
         */
        template <typename T>
        explicit FObjectReference(const CoreUObject::TObjectRef<T>& Reference);

    public:

        /**
         * @brief 대상 객체의 영속 GUID를 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::FGuid& GetGuid() const;

        /**
         * @brief 연결된 핸들로 살아 있는 대상 객체를 조회합니다.
         * @return 대상 객체의 비소유 주소이며, 연결되지 않았거나 파괴되었으면 nullptr입니다.
         */
        CoreUObject::UObject* Resolve() const;

        /**
         * @brief 장면 대응표에서 GUID를 찾아 핸들을 연결합니다.
         * @param[in] Objects 이 장면에서 읽은 객체의 대응표입니다.
         * @note 대응표에 없는 GUID는 유지하고 핸들만 비웁니다.
         */
        void Bind(const CoreUObject::FObjectGuidMap& Objects);

        /**
         * @brief 저장된 참조를 요청한 객체 형식의 런타임 참조로 변환합니다.
         * @tparam T 참조 대상의 형식입니다.
         * @return 호환되는 객체 참조이며 변환할 수 없으면 std::nullopt입니다.
         */
        template <typename T>
        std::optional<CoreUObject::TObjectRef<T>> ToObjectRef() const;

    private:
        /**
         * @brief 대상 객체의 영속 GUID입니다.
         */
        Core::FGuid mGuid{Core::FGuid::Null()};

        /**
         * @brief 런타임에 해석된 대상 객체의 세대 핸들입니다.
         */
        CoreUObject::FObjectHandle mHandle{};
    };


    template <typename T>
    FObjectReference::FObjectReference(const CoreUObject::TObjectRef<T>& Reference)
		: mGuid{Reference.mGuid},
		  mHandle{Reference.mHandle} {
    }

    template <typename T>
    std::optional<CoreUObject::TObjectRef<T>> FObjectReference::ToObjectRef() const {
        using FMutableType = std::remove_const_t<T>;

        if (Resolve() != nullptr && CoreUObject::ObjectRefPrivate::Resolve(mHandle, FMutableType::StaticClass()) == nullptr) {
            return std::nullopt;
        }

        CoreUObject::TObjectRef<T> Value{};
        Value.mHandle = mHandle;
        Value.mGuid = mGuid;

        return Value;
    }

}
