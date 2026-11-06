/**
 * @file FMeshDrawCommand.cpp
 * @brief 메시 패스가 정렬·제출하는 파이프라인·머티리얼·메시 드로 묶음을 정의합니다.
 * @ingroup Renderer
 */

#include "FMeshDrawCommand.h"

namespace Renderer {

    /**
     * @brief 전달된 설정과 기본값으로 FMeshDrawCommand 객체를 생성합니다.
     */
    Renderer::FMeshDrawCommand::FMeshDrawCommand(Renderer::FMaterialPipeline* Pipeline, Asset::UMaterial* Material, Asset::UStaticMesh* Mesh, const Renderer::FScenePrimitive* Primitive, Core::Uint32 FirstIndex, Core::Uint32 IndexCount)
		: mPipeline{Pipeline},
		  mMaterial{Material},
		  mMesh{Mesh},
		  mPrimitive{Primitive},
		  mFirstIndex{FirstIndex},
		  mIndexCount{IndexCount} {
    }

}
