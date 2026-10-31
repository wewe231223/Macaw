/**
 * @file FFBXLoader.h
 * @brief Autodesk FBX SDK로 원본 FBX 장면을 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Import/IMeshImporter.h"
#include "Asset/Import/FFBXImageProcessor.h"

#include <fbxsdk.h>

namespace Asset {

    /**
     * @brief Autodesk FBX SDK로 원본 FBX 장면을 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
     */
    class FFBXLoader final : public Asset::IMeshImporter {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FFBXLoader, Asset::IMeshImporter);

    private:
        /**
         * @brief FBX 재질과 임포트 결과 재질 목록 인덱스의 대응입니다.
         */
        using FMaterialIndices = Core::TMap<fbxsdk::FbxSurfaceMaterial*, Core::Uint32>;

    public:
        /**
         * @brief 가져올 원본 FBX 파일 경로를 보관하도록 임포터를 초기화합니다.
         * @param[in] Source 가져올 원본 FBX 파일 경로입니다.
         */
        explicit FFBXLoader(std::filesystem::path Source);
        /**
         * @brief FBX SDK 관리자와 그 관리자가 소유한 임포터·장면을 해제합니다.
         */
        ~FFBXLoader() override;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FFBXLoader);

    private:
        /**
         * @brief FBX 장면의 노드 메시를 순회해 정점·삼각형·재질 슬롯과 텍스처 참조를 수집합니다.
         * @return 유효한 임포트 결과를 구성하면 true이며 실패하면 Error에 원인을 기록하고 false입니다.
         */
        bool ImportSource() override;

        /**
         * @brief 레이어 요소의 매핑·참조 모드에 따라 다각형 정점의 값을 읽습니다.
         * @tparam TValue 레이어 요소가 보관하는 값 형식입니다.
         * @param[in] Element 값을 읽을 레이어 요소이며 nullptr이면 Value를 바꾸지 않습니다.
         * @param[in] ControlPoint 다각형 정점의 제어점 인덱스입니다.
         * @param[in] Polygon 다각형 인덱스입니다.
         * @param[in] PolygonVertex 메시 전체 기준 다각형 정점 인덱스입니다.
         * @param[out] Value 읽은 값을 기록할 대상입니다.
         * @return 값을 읽었거나 요소가 없으면 true이며 지원하지 않는 모드이거나 인덱스가 범위를 벗어나면 false입니다.
         */
        template <typename TValue>
        bool ReadElement(const fbxsdk::FbxLayerElementTemplate<TValue>* Element, int ControlPoint, int Polygon, int PolygonVertex, TValue& Value) const;

        /**
         * @brief 행렬의 이동 성분을 제외하고 방향 벡터를 변환합니다.
         * @param[in] Matrix 적용할 FBX 아핀 행렬입니다.
         * @param[in] Vector 변환할 방향 벡터입니다.
         * @return 변환한 방향 벡터이며 정규화하지 않습니다.
         */
        Core::FVector TransformDirection(const fbxsdk::FbxAMatrix& Matrix, const fbxsdk::FbxVector4& Vector) const;

        /**
         * @brief FBX 재질의 기본 색상·불투명도·양면 여부와 기본 색상·노멀 텍스처를 읽어 재질 슬롯과 재질 목록에 추가합니다.
         * @param[in] Material 읽을 FBX 재질이며 nullptr이면 기본 재질을 추가합니다.
         * @return 성공하면 true이며 텍스처를 읽지 못하거나 색상이 유효하지 않으면 Error에 원인을 기록하고 false입니다.
         */
        bool ReadMaterial(fbxsdk::FbxSurfaceMaterial* Material);
        /**
         * @brief FBX 재질의 결과 재질 인덱스를 찾고 처음 만난 재질이면 읽어서 등록합니다.
         * @param[in] Material 인덱스를 찾을 FBX 재질이며 nullptr이면 기본 재질을 뜻합니다.
         * @param[out] Index 결과 재질 목록에서의 인덱스를 기록할 대상입니다.
         * @return 인덱스를 구하면 true이며 새 재질을 읽지 못하면 false입니다.
         */
        bool FindMaterialIndex(fbxsdk::FbxSurfaceMaterial* Material, Core::Uint32& Index);
        /**
         * @brief 노드의 전역 변환을 적용해 FBX 메시의 정점·노멀·탄젠트·UV를 결과 메시 데이터에 추가하고 재질별 섹션을 구성합니다.
         * @param[in] Node 메시를 소유하며 전역 변환을 제공하는 FBX 노드입니다.
         * @param[in] Mesh 읽을 삼각형화된 FBX 메시입니다.
         * @return 성공하면 true이며 버퍼 한계를 넘거나 정점 요소가 유효하지 않으면 Error에 원인을 기록하고 false입니다.
         */
        bool ReadMesh(fbxsdk::FbxNode& Node, fbxsdk::FbxMesh& Mesh);
        /**
         * @brief 노드와 모든 자식 노드를 깊이 우선으로 순회하며 메시를 읽습니다.
         * @param[in] Node 순회를 시작할 FBX 노드입니다.
         * @return 모든 메시를 읽으면 true이며 하나라도 실패하면 false입니다.
         */
        bool ReadNode(fbxsdk::FbxNode& Node);

        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 임포터와 장면을 포함한 FBX SDK 객체의 수명을 소유하는 관리자입니다.
         */
        fbxsdk::FbxManager* mManager{};
        ReflectProperty(mManager, "Manager", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief mManager가 소유하는 원본 FBX 파일 임포터의 비소유 주소입니다.
         */
        fbxsdk::FbxImporter* mImporter{};
        ReflectProperty(mImporter, "Importer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief mManager가 소유하는 읽어 들인 FBX 장면의 비소유 주소입니다.
         */
        fbxsdk::FbxScene* mScene{};
        ReflectProperty(mScene, "Scene", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 파일 텍스처를 찾아 형태를 판별하고 재질 슬롯 정책을 적용하는 이미지 처리기입니다.
         */
        Asset::FFBXImageProcessor mImageProcessor{};
        ReflectProperty(mImageProcessor, "ImageProcessor", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이미 읽은 FBX 재질의 결과 재질 인덱스 캐시입니다.
         */
        FMaterialIndices mMaterialIndices{};
        ReflectProperty(mMaterialIndices, "MaterialIndices", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}

#include "Asset/Import/FFBXLoaderTemplates.h"
