#include "Core/LoadingStatus.h"

namespace Core {

    namespace {
        Core::LoadingStatus::ISink* GSink{};
    }

    void Core::LoadingStatus::SetSink(Core::LoadingStatus::ISink* Sink) {
        GSink = Sink;
    }

    void Core::LoadingStatus::BeginStage(std::string_view Stage, float Progress) {
        if (GSink != nullptr) {
            GSink->OnStage(Stage, Progress);
        }
    }

    void Core::LoadingStatus::ReportFile(std::string_view Action, const std::filesystem::path& File) {
        if (GSink != nullptr) {
            GSink->OnFile(Action, File);
        }
    }

}
