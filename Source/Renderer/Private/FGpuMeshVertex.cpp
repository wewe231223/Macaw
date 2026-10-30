/**
 * @file FGpuMeshVertex.cpp
 * @brief GPU 입력 레이아웃에 맞춘 24바이트 위치·법선 정점입니다.
 * @ingroup Renderer
 */

#include "FGpuMeshVertex.h"
#include "Asset/FMeshVertex.h"

namespace Renderer {

    /**
     * @brief FGpuMeshVertex의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Renderer::FGpuMeshVertex::FGpuMeshVertex(const Asset::FMeshVertex& Vertex)
		: mPosition{Vertex.mPosition.ToArray()},
		  mNormal{Vertex.mNormal.ToArray()} {
    }

}
