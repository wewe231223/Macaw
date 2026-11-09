/**
 * @file FMeshDrawCommand.h
 * @brief 메시 패스가 정렬·제출하는 파이프라인·머티리얼·메시 드로 묶음을 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace Asset {
    class UMaterial;
    class UStaticMesh;
}

namespace Renderer {

    class FRenderer;
    class FMeshPass;
    class FMaterialPipeline;
    struct FScenePrimitive;

    /**
     * @brief 파이프라인·머티리얼·메시·객체 상수 버퍼의 비소유 드로 묶음입니다.
     */
    struct FMeshDrawCommand {
    public:
        /**
         * @brief 전달된 설정과 기본값으로 FMeshDrawCommand 객체를 생성합니다.
         * @param[in] Pipeline 드로에 사용할 GPU 파이프라인입니다.
         * @param[in] Material 바인딩할 머티리얼이며, 머티리얼을 쓰지 않는 패스는 nullptr입니다.
         * @param[in] Mesh 드로에 사용할 정적 메시입니다.
         * @param[in] Primitive 월드 행렬과 UV가 저장된 렌더 프리미티브입니다.
         * @param[in] FirstIndex SubMesh의 첫 인덱스 오프셋입니다.
         * @param[in] IndexCount SubMesh의 삼각형 인덱스 개수입니다.
         */
        FMeshDrawCommand(Renderer::FMaterialPipeline* Pipeline, Asset::UMaterial* Material, Asset::UStaticMesh* Mesh, const Renderer::FScenePrimitive* Primitive, Core::Uint32 FirstIndex, Core::Uint32 IndexCount);

    private:
        /**
         * @brief 렌더러가 정렬한 드로의 구간과 GPU 자원을 읽도록 허용합니다.
         */
        friend class FRenderer;

        /**
         * @brief 메시 패스가 드로 목록을 정렬하도록 허용합니다.
         */
        friend class FMeshPass;

    private:
        /**
         * @brief 이번 메시 드로에 사용할 머티리얼 파이프라인입니다.
         */
        Renderer::FMaterialPipeline* mPipeline{};

        /**
         * @brief 연결된 머티리얼의 비소유 주소이며, 머티리얼을 쓰지 않는 패스는 nullptr입니다.
         */
        Asset::UMaterial* mMaterial{};

        /**
         * @brief 이번 드로에 사용할 정적 메시입니다.
         */
        Asset::UStaticMesh* mMesh{};

        /**
         * @brief 객체 행렬과 UV 변환을 가진 렌더 프리미티브입니다.
         */
        const Renderer::FScenePrimitive* mPrimitive{};

        /**
         * @brief 이 드로의 첫 인덱스 오프셋입니다.
         */
        Core::Uint32 mFirstIndex{};

        /**
         * @brief 이 드로의 삼각형 인덱스 개수입니다.
         */
        Core::Uint32 mIndexCount{};
    };

}
