/**
 * @file IMeshImporter.h
 * @brief 원본 파일에서 메시 데이터와 재질 참조를 가져오는 임포터의 공통 인터페이스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>

namespace Engine {

    class FMeshImportResult;

    /**
     * @brief 원본 파일에서 메시 데이터와 재질 참조를 가져오는 임포터의 공통 인터페이스입니다.
     */
    class IMeshImporter {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(IMeshImporter);

    public:
        /**
         * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
         */
        virtual ~IMeshImporter();

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(IMeshImporter);

        /**
         * @brief 애플리케이션에서 공유하는 기본 Assimp 메시 임포터를 반환합니다.
         * @return 프로세스 수명 동안 유지되는 기본 임포터의 읽기 전용 참조입니다.
         */
        static const Engine::IMeshImporter& GetDefault();

        /**
         * @brief 원본 메시 파일을 읽어 정점·삼각형·재질 슬롯과 텍스처 참조를 수집합니다.
         * @param[in] Source 임포트할 원본 메시 파일 경로입니다.
         * @param[out] Result 가져온 메시 데이터와 재질 목록을 기록할 결과 객체입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 유효한 임포트 결과를 구성하면 true이며 실패하면 Error에 원인을 기록하고 false입니다.
         */
        virtual bool Import(const std::filesystem::path& Source, Engine::FMeshImportResult& Result, Core::FString& Error) const = 0;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
    };

}
