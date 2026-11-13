/**
 * @file FViewerTypes.h
 * @brief 뷰어 촬영의 준비 결과와 재현 가능한 촬영 상태를 정의합니다.
 * @ingroup Viewer
 */

#pragma once

#include "Core/Types.h"

namespace Viewer {

    /**
     * @brief 촬영 상태 준비의 완료·대기·실패 결과입니다.
     * @ingroup Viewer
     */
    enum class EViewerPrepareResult : Core::Uint8 {

        /**
         * @brief 요청한 상태가 준비되어 바로 촬영할 수 있습니다.
         */
        Ready,

        /**
         * @brief 준비가 끝나지 않아 이후 호출에서 계속 처리해야 합니다.
         */
        Pending,

        /**
         * @brief 촬영 준비에 실패했으며 오류 문자열을 확인해야 합니다.
         */
        Failed
    };

    /**
     * @brief 촬영할 시각, 난수 시드와 고정 시뮬레이션 간격을 전달하는 값 객체입니다.
     * @ingroup Viewer
     */
    class FViewerCaptureState {
    public:

        /**
         * @brief 촬영할 시각·시드·고정 간격을 보관하며 유효성은 콘텐츠 준비 단계에서 검사합니다.
         * @param Time 촬영할 콘텐츠 시각(초)입니다.
         * @param Seed 촬영 상태를 재현할 난수 시드입니다.
         * @param FixedStep 촬영 준비에서 사용할 양의 고정 간격(초)입니다.
         */
        explicit FViewerCaptureState(double Time = 0.0, Core::Uint32 Seed = 0, float FixedStep = 1.0f / 60.0f);

    public:

        /**
         * @brief 촬영할 콘텐츠 시각을 반환합니다.
         * @return 콘텐츠 시각(초)입니다.
         */
        double GetTime() const;

        /**
         * @brief 촬영 상태를 재현하기 위한 난수 시드를 반환합니다.
         * @return 콘텐츠에 전달할 시드 값입니다.
         */
        Core::Uint32 GetSeed() const;

        /**
         * @brief 촬영 준비에 사용할 고정 시뮬레이션 간격을 반환합니다.
         * @return 고정 간격(초)입니다.
         */
        float GetFixedStep() const;

    private:

        /**
         * @brief 촬영할 콘텐츠 시각(초)입니다.
         */
        double mTime{};

        /**
         * @brief 재현 가능한 촬영 상태를 위한 난수 시드입니다.
         */
        Core::Uint32 mSeed{};

        /**
         * @brief 촬영 준비 시 사용할 고정 시뮬레이션 간격(초)입니다.
         */
        float mFixedStep{1.0f / 60.0f};
    };
}
