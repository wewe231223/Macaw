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
         * @brief 컷이 시작하는 시간이며 틱 단위입니다.
         */
        Core::Int32 mTick{};

        /**
         * @brief 카메라를 가리키는 시퀀스 바인딩 ID입니다.
         */
        Core::FGuid mCameraBinding{};
    };

}
