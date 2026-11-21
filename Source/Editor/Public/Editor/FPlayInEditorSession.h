/**
 * @file FPlayInEditorSession.h
 * @brief 편집 월드 복제와 PIE 실행 세션의 수명 관리에 대한 선언입니다.
 * @ingroup Editor
 */

#pragma once

#include "CoreUObject/TObjectRef.h"

#include "Editor/FEditorContext.h"
#include "Engine/FGameViewportClient.h"
#include "Editor/FEditorViewport.h"
#include "Engine/Engine.h"
#include <utility>

namespace Editor {

    class FPlayInEditorWindow;

    /**
     * @brief 편집 월드 복제와 PIE 실행 세션의 수명 관리를 담당합니다.
     */
    class FPlayInEditorSession {
    public:
        /**
         * @brief PIE 실행 위치·일시 정지·입력 캡처와 별도 창 크기 설정입니다.
         */
        struct FOptions {
            /**
             * @brief PIE를 별도 창에서 시작할지 나타냅니다.
             */
            bool mNewWindow{};

            /**
             * @brief 에디터 카메라 위치에서 PIE를 시작할지 나타냅니다.
             */
            bool mStartAtEditorCamera{};

            /**
             * @brief 별도 PIE 창의 클라이언트 너비이며 단위는 픽셀입니다.
             */
            Uint32 mWindowWidth{1280};

            /**
             * @brief 별도 PIE 창의 클라이언트 높이이며 단위는 픽셀입니다.
             */
            Uint32 mWindowHeight{720};

            /**
             * @brief PIE 시작 시 게임 입력을 캡처할지 지정합니다.
             */
            bool mCaptureInput{true};

            /**
             * @brief PIE를 일시 정지 상태로 시작할지 나타냅니다.
             */
            bool mStartPaused{};
        };

        enum class EMode {
            Play,
            Simulate
        };

    public:
        /**
         * @brief 전달받은 설정과 의존성으로 FPlayInEditorSession의 초기 상태를 구성합니다.
         * @param[in] Engine 월드 조회와 실행을 제공할 엔진의 비소유 주소입니다.
         * @param[in] Context PIE 원본 월드를 가진 에디터 컨텍스트입니다.
         * @param[in] Viewport PIE 입력과 출력을 연결할 편집 뷰포트의 비소유 주소입니다.
         */
        FPlayInEditorSession(Engine::UEngine* Engine, Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport);

        /**
         * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~FPlayInEditorSession();

        /**
         * @brief 소유 상태가 중복되지 않도록 복사·이동 생성을 금지합니다.
         */
        FPlayInEditorSession(const Editor::FPlayInEditorSession&) = delete;

        /**
         * @brief 소유 상태가 중복되지 않도록 대입을 금지합니다.
         */
        Editor::FPlayInEditorSession& operator=(const Editor::FPlayInEditorSession&) = delete;

    public:
        /**
         * @brief 현재 PIE 실행 옵션을 반환합니다.
         * @return 세션이 보관하는 실행 옵션입니다.
         */
        const FOptions& GetOptions() const;

        /**
         * @brief PIE가 비활성이고 창 크기가 허용 범위일 때 다음 실행 옵션을 갱신합니다.
         * @param[in] Options 다음 PIE 실행에 적용할 옵션입니다.
         * @return 옵션을 적용했으면 true입니다.
         */
        bool SetOptions(const FOptions& Options);

        /**
         * @brief PIE가 별도 창에서 실행 중인지 검사합니다.
         * @return 실행 중인 별도 PIE 창이 있으면 true입니다.
         */
        bool IsPlayingInNewWindow() const;

        /**
         * @brief 별도 PIE 창의 메시지·입력과 렌더링을 처리합니다.
         */
        void DrawPlayWindow();

        /**
         * @brief 편집 월드와 뷰포트 상태가 PIE 시작 조건을 만족하는지 검사합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool CanStart(EMode Mode = EMode::Play) const;

        /**
         * @brief 편집 월드를 복제하여 PIE를 시작하고 뷰포트 입력과 조회 대상을 연결합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         * @param[in] Input 키·마우스 상태를 조회할 공통 입력입니다.
         */
        bool Start(Core::FInput* Input = nullptr, EMode Mode = EMode::Play);

        /**
         * @brief PIE 실행을 종료하고 복제 월드와 객체 대응표를 정리한 뒤 편집 상태를 복원합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Stop();

        /**
         * @brief PIE 월드의 실행을 일시정지하거나 재개합니다.
         * @param[in] Paused true이면 일시정지하고 false이면 실행을 재개합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetPaused(bool Paused);

        /**
         * @brief 일시정지한 PIE 월드를 지정한 시간만큼 한 번 갱신합니다.
         * @param[in] DeltaTime 이전 갱신 이후의 경과 시간이며 초 단위입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Step(float DeltaTime = 1.0f / 60.0f);

        /**
         * @brief PIE 월드의 유효성을 검사하고 지연된 종료 요청을 처리합니다.
         */
        void Update();

        /**
         * @brief PIE 게임 뷰포트에 프레임 입력과 경과 시간을 전달합니다.
         * @param[in] DeltaTime 프레임에서 진행할 초 단위 시간입니다.
         * @param[in] AllowInput 게임 뷰포트 입력 처리를 허용할지 지정합니다.
         */
        void ProcessInput(float DeltaTime, bool AllowInput);

        /**
         * @brief PIE 월드가 엔진에 유효하게 등록되어 실행 세션이 활성인지 검사합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsActive() const;

        bool IsSimulating() const;

        /**
         * @brief PIE 실행 월드의 일시정지 여부를 반환합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsPaused() const;

        /**
         * @brief PIE 월드의 핸들을 반환합니다.
         * @return 조회한 대상의 세대 핸들이며, 대상이 없으면 무효 핸들입니다.
         */
        Engine::FWorldHandle GetWorldHandle() const;

        /**
         * @brief PIE 월드의 선택과 조회에 사용하는 편집 컨텍스트를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Editor::FEditorContext* GetInspectionContext();

        /**
         * @brief PIE 월드의 입력과 장면 뷰를 처리하는 클라이언트를 반환합니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Engine::FGameViewportClient* GetViewportClient();

        /**
         * @brief 편집 객체에 대응하는 PIE 복제 객체의 핸들을 조회합니다.
         * @param[in] Original PIE 복제본을 조회할 원본 편집 객체의 핸들입니다.
         * @return 조회한 대상의 세대 핸들이며, 대상이 없으면 무효 핸들입니다.
         */
        CoreUObject::TObjectRef<CoreUObject::UObject> FindPlayObject(CoreUObject::TObjectRef<CoreUObject::UObject> Original) const;

        /**
         * @brief PIE 복제 객체에 대응하는 원본 편집 객체의 핸들을 조회합니다.
         * @param[in] Duplicate 원본을 조회할 PIE 복제 객체의 핸들입니다.
         * @return 조회한 대상의 세대 핸들이며, 대상이 없으면 무효 핸들입니다.
         */
        CoreUObject::TObjectRef<CoreUObject::UObject> FindEditorObject(CoreUObject::TObjectRef<CoreUObject::UObject> Duplicate) const;

    private:
        /**
         * @brief 다음 PIE 실행에 사용할 세션 옵션입니다.
         */
        FOptions mOptions{};

        EMode mMode{EMode::Play};

        /**
         * @brief 별도 PIE 실행 창의 소유 객체입니다.
         */
        std::unique_ptr<Editor::FPlayInEditorWindow> mPlayWindow{};

        /**
         * @brief 월드 조회와 실행을 제공하는 엔진의 비소유 주소입니다.
         */
        Engine::UEngine* mEngine{};

        /**
         * @brief 원본 편집 월드의 선택과 상태를 제공하는 비소유 컨텍스트입니다.
         */
        Editor::FEditorContext* mContext{};

        /**
         * @brief PIE 입력과 장면 출력을 연결할 편집 뷰포트의 비소유 주소입니다.
         */
        Editor::FEditorViewport* mViewport{};

        /**
         * @brief PIE 복제 객체의 선택과 조회를 위한 별도 컨텍스트입니다.
         */
        Editor::FEditorContext mInspectionContext;

        /**
         * @brief PIE 월드의 활성 카메라와 입력을 처리하는 클라이언트입니다.
         */
        Engine::FGameViewportClient mGameClient;

        /**
         * @brief 게임 뷰포트로 전달할 프레임 입력의 비소유 주소입니다.
         */
        Core::FInput* mInput{};

        /**
         * @brief 세션이 생성하여 관리하는 PIE 복제 월드의 핸들입니다.
         */
        Engine::FWorldHandle mPlayWorld{};

        /**
         * @brief PIE 종료 후 복원할 이전 현재 월드의 핸들입니다.
         */
        Engine::FWorldHandle mPreviousWorld{};

        /**
         * @brief 원본 편집 객체와 PIE 복제 객체 핸들의 대응 목록입니다.
         */
        Core::TArray<std::pair<CoreUObject::TObjectRef<CoreUObject::UObject>, CoreUObject::TObjectRef<CoreUObject::UObject>>> mObjectPairs{};

        /**
         * @brief PIE 시작 또는 종료 도중의 재진입을 막는 플래그입니다.
         */
        bool mTransitioning{};

        /**
         * @brief 콜백 처리가 끝난 안전한 시점에 PIE를 종료할지 나타냅니다.
         */
        bool mStopRequested{};
    };

}
