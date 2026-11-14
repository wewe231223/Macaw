/**
 * @file FBonePreview.h
 * @brief 스켈레톤의 본·관절 표시와 선택에 사용할 도형을 생성합니다.
 * @ingroup Viewer
 */

#pragma once

#include "Core/Types.h"
#include "Asset/Import/FMeshImportBone.h"
#include "Engine/EEditorMesh.h"

namespace Viewer {

    /**
     * @brief 본 선택과 표시에 사용하는 에디터 메시 종류, 월드 행렬과 본 인덱스를 보관합니다.
     * @ingroup Viewer
     */
    class FBoneShape {
    public:

        /**
         * @brief 표시 메시 종류, 월드 행렬과 본 인덱스를 보관합니다.
         * @param Mesh 본·관절 도형을 그릴 에디터 메시 종류입니다.
         * @param World 표시 도형의 월드 변환 행렬입니다.
         * @param Bone 대응하는 스켈레톤 본 인덱스입니다.
         */
        FBoneShape(Engine::EEditorMesh Mesh, const Core::FMatrix& World, Core::Index_t Bone);

    public:

        /**
         * @brief 본 도형을 그릴 에디터 메시 종류를 반환합니다.
         * @return 본 또는 관절 표시용 에디터 메시 종류입니다.
         */
        Engine::EEditorMesh GetMesh() const;

        /**
         * @brief 본 표시 도형의 월드 변환을 반환합니다.
         * @return 도형이 보관하는 행렬의 읽기 전용 참조입니다.
         */
        const Core::FMatrix& GetWorld() const;

        /**
         * @brief 도형에 연결된 본 인덱스를 반환합니다.
         * @return 스켈레톤 본 배열의 인덱스입니다.
         */
        Core::Index_t GetBone() const;

    private:

        /**
         * @brief 본 또는 관절 도형을 그리는 에디터 메시 종류입니다.
         */
        Engine::EEditorMesh mMesh{Engine::EEditorMesh::Count};

        /**
         * @brief 도형을 월드 공간으로 옮기는 변환 행렬입니다.
         */
        Core::FMatrix mWorld{};

        /**
         * @brief 표시 도형이 대응하는 스켈레톤 본 인덱스입니다.
         */
        Core::Index_t mBone{};
    };

    Core::TArray<Viewer::FBoneShape> BuildBoneShapes(const Core::TArray<Asset::FMeshImportBone>& Bones, const Core::TArray<Core::FMatrix>& Transforms, float Radius);
}
