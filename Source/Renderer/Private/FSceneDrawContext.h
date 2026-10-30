/**
 * @file FSceneDrawContext.h
 * @brief 장면 렌더링에 빌려 쓰는 장치 컨텍스트, 메시 및 파이프라인 리소스입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/UStaticMesh.h"

namespace RenderCore {
    class FDynamicVertexBuffer;
}

namespace Renderer {

    class FRenderer;

    /**
     * @brief 장면 렌더링에 빌려 쓰는 장치 컨텍스트, 메시 및 파이프라인 리소스입니다.
     * @ingroup Renderer
     */
    struct FSceneDrawContext {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FSceneDrawContext);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSceneDrawContext);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        Renderer::FRenderer* mRenderer{};
        ReflectProperty(mRenderer, "Renderer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Direct3D 11 즉시 컨텍스트입니다.
         */
        ID3D11DeviceContext* mContext{};
        ReflectProperty(mContext, "Context", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프리미티브 종류별 GPU 메시입니다.
         */
        const std::array<std::unique_ptr<Engine::UStaticMesh>, 3>* mMeshes{};
        ReflectProperty(mMeshes, "Meshes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 뷰·투영 상수 버퍼입니다.
         */
        ID3D11Buffer* mViewBuffer{};
        ReflectProperty(mViewBuffer, "ViewBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메시 인스턴스의 월드 행렬을 업로드하는 동적 버퍼의 비소유 주소입니다.
         */
        RenderCore::FDynamicVertexBuffer* mInstanceBuffer{};
        ReflectProperty(mInstanceBuffer, "InstanceBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
