/**
 * @file FAnimSequenceTimeline.h
 * @brief 시퀀스의 프레임 눈금, 탐색 구간과 재생 조작 UI를 관리합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"

namespace Asset {
    class UAnimSequence;
}

namespace Viewer {
    class FAnimationViewerContent;
}

namespace Editor {

    /**
     * @brief 시퀀스의 프레임 눈금, 탐색 구간과 재생 조작 UI를 관리합니다.
     * @ingroup Editor
     */
    class FAnimSequenceTimeline {
    public:

        /**
         * @brief 빈 시퀀스의 타임라인 표시 범위를 구성합니다.
         */
        FAnimSequenceTimeline();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FAnimSequenceTimeline();

    public:

        /**
         * @brief 시퀀스 길이와 프레임률로 타임라인 범위·리비전을 초기화합니다.
         * @param Sequence 시간 범위와 프레임률을 제공하는 시퀀스입니다.
         */
        void Reset(const Asset::UAnimSequence& Sequence);

        /**
         * @brief 시간 눈금, 현재 프레임과 재생 조작 막대를 표시합니다.
         * @param Preview 재생·포즈를 조작할 애니메이션 뷰어 콘텐츠입니다.
         * @param Sequence 시간 범위와 프레임률을 제공하는 시퀀스입니다.
         * @param Height 출력 또는 UI 영역의 높이(픽셀)입니다.
         */
        void Draw(Viewer::FAnimationViewerContent& Preview, const Asset::UAnimSequence& Sequence, float Height);

        /**
         * @brief 재생을 정지하고 지정 방향으로 시퀀스 한 프레임을 이동합니다.
         * @param Preview 재생·포즈를 조작할 애니메이션 뷰어 콘텐츠입니다.
         * @param Sequence 시간 범위와 프레임률을 제공하는 시퀀스입니다.
         * @param Direction 음수는 이전, 양수는 다음 프레임입니다.
         */
        void StepFrame(Viewer::FAnimationViewerContent& Preview, const Asset::UAnimSequence& Sequence, Core::Int32 Direction);

    private:

        /**
         * @brief 프레임 눈금과 현재 시각을 표시하고 스크러빙 입력을 처리합니다.
         * @param Preview 재생·포즈를 조작할 애니메이션 뷰어 콘텐츠입니다.
         * @param Sequence 시간 범위와 프레임률을 제공하는 시퀀스입니다.
         * @param Height 출력 또는 UI 영역의 높이(픽셀)입니다.
         */
        void DrawTimeArea(Viewer::FAnimationViewerContent& Preview, const Asset::UAnimSequence& Sequence, float Height);

        /**
         * @brief 처음·끝·프레임 이동, 순·역재생과 반복 조작 UI를 표시합니다.
         * @param Preview 재생·포즈를 조작할 애니메이션 뷰어 콘텐츠입니다.
         * @param Sequence 시간 범위와 프레임률을 제공하는 시퀀스입니다.
         */
        void DrawTransportBar(Viewer::FAnimationViewerContent& Preview, const Asset::UAnimSequence& Sequence);

    private:

        /**
         * @brief 타임라인 표시 범위의 마지막 프레임입니다.
         */
        Core::Int32 mEndFrame{1};

        /**
         * @brief 타임라인 상태를 초기화할 때 위젯 ID를 갱신하는 세대 번호입니다.
         */
        Core::Uint32 mSequenceRevision{};
    };

}
