/**
 * @file Engine.cpp
 * @brief 현재 월드 컨텍스트와 렌더러 연결을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Engine.h"
#include "Engine/FSceneInterface.h"
#include "Engine/UWorld.h"
#include "Engine/FWorldContext.h"
#include "Engine/EPrimitiveType.h"
#include "RenderCore/IRendererModule.h"
#include "Engine/Reflection/RegisterEngine.h"
#include "Engine/PrimitiveMesh.h"

/**
 * @brief UEngine의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UEngine::UEngine() = default;

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UEngine::~UEngine() {
    Shutdown();
}

/**
 * @brief 렌더러를 연결하고 초기 월드 컨텍스트와 장면을 준비합니다.
 */
bool UEngine::Initialize(IRendererModule& Renderer) {
    if (mWorldContext != nullptr) {
        return false;
    }

    ReflectionModules::RegisterEngine();
    StaticClass();

    for (Uint32 Index{}; Index < static_cast<Uint32>(EPrimitiveType::Count); ++Index) {
        const auto Type{static_cast<EPrimitiveType>(Index)};

        if (!Renderer.UploadMesh(Type, GetPrimitiveMesh(Type))) {
            return false;
        }
    }

    mRenderer = &Renderer;

    return ReplaceWorld(std::make_unique<FWorldContext>());
}

/**
 * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
 */
void UEngine::Shutdown() {
    mWorldContext.reset();
    mRenderer = nullptr;
}

/**
 * @brief 현재 소속되거나 관리하는 월드의 비소유 주소를 반환합니다.
 */
UWorld* UEngine::GetWorld() const {
    return mWorldContext != nullptr ? mWorldContext->GetWorld() : nullptr;
}

/**
 * @brief 레지스트리와 월드를 함께 소유한 현재 컨텍스트를 반환합니다.
 */
FWorldContext* UEngine::GetWorldContext() const {
    return mWorldContext.get();
}

/**
 * @brief 연결된 렌더러의 비소유 주소를 반환합니다.
 */
IRendererModule* UEngine::GetRenderer() const {
    return mRenderer;
}

/**
 * @brief 새 월드 컨텍스트를 검증하고 장면을 연결한 뒤 현재 월드를 교체합니다.
 */
bool UEngine::ReplaceWorld(std::unique_ptr<FWorldContext> Context) {
    if (Context == nullptr || Context->GetWorld() == nullptr || mRenderer == nullptr) {
        return false;
    }

    if (!Context->GetWorld()->AttachScene(mRenderer->AllocateScene())) {
        return false;
    }

    mWorldContext = std::move(Context);
    ++mWorldGeneration;

    return true;
}

/**
 * @brief 월드 교체 시 증가하는 컨텍스트 세대를 반환합니다.
 */
Uint64 UEngine::GetWorldGeneration() const {
    return mWorldGeneration;
}

/**
 * @brief 월드의 프레임 말 렌더 갱신을 모두 반영합니다.
 */
void UEngine::SynchronizeWorld() {
    if (GetWorld() != nullptr) {
        GetWorld()->SendAllEndOfFrameUpdates();
    }
}
