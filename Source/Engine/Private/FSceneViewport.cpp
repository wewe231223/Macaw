/**
 * @file FSceneViewport.cpp
 * @brief 뷰포트 클라이언트와 렌더 타깃의 입력 및 그리기 호출을 연결합니다.
 * @ingroup Engine
 */

#include "Engine/FSceneViewport.h"
#include "Core/Math/FVector2.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FViewportInput.h"
#include "Engine/FViewportClient.h"
#include "RenderCore/IRendererModule.h"

#include <cmath>

/**
 * @brief FSceneViewport의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FSceneViewport::FSceneViewport(IRendererModule* Renderer, FViewportClient* Client)
	: mRenderer{Renderer},
	  mClient{Client} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FSceneViewport::~FSceneViewport() = default;

/**
 * @brief 새 크기에 맞추어 렌더 타깃 리소스를 생성하거나 교체합니다.
 */
bool FSceneViewport::Resize(Uint32 Width, Uint32 Height) {
    if (mTarget == nullptr) {
        mTarget = mRenderer->CreateRenderTarget(Width, Height);

        return mTarget != nullptr;
    }

    if (Width == mTarget->GetWidth() && Height == mTarget->GetHeight()) {
        return true;
    }

    return mRenderer->ResizeRenderTarget(mTarget.get(), Width, Height);
}

/**
 * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
 */
void FSceneViewport::Input(const FViewportInput& Input, float DeltaTime) {
    if (mClient != nullptr && mTarget != nullptr) {
        mClient->Input(this, Input, DeltaTime);
    }
}

/**
 * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
 */
void FSceneViewport::Draw() {
    if (mTarget != nullptr && mClient != nullptr) {
        mClient->Draw(this);
    }
}

/**
 * @brief 컬러 렌더 타깃 뷰를 반환합니다.
 */
FRenderTarget* FSceneViewport::GetRenderTarget() const {
    return mTarget.get();
}

/**
 * @brief 크기를 반환합니다.
 */
FVector2 FSceneViewport::GetSize() const {
    return mTarget != nullptr ? FVector2{static_cast<float>(mTarget->GetWidth()), static_cast<float>(mTarget->GetHeight())} : FVector2{};
}

/**
 * @brief 유한한 타깃 내부 좌표에 대해 클라이언트의 피킹 결과를 조회합니다.
 */
FHitProxy FSceneViewport::GetHitProxy(const FVector2& Position) const {
    if (mTarget == nullptr || mClient == nullptr || !std::isfinite(Position.x) || !std::isfinite(Position.y) || Position.x < 0.0f || Position.y < 0.0f || Position.x >= mTarget->GetWidth() || Position.y >= mTarget->GetHeight()) {
        return {};
    }

    return mClient->GetHitProxy(this, Position);
}
