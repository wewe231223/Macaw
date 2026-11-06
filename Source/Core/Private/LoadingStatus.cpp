/**
 * @file LoadingStatus.cpp
 * @brief 로딩 진행 메시지를 등록된 수신기에 전달합니다.
 * @ingroup Core
 */

#include "Core/LoadingStatus.h"

namespace Core {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 로딩 진행 메시지를 받을 비소유 수신기입니다.
         */
        Core::LoadingStatus::ISink* GSink{};
    }

    /**
     * @brief 보고를 받을 수신자를 교체합니다.
     */
    void Core::LoadingStatus::SetSink(Core::LoadingStatus::ISink* Sink) {
        GSink = Sink;
    }

    /**
     * @brief 새 로딩 단계의 시작을 알립니다.
     */
    void Core::LoadingStatus::BeginStage(std::string_view Stage, float Progress) {
        if (GSink != nullptr) {
            GSink->OnStage(Stage, Progress);
        }
    }

    /**
     * @brief 처리하기 시작한 파일을 알립니다.
     */
    void Core::LoadingStatus::ReportFile(std::string_view Action, const std::filesystem::path& File) {
        if (GSink != nullptr) {
            GSink->OnFile(Action, File);
        }
    }

}
