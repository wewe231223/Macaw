/**
 * @file FPreviewScene.h
 * @brief 전역 에셋을 공유하는 독립 프리뷰 월드의 수명을 정의합니다.
 */

#pragma once
#include "World/UWorld.h"
#include "RenderCore/FPrimitiveSceneProxy.h"

class FRenderer;
class FRenderScene;
class UDirectionalLightComponent;

class FPreviewScene final {
public:
    FPreviewScene() = default;
    ~FPreviewScene();
    FPreviewScene(const FPreviewScene&) = delete;

    FPreviewScene& operator=(const FPreviewScene&) = delete;

    FPreviewScene(FPreviewScene&&) = delete;

    FPreviewScene& operator=(FPreviewScene&&) = delete;

public:
    /**
     * @brief 전역 에셋 서비스를 사용하는 독립된 프리뷰 월드와 렌더 씬을 생성합니다.
     * @param Renderer 프리뷰 씬을 소유할 렌더러입니다.
     * @return 에셋 서비스가 존재하고 프리뷰 생성에 성공하면 true입니다.
     * @details 같은 렌더러와 에셋 서비스 세대에서 초기화되어 있으면 월드와 프리뷰 상태를 유지합니다. 세대가 바뀌면 프리뷰 월드를 다시 생성합니다.
     */
    bool Initialize(FRenderer& Renderer);
    void SetMesh(const FMeshSceneData& Mesh, const FMatrix& World);
    void SetLight(const FVector3& Direction, const FVector3& Color, float Intensity);
    const FRenderScene* Synchronize();
    void Reset();

private:
    UWorld mWorld{};
    FRenderer* mRenderer{nullptr};

    /** @brief 프리뷰 월드를 생성한 에셋 서비스 세대입니다. 세대 변경 시 프리뷰를 다시 생성합니다. */
    Uint64 mRegistryGeneration{};
    UStaticMeshComponent* mMesh{nullptr};
    UDirectionalLightComponent* mLight{nullptr};
};
