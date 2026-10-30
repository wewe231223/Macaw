/**
 * @file FAssimpMeshImporter.h
 * @brief Assimp로 원본 메시를 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Import/IMeshImporter.h"

namespace Engine {

    /**
     * @brief Assimp로 원본 메시를 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
     */
    class FAssimpMeshImporter final : public Engine::IMeshImporter {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FAssimpMeshImporter, Engine::IMeshImporter);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FAssimpMeshImporter);

        /**
         * @brief 원본 메시 파일을 읽어 정점·삼각형·재질 슬롯과 텍스처 참조를 수집합니다.
         * @param[in] Source 임포트할 원본 메시 파일 경로입니다.
         * @param[out] Result 가져온 메시 데이터와 재질 목록을 기록할 결과 객체입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 유효한 임포트 결과를 구성하면 true이며 실패하면 Error에 원인을 기록하고 false입니다.
         */
        bool Import(const std::filesystem::path& Source, Engine::FMeshImportResult& Result, Core::FString& Error) const override;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
    };

}
