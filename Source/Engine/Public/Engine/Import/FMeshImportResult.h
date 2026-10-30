/**
 * @file FMeshImportResult.h
 * @brief 정점·인덱스·서브메시와 원본 재질 목록을 함께 보관하는 메시 임포트 결과입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/FMeshData.h"
#include "Engine/Import/FMeshImportMaterial.h"

namespace Engine {

    /**
     * @brief 정점·인덱스·서브메시와 원본 재질 목록을 함께 보관하는 메시 임포트 결과입니다.
     */
    class FMeshImportResult {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FMeshImportResult);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMeshImportResult);

        /**
         * @brief 임포트 결과에 보관한 정점·인덱스·서브메시 데이터를 조회합니다.
         * @return 결과 객체가 소유하는 메시 데이터의 참조입니다.
         */
        Engine::FMeshData& GetMeshData();
        /**
         * @brief 임포트 결과에 보관한 정점·인덱스·서브메시 데이터를 조회합니다.
         * @return 결과 객체가 소유하는 메시 데이터의 참조입니다.
         */
        const Engine::FMeshData& GetMeshData() const;

        /**
         * @brief 임포트한 메시의 재질 슬롯과 대응하는 재질 목록을 조회합니다.
         * @return 결과 객체가 소유하는 재질 목록의 참조입니다.
         */
        Core::TArray<Engine::FMeshImportMaterial>& GetMaterials();
        /**
         * @brief 임포트한 메시의 재질 슬롯과 대응하는 재질 목록을 조회합니다.
         * @return 결과 객체가 소유하는 재질 목록의 참조입니다.
         */
        const Core::TArray<Engine::FMeshImportMaterial>& GetMaterials() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 임포트한 위치·법선·UV·인덱스와 서브메시·재질 슬롯입니다.
         */
        Engine::FMeshData mMeshData{};
        ReflectProperty(mMeshData, "MeshData", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메시 재질 슬롯과 대응하는 원본 재질 설정 목록입니다.
         */
        Core::TArray<Engine::FMeshImportMaterial> mMaterials{};
        ReflectProperty(mMaterials, "Materials", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
