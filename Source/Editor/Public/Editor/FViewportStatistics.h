/**
 * @file FViewportStatistics.h
 * @brief 프레임 시간, 객체, 메모리와 렌더링 통계를 표시합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace Engine {
    class UWorld;
}

namespace RenderCore {
    struct FRenderStats;
}

namespace Core {
    struct FVector2;
}

struct ImDrawList;

namespace Editor {

    /**
     * @brief 프레임 시간, 객체, 메모리와 렌더링 통계를 표시합니다.
     * @ingroup Editor
     */
    class FViewportStatistics {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FViewportStatistics);

        /**
         * @brief 뷰포트 통계의 표시 그룹을 비트 플래그로 구분합니다.
         */
        enum class EGroup : Core::Uint32 {
            /**
             * @brief 프레임 속도와 프레임 시간 통계입니다.
             */
            Fps = 1,
            /**
             * @brief 월드 액터 수와 객체 수명 통계입니다.
             */
            Scene = 2,
            /**
             * @brief CPU 및 GPU 메모리 사용량 통계입니다.
             */
            Memory = 4,
            /**
             * @brief 렌더 프리미티브·드로 호출·변환 업로드 통계입니다.
             */
            Render = 8,
            /**
             * @brief 그림자 맵·드로·아틀라스 사용량 통계 그룹입니다.
             */
            Shadow = 16,
            /**
             * @brief 프레임 작업과 주요 처리 시간 통계 그룹입니다.
             */
            Unit = 32,
            /**
             * @brief 액터·컴포넌트 실행 통계 그룹입니다.
             */
            Game = 64,
            /**
             * @brief 월드 Tick·동기화·렌더 갱신 통계 그룹입니다.
             */
            World = 128,
            /**
             * @brief Lua 실행·코루틴 통계 그룹입니다.
             */
            Script = 256,
            /**
             * @brief 에셋 로드·임포트·캐시 통계 그룹입니다.
             */
            Assets = 512,
            /**
             * @brief 지원하는 모든 통계 그룹의 비트 조합입니다.
             */
            All = 1023
        };

    public:
        /**
         * @brief FViewportStatistics의 초기 표시 상태를 구성합니다.
         */
        FViewportStatistics() = default;

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FViewportStatistics() = default;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FViewportStatistics);

        /**
         * @brief stat 그룹 표시·목록·이력 초기화 명령을 처리합니다.
         * @param[in] Command 처리할 콘솔 명령 문자열입니다.
         * @param[out] Output 명령 처리 결과 또는 사용법을 받을 문자열입니다.
         * @return stat 명령이면 구문 오류를 포함해 true이며 다른 명령이면 false입니다.
         */
        bool ExecuteCommand(const Core::FString& Command, Core::FString& Output);

        /**
         * @brief 지정한 통계 그룹이 표시되도록 설정됐는지 검사합니다.
         * @param[in] Group 확인할 통계 그룹 비트입니다.
         * @return 지정한 모든 그룹 비트가 켜져 있으면 true입니다.
         */
        bool IsEnabled(EGroup Group) const;

        /**
         * @brief 지정한 통계 그룹이 표시되도록 설정됐는지 검사합니다.
         * @param[in] Group 확인할 통계 그룹 이름입니다.
         * @return 이름이 가리키는 그룹이 켜져 있으면 true입니다.
         */
        bool IsEnabled(const Core::FString& Group) const;

        /**
         * @brief 검색 가능한 통계 표와 이력 초기화 버튼을 별도 창에 그립니다.
         * @param[in,out] Open 창을 열지 지정하며 그리기 함수에서는 닫기 동작도 반영합니다.
         */
        void DrawPanel(bool& Open);

        /**
         * @brief 하나 이상의 뷰포트 통계 그룹이 표시되도록 설정됐는지 반환합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsVisible() const;

        /**
         * @brief 뷰포트 이미지 위에 입력을 받지 않는 통계 내용을 그립니다.
         * @param[in] DeltaTime 이전 프레임 이후 경과한 시간(초)입니다.
         * @param[in] World 액터 수를 조회할 월드의 비소유 주소입니다.
         * @param[in] Render 이번 프레임의 렌더링 통계입니다.
         * @param[in] Origin 뷰포트 이미지의 화면상 좌측 상단 좌표입니다.
         * @param[in] Size 뷰포트 이미지의 가로·세로 픽셀 크기입니다.
         * @param[in,out] DrawList 오버레이를 추가할 ImGui 목록이며 nullptr이면 통계 그리기를 생략합니다.
         */
        void Draw(float DeltaTime, const Engine::UWorld* World, const RenderCore::FRenderStats& Render, const Core::FVector2& Origin, const Core::FVector2& Size, ImDrawList* DrawList = nullptr) const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 기본 표시 상태와 다르게 처리할 통계 그룹 이름 목록입니다.
         */
        Core::TArray<Core::FString> mEnabledGroups{};
        ReflectProperty(mEnabledGroups, "EnabledGroups", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 기본적으로 모든 통계 그룹을 켜고 예외 목록을 적용할지 나타냅니다.
         */
        bool mAllGroups{};
        ReflectProperty(mAllGroups, "AllGroups", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 통계 패널의 그룹·항목 이름 검색 버퍼입니다.
         */
        std::array<char, 128> mFilter{};
        ReflectProperty(mFilter, "Filter", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
