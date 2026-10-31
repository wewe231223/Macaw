/**
 * @file LoadingStatus.h
 * @brief 긴 초기화 작업의 단계와 처리 중인 파일을 로딩 화면에 알립니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <filesystem>
#include <string_view>

namespace Core {

    /**
     * @brief 프로젝트 열기처럼 메인 스레드를 오래 점유하는 작업의 진행 상태를 전달합니다.
     * @ingroup Core
     */
    namespace LoadingStatus {
        /**
         * @brief 로딩 단계와 파일 보고를 받아 화면에 표시하는 수신자 인터페이스입니다.
         */
        class ISink {
        public:
            /**
             * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
             */
            ReflectTypes(ISink);

        public:
            /**
             * @brief 객체 수명을 끝냅니다.
             */
            virtual ~ISink() = default;

        public:
            /**
             * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
             * @return 등록된 타입 메타데이터의 비소유 주소입니다.
             */
            ReflectStruct(ISink);

            /**
             * @brief 새 로딩 단계가 시작되었음을 받습니다.
             * @param[in] Stage 사용자에게 표시할 단계 이름입니다.
             * @param[in] Progress 단계 시작 시점의 전체 진행률이며 0~1 범위입니다.
             */
            virtual void OnStage(std::string_view Stage, float Progress) = 0;

            /**
             * @brief 현재 단계에서 처리하기 시작한 파일을 받습니다.
             * @param[in] Action 파일에 수행하는 작업 이름입니다.
             * @param[in] File 처리 중인 파일 경로입니다.
             */
            virtual void OnFile(std::string_view Action, const std::filesystem::path& File) = 0;

        private:
            /**
             * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
             */
            ReflectMembers();
        };

        /**
         * @brief 보고를 받을 수신자를 교체합니다.
         * @param[in] Sink 새 수신자의 비소유 주소이며 nullptr이면 보고를 무시합니다.
         */
        void SetSink(Core::LoadingStatus::ISink* Sink);

        /**
         * @brief 새 로딩 단계의 시작을 알립니다.
         * @param[in] Stage 사용자에게 표시할 단계 이름입니다.
         * @param[in] Progress 단계 시작 시점의 전체 진행률이며 0~1 범위입니다.
         */
        void BeginStage(std::string_view Stage, float Progress);

        /**
         * @brief 처리하기 시작한 파일을 알립니다.
         * @param[in] Action 파일에 수행하는 작업 이름입니다.
         * @param[in] File 처리 중인 파일 경로입니다.
         */
        void ReportFile(std::string_view Action, const std::filesystem::path& File);
    }

}
