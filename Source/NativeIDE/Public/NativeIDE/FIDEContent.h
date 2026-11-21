/**
 * @file FIDEContent.h
 * @brief 소스 파일 하나를 열어 편집하고 저장하는 소스 편집기 본문을 선언합니다.
 * @ingroup NativeIDE
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <imgui.h>
#include <ImGuiColorTextEdit/TextEditor.h>

namespace NativeIDE {
    class FSourceIntellisense;

    enum class EIDELanguage {
        Lua,
        Hlsl
    };

    /**
     * @brief 소스 파일 하나를 텍스트 편집기로 표시하고 디스크와 동기화합니다.
     * @ingroup NativeIDE
     * @note 원본의 UTF-8 BOM과 줄 끝 형식을 저장 시 보존하며, 보존할 수 없는 내용이 있으면 읽기 전용으로 엽니다.
     */
    class FIDEContent {
    public:
        struct FDiagnostic {
            std::size_t mLine{};
            std::string mMessage{};
            bool mWarning{};
        };

        struct FBuildResult {
            bool mSucceeded{};
            std::string mOutput{};
            std::vector<FDiagnostic> mDiagnostics{};
        };

        using FValidator = std::function<std::future<FBuildResult>(const std::filesystem::path&, std::string_view)>;
        using FBuildSavedHandler = std::function<std::string()>;

    public:
        /**
         * @brief 텍스트 편집기, 언어별 인텔리센스와 한글을 병합한 편집기 글꼴을 준비합니다.
         * @pre 활성 ImGui 컨텍스트가 있어야 하며 글꼴 아틀라스에 글꼴을 추가할 수 있어야 합니다.
         */
        explicit FIDEContent(NativeIDE::EIDELanguage Language = NativeIDE::EIDELanguage::Lua, NativeIDE::FIDEContent::FValidator Validator = {}, NativeIDE::FIDEContent::FBuildSavedHandler BuildSavedHandler = {});

        /**
         * @brief 편집기와 인텔리센스를 해제합니다.
         */
        ~FIDEContent();

    public:
        /**
         * @brief 스크립트 파일을 읽어 편집기에 표시하고 인텔리센스 문서를 엽니다.
         * @param[in] Path 열 스크립트 파일 경로입니다.
         * @param[in] Workspace 언어 서버의 작업 공간 루트로 사용할 스크립트 폴더입니다.
         * @return 파일을 읽었으면 true이며 실패하면 기존 상태를 유지하고 false입니다.
         * @post 성공하면 변경 내용 없음 상태가 되며 이진 데이터처럼 보존할 수 없는 내용은 읽기 전용으로 표시합니다.
         */
        bool Open(const std::filesystem::path& Path, const std::filesystem::path& Workspace);

        /**
         * @brief 변경 내용을 원본 BOM과 줄 끝 형식으로 변환해 원자적으로 저장합니다.
         * @return 저장했거나 저장할 변경이 없으면 true이며 실패하면 false입니다.
         * @note 열어 둔 이후 디스크의 파일이 바뀌었으면 덮어쓰지 않고 실패하며 오류 문장을 탭에 표시합니다.
         */
        bool Save();
        void Build();

        /**
         * @brief 인텔리센스 문서를 닫고 저장하지 않은 변경을 버립니다.
         */
        void Close();

        /**
         * @brief 마지막으로 연 스크립트 파일 경로를 반환합니다.
         * @return 열린 파일 경로이며 연 적이 없으면 빈 경로입니다.
         */
        const std::filesystem::path& GetPath() const;

        /**
         * @brief 저장하지 않은 변경이 있는지 검사합니다.
         * @return 편집기 내용이 마지막 저장 내용과 다르면 true입니다.
         */
        bool IsDirty() const;
        bool HasUnbuiltChanges() const;

        /**
         * @brief 인텔리센스를 갱신하고 상태 문장과 편집기를 현재 ImGui 창에 그립니다.
         * @pre 활성 ImGui 프레임과 창 안에서 호출해야 합니다.
         */
        void Draw();

    private:
        void UpdateBuild();
        void DrawBuildOutput(float Height);

        /**
         * @brief 편집기의 전체 내용을 LF 줄 끝 문자열로 반환합니다.
         * @return 편집기 전체 텍스트입니다.
         */
        std::string GetSourceText() const;

        /**
         * @brief 변경이 있으면 편집기를 마지막 저장 내용으로 되돌리고 저장 오류를 지웁니다.
         */
        void DiscardChanges();

    private:
        /**
         * @brief 스크립트를 표시하고 편집하는 텍스트 편집기입니다.
         */
        std::unique_ptr<TextEditor> mEditor{};

        /**
         * @brief mEditor에 자동 완성과 진단을 제공하는 인텔리센스입니다.
         */
        std::unique_ptr<NativeIDE::FSourceIntellisense> mIntellisense{};

        /**
         * @brief 편집기에 사용하는 기본 글꼴이며 ImGui 글꼴 아틀라스가 소유합니다.
         */
        ImFont* mFont{};

        /**
         * @brief 열린 스크립트 파일 경로입니다.
         */
        std::filesystem::path mPath{};

        /**
         * @brief 마지막으로 읽거나 저장한 디스크 원본 바이트이며 외부 변경 감지에 사용합니다.
         */
        std::string mSavedSource{};

        /**
         * @brief mSavedSource에서 BOM을 빼고 줄 끝을 LF로 바꾼 텍스트이며 변경 여부 비교에 사용합니다.
         */
        std::string mSavedText{};

        /**
         * @brief 원본 파일의 줄 끝 문자열이며 저장 시 LF를 이 형식으로 되돌립니다.
         */
        std::string_view mLineEnding{"\n"};

        /**
         * @brief 마지막 저장 실패 이유이며 비어 있으면 표시하지 않습니다.
         */
        std::string mSaveError{};
        std::string mBuiltText{};
        std::string mBuildOutput{};
        NativeIDE::FIDEContent::FBuildSavedHandler mBuildSavedHandler{};
        float mBuildPanelRatio{0.30f};
        std::future<NativeIDE::FIDEContent::FBuildResult> mBuild{};
        std::uint64_t mRevision{};
        std::uint64_t mBuildRevision{};
        bool mCurrentBuild{};
        bool mUnbuilt{};

        /**
         * @brief 저장하지 않은 변경이 있는지 나타냅니다.
         */
        bool mDirty{};
    };
}
