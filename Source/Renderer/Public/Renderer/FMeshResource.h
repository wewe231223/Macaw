/**
 * @file FMeshResource.h
 * @brief GPU 정점·인덱스 버퍼와 인덱스 개수 및 메모리 크기입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <d3d11.h>
#include <wrl/client.h>

namespace Renderer {

    /**
     * @brief GPU 정점·인덱스 버퍼와 인덱스 개수 및 메모리 크기입니다.
     * @ingroup Renderer
     */
    struct FMeshResource {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FMeshResource);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMeshResource);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief GPU 정점 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mVertices{};
        ReflectProperty(mVertices, "Vertices", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief GPU 인덱스 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mIndices{};
        ReflectProperty(mIndices, "Indices", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 드로 호출에 사용할 인덱스 개수입니다.
         */
        Core::Uint32 mIndexCount{};
        ReflectProperty(mIndexCount, "IndexCount", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 바이트 저장소 또는 사용량입니다.
         */
        std::size_t mBytes{};
        ReflectProperty(mBytes, "Bytes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
