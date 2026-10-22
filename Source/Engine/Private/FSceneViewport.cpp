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

/**
 * @brief FSceneViewport의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FSceneViewport::FSceneViewport(IRendererModule& Renderer, FViewportClient& Client)
	: mRenderer{&Renderer},
	  mClient{&Client} {
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

    return mRenderer->ResizeRenderTarget(*mTarget, Width, Height);
}

/**
 * @brief 현재 프레임의 뷰포트 입력을 처리합니다.
 */
void FSceneViewport::Input(const FViewportInput& Input, float DeltaTime) {
    mClient->Input(Input, GetSize(), DeltaTime);
}

/**
 * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
 */
void FSceneViewport::Draw() {
    if (mTarget != nullptr) {
        mClient->Draw(*this);
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
