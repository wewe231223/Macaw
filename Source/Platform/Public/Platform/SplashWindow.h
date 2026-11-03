/**
 * @file SplashWindow.h
 * @brief 프로젝트를 여는 동안 별도 스레드에서 갱신되는 스플래시 창을 표시합니다.
 * @ingroup Platform
 */

#pragma once

#include "Core/LoadingStatus.h"
#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <Windows.h>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

namespace Platform {

    /**
     * @brief 메인 스레드가 로딩으로 막힌 동안 자체 스레드에서 단계·파일·진행률을 그리는 스플래시 창입니다.
     * @ingroup Platform
     */
    class FSplashWindow final : public Core::LoadingStatus::ISink {
    public:
        /**
         * @brief 표시하지 않은 상태로 스플래시 창을 구성합니다.
         */
        FSplashWindow();

        /**
         * @brief 창이 남아 있으면 닫고 스레드를 정리합니다.
         */
        ~FSplashWindow() override;

        /**
         * @brief 스레드와 창을 소유하므로 복사를 금지합니다.
         */
        FSplashWindow(const Platform::FSplashWindow&) = delete;

        /**
         * @brief 스레드와 창을 소유하므로 대입을 금지합니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Platform::FSplashWindow& operator=(const Platform::FSplashWindow&) = delete;

    public:
        /**
         * @brief 스플래시 스레드를 시작하고 첫 화면이 그려질 때까지 기다립니다.
         * @param[in] Anchor 창을 가운데에 놓을 모니터를 고르는 기준 창입니다.
         * @param[in] ProjectName 화면에 표시할 프로젝트 이름입니다.
         * @param[in] ProjectDirectory 파일 경로를 상대 경로로 줄일 기준 폴더입니다.
         */
        void Show(HWND Anchor,std::wstring_view ProjectName, const std::filesystem::path& ProjectDirectory);

        /**
         * @brief 진행률을 끝까지 채우는 애니메이션이 끝날 때까지 기다립니다. 창은 남겨 둡니다.
         */
        void Finish();

        /**
         * @brief 창을 파괴하고 스플래시 스레드가 끝날 때까지 기다립니다.
         */
        void Close();

        /**
         * @brief 표시할 로딩 단계와 진행률을 교체합니다.
         * @param[in] Stage 사용자에게 표시할 단계 이름입니다.
         * @param[in] Progress 단계 시작 시점의 전체 진행률이며 0~1 범위입니다.
         */
        void OnStage(std::string_view Stage, float Progress) override;

        /**
         * @brief 표시할 처리 중 파일을 교체합니다.
         * @param[in] Action 파일에 수행하는 작업 이름입니다.
         * @param[in] File 처리 중인 파일 경로입니다.
         */
        void OnFile(std::string_view Action, const std::filesystem::path& File) override;

    private:
        /**
         * @brief 스플래시 창을 만들고 닫기 요청까지 메시지 처리와 그리기를 반복합니다.
         * @param[in] Monitor 창을 가운데에 놓을 모니터입니다.
         */
        void ThreadMain(HMONITOR Monitor);

    private:
        /**
         * @brief 창을 만들고 그리는 스플래시 전용 스레드입니다.
         */
        std::thread mThread{};

        /**
         * @brief 메인 스레드와 스플래시 스레드가 공유하는 아래 상태를 보호합니다.
         */
        std::mutex mMutex{};

        /**
         * @brief 첫 화면 표시와 완료 애니메이션 종료를 메인 스레드에 알립니다.
         */
        std::condition_variable mSignal{};

        /**
         * @brief 파일 경로를 상대 경로로 줄일 프로젝트 폴더입니다. 메인 스레드만 읽습니다.
         */
        std::filesystem::path mProjectDirectory{};

        /**
         * @brief 화면에 표시할 프로젝트 이름입니다.
         */
        std::wstring mProjectName{};

        /**
         * @brief 현재 로딩 단계 이름입니다.
         */
        std::wstring mStage{};

        /**
         * @brief 현재 파일에 수행하는 작업 이름입니다.
         */
        std::wstring mAction{};

        /**
         * @brief 처리 중인 파일의 표시용 경로입니다.
         */
        std::wstring mFile{};

        /**
         * @brief 진행 막대가 따라갈 목표 진행률입니다.
         */
        float mProgress{};

        /**
         * @brief 스플래시 스레드가 창 생성을 마쳤는지 여부입니다.
         */
        bool mStarted{};

        /**
         * @brief 진행률을 끝까지 채우도록 요청했는지 여부입니다.
         */
        bool mFinishRequested{};

        /**
         * @brief 완료 애니메이션이 끝났는지 여부입니다.
         */
        bool mFinished{};

        /**
         * @brief 창 파괴를 요청했는지 여부입니다.
         */
        bool mCloseRequested{};
    };

}
