/**
 * @file FSequenceKey.h
 * @brief 시퀀스 채널의 단일 키에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Sequence/ESequenceInterpolation.h"
#include "Core/ReflectionMacros.h"

namespace Asset {

    /**
     * @brief 시퀀스 채널의 단일 키를 저장합니다.
     * @ingroup Asset
     */
    struct FSequenceKey {
    public:
        /**
         * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
         */
        ReflectTypes(FSequenceKey);

    public:
        /**
         * @brief 정적 구조체 메타데이터를 반환합니다.
         * @return 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSequenceKey);

    private:
        /**
         * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 키의 시간이며 ULevelSequence::TicksPerSecond 단위입니다.
         */
        Core::Int32 mTick{};
        ReflectProperty(mTick, "Tick", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 키의 값입니다.
         */
        float mValue{};
        ReflectProperty(mValue, "Value", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이 키에서 다음 키까지의 보간 방식입니다.
         */
        Asset::ESequenceInterpolation mInterpolation{Asset::ESequenceInterpolation::Cubic};
        ReflectProperty(mInterpolation, "Interpolation", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
