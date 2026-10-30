/**
 * @file FPlayInEditorSession.h
 * @brief 편집 월드 복제와 PIE 실행 세션의 수명 관리에 대한 선언입니다.
 * @ingroup Editor
 */

#pragma once

#include "Editor/FEditorContext.h"
#include "Engine/FGameViewportClient.h"
#include <utility>

namespace Engine {
    class UEngine;
}

namespace Editor {

    class FEditorViewport;

    /**
     * @brief 편집 월드 복제와 PIE 실행 세션의 수명 관리를 담당합니다.
     */
    class FPlayInEditorSession {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(FPlayInEditorSession);

    public:
        /**
         * @brief 전달받은 설정과 의존성으로 FPlayInEditorSession의 초기 상태를 구성합니다.
         * @param[in] Engine 월드 조회와 실행을 제공할 엔진의 비소유 주소입니다.
         * @param[in] Context 조회 또는 직렬화에 사용할 컨텍스트의 비소유 주소입니다.
         * @param[in] Viewport PIE 입력과 출력을 연결할 편집 뷰포트의 비소유 주소입니다.
         */
        FPlayInEditorSession(Engine::UEngine* Engine, Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport);

        /**
         * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~FPlayInEditorSession();

        /**
         * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
         */
        FPlayInEditorSession(const Editor::FPlayInEditorSession&) = delete;

        /**
         * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
         */
        Editor::FPlayInEditorSession& operator=(const Editor::FPlayInEditorSession&) = delete;

    public:
        /**
         * @brief 이 타입의 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FPlayInEditorSession);

        /**
         * @brief 편집 월드와 뷰포트 상태가 PIE 시작 조건을 만족하는지 검사합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool CanStart() const;

        /**
         * @brief 편집 월드를 복제하여 PIE를 시작하고 뷰포트 입력과 조회 대상을 연결합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Start();

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
         * @brief PIE 월드가 엔진에 유효하게 등록되어 실행 세션이 활성인지 검사합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsActive() const;

        /**
         * @brief PIE 실행 월드의 일시정지 여부를 반환합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsPaused() const;

        /**
         * @brief 클라이언트 또는 세션이 사용하는 월드의 세대 핸들을 반환합니다.
         * @return 조회한 대상의 세대 핸들이며, 대상이 없으면 무효 핸들입니다.
         */
        Engine::FWorldHandle GetWorldHandle() const;

        /**
         * @brief PIE 월드의 선택과 조회에 사용하는 편집 컨텍스트를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
         */
        Editor::FEditorContext* GetInspectionContext();

        /**
         * @brief PIE 월드의 입력과 장면 뷰를 처리하는 클라이언트를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
         */
        Engine::FGameViewportClient* GetViewportClient();

        /**
         * @brief 편집 객체에 대응하는 PIE 복제 객체의 핸들을 조회합니다.
         * @param[in] Original PIE 복제본을 조회할 원본 편집 객체의 핸들입니다.
         * @return 조회한 대상의 세대 핸들이며, 대상이 없으면 무효 핸들입니다.
         */
        CoreUObject::FObjectHandle FindPlayObject(CoreUObject::FObjectHandle Original) const;

        /**
         * @brief PIE 복제 객체에 대응하는 원본 편집 객체의 핸들을 조회합니다.
         * @param[in] Duplicate 원본을 조회할 PIE 복제 객체의 핸들입니다.
         * @return 조회한 대상의 세대 핸들이며, 대상이 없으면 무효 핸들입니다.
         */
        CoreUObject::FObjectHandle FindEditorObject(CoreUObject::FObjectHandle Duplicate) const;

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 월드 조회와 실행을 제공하는 엔진의 비소유 주소입니다.
         */
        Engine::UEngine* mEngine{};
        ReflectProperty(mEngine, "Engine", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 원본 편집 월드의 선택과 상태를 제공하는 비소유 컨텍스트입니다.
         */
        Editor::FEditorContext* mContext{};
        ReflectProperty(mContext, "Context", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief PIE 입력과 장면 출력을 연결할 편집 뷰포트의 비소유 주소입니다.
         */
        Editor::FEditorViewport* mViewport{};
        ReflectProperty(mViewport, "Viewport", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief PIE 복제 객체의 선택과 조회를 위한 별도 컨텍스트입니다.
         */
        Editor::FEditorContext mInspectionContext;
        ReflectProperty(mInspectionContext, "InspectionContext", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief PIE 월드의 활성 카메라와 입력을 처리하는 클라이언트입니다.
         */
        Engine::FGameViewportClient mGameClient;
        ReflectProperty(mGameClient, "GameClient", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 세션이 생성하여 관리하는 PIE 복제 월드의 핸들입니다.
         */
        Engine::FWorldHandle mPlayWorld{};
        ReflectProperty(mPlayWorld, "PlayWorld", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief PIE 종료 후 복원할 이전 현재 월드의 핸들입니다.
         */
        Engine::FWorldHandle mPreviousWorld{};
        ReflectProperty(mPreviousWorld, "PreviousWorld", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 원본 편집 객체와 PIE 복제 객체 핸들의 대응 목록입니다.
         */
        Core::TArray<std::pair<CoreUObject::FObjectHandle, CoreUObject::FObjectHandle>> mObjectPairs{};
        ReflectProperty(mObjectPairs, "ObjectPairs", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief PIE 시작 또는 종료 도중의 재진입을 막는 플래그입니다.
         */
        bool mTransitioning{};
        ReflectProperty(mTransitioning, "Transitioning", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 콜백 처리가 끝난 안전한 시점에 PIE를 종료할지 나타냅니다.
         */
        bool mStopRequested{};
        ReflectProperty(mStopRequested, "StopRequested", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
