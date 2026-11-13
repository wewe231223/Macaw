/**
 * @file IViewerContent.cpp
 * @brief 에셋별 준비, 갱신, 경계, 장면과 촬영 상태를 공통 뷰어에 제공하는 인터페이스입니다.
 * @ingroup Viewer
 */

#include "Viewer/IViewerContent.h"

#include <cmath>

using namespace Viewer;

IViewerContent::~IViewerContent() = default;

Asset::UTexture2D* IViewerContent::GetTexture() const {
    return nullptr;
}

void IViewerContent::Tick(float) {
}

void IViewerContent::Synchronize() {
}

void IViewerContent::BuildOverlay(Engine::FPrimitiveDrawInterface&, float) const {
}

FViewerCaptureState IViewerContent::GetCaptureState() const {
    return FViewerCaptureState{};
}

EViewerPrepareResult IViewerContent::PrepareCapture(const FViewerCaptureState& State, Core::FString& Error) {
    if (!IsReady() || !std::isfinite(State.GetTime()) || State.GetTime() < 0.0 || !std::isfinite(State.GetFixedStep()) || State.GetFixedStep() <= 0.0f) {
        Error = "The viewer content or capture state is invalid";

        return EViewerPrepareResult::Failed;
    }

    Synchronize();

    return EViewerPrepareResult::Ready;
}
