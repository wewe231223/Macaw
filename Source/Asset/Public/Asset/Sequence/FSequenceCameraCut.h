/**
 * @file FSequenceCameraCut.h
 * @brief 지정한 시간부터 사용할 카메라를 가리키는 컷에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Guid.h"
#include "Core/ReflectionMacros.h"

namespace Asset {

    /**
     * @brief 지정한 시간부터 다음 컷까지 사용할 카메라를 가리킵니다.
     * @ingroup Asset
     */
    struct FSequenceCameraCut {
    public:
        /**
         * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
         */
        ReflectTypes(FSequenceCameraCut);

    public:
        /**
         * @brief 정적 구조체 메타데이터를 반환합니다.
         * @return 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSequenceCameraCut);

    private:
        /**
         * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 컷이 시작하는 시간이며 틱 단위입니다.
         */
        Core::Int32 mTick{};
        ReflectProperty(mTick, "Tick", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 카메라를 가리키는 시퀀스 바인딩 ID입니다.
         */
        Core::FGuid mCameraBinding{};
        ReflectProperty(mCameraBinding, "CameraBinding", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
