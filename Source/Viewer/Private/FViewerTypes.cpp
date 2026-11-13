/**
 * @file FViewerTypes.cpp
 * @brief 뷰어 촬영의 준비 결과와 재현 가능한 촬영 상태를 정의합니다.
 * @ingroup Viewer
 */

#include "Viewer/FViewerTypes.h"

using namespace Viewer;

FViewerCaptureState::FViewerCaptureState(double Time, Core::Uint32 Seed, float FixedStep)
	: mTime{Time},
	  mSeed{Seed},
	  mFixedStep{FixedStep} {
}

double FViewerCaptureState::GetTime() const {
    return mTime;
}

Core::Uint32 FViewerCaptureState::GetSeed() const {
    return mSeed;
}

float FViewerCaptureState::GetFixedStep() const {
    return mFixedStep;
}
