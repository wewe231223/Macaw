/**
 * @file FEditorViewport.h
 * @brief 카메라, 기즈모와 장면 타깃을 소유하는 독립 에디터 뷰포트를 정의합니다.
 * @ingroup Editor
 */

#pragma once

#include "Editor/FEditorViewportClient.h"
#include "Editor/FViewportStatistics.h"

class FSceneViewport;
class IRendererModule;
class FRenderTarget;

/**
 * @brief 자체 뷰포트 클라이언트와 장면 타깃을 소유하고 UI 표시 및 입력 전달을 담당합니다.
 * @ingroup Editor
 * @note 편집 컨텍스트와 렌더러는 비소유 의존성이며 이 뷰포트가 사용하는 동안 유지되어야 합니다.
 */
class FEditorViewport {
public:
    /**
     * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
     */
    ReflectTypes(FEditorViewport);

public:
    /**
     * @brief 공유 편집 컨텍스트와 창 이름으로 독립 뷰포트의 초기 상태를 구성합니다.
     * @param[in,out] Context 클라이언트와 기즈모가 사용할 비소유 편집 컨텍스트입니다.
     * @param[in] Name 다른 뷰포트와 구분할 ImGui 창 이름입니다.
     * @note 렌더 타깃과 ImGui 컨텍스트는 생성하지 않습니다.
     */
    explicit FEditorViewport(FEditorContext* Context, FString Name = "Viewport");
    /**
     * @brief 진행 중인 자체 드래그를 취소하고 소유한 장면 뷰포트와 타깃을 정리합니다.
     */
    ~FEditorViewport();
    /**
     * @brief 클라이언트와 타깃의 고유 소유권이 복제되지 않도록 복사 생성을 금지합니다.
     */
    FEditorViewport(const FEditorViewport&) = delete;
    /**
     * @brief 고유 소유권과 내부 비소유 연결이 대입되지 않도록 복사 할당을 금지합니다.
     */
    FEditorViewport& operator=(const FEditorViewport&) = delete;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FEditorViewport);

    /**
     * @brief ImGui 초기화와 별도로 장면 뷰포트와 초기 컬러 타깃을 준비합니다.
     * @param[in,out] Renderer 컬러 렌더 타깃을 제공할 비소유 렌더러입니다.
     * @return 준비에 성공하면 true이며 렌더러가 없거나 이미 초기화했거나 타깃 생성에 실패하면 false입니다.
     */
    bool Initialize(IRendererModule* Renderer);
    /**
     * @brief 자체 기즈모 드래그를 취소하고 장면 뷰포트와 컬러 타깃을 해제합니다.
     * @note 드래그 중이었다면 시작 변환을 복원하며 여러 번 호출해도 안전합니다.
     */
    void Shutdown();
    /**
     * @brief 툴바와 장면 이미지를 표시하고 크기 변경 및 입력을 장면 뷰포트에 전달합니다.
     * @param[in] DeltaTime 현재 프레임의 초 단위 경과 시간입니다.
     * @pre 활성 ImGui 프레임 안에서 호출해야 합니다.
     * @note 이미지가 표시되지 않을 때에도 진행 중인 드래그의 버튼 해제와 Esc 취소를 처리합니다.
     */
    void DrawUI(float DeltaTime);
    void DrawContents(float DeltaTime, bool AllowInput = true);
    bool IsMouseCaptured() const;
    void ReleaseInput();
    /**
     * @brief 클라이언트의 현재 카메라로 장면을 자체 컬러 타깃에 그립니다.
     * @note 엔진의 월드 렌더 동기화 이후 호출하며 초기화 전에는 아무 작업도 하지 않습니다.
     */
    void Draw();
    /**
     * @brief 뷰포트 이미지 영역 위에 월드와 렌더링 통계를 표시합니다.
     * @param[in] DeltaTime 이전 갱신 이후의 경과 시간이며 초 단위입니다.
     * @param[in] World 조회·설정 또는 직렬화할 월드입니다.
     * @param[in] Render 표시할 현재 프레임의 렌더링 통계입니다.
     */
    void DrawStatistics(float DeltaTime, const UWorld* World, const FRenderStats& Render);
    /**
     * @brief 이 뷰포트가 소유한 통계 표시 객체의 주소를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    FViewportStatistics* GetStatistics();
    /**
     * @brief 창 표시와 도킹 식별에 사용하는 뷰포트 이름을 반환합니다.
     * @return 이 뷰포트가 소유한 이름의 참조입니다.
     */
    const FString& GetName() const;
    /**
     * @brief 이 뷰포트의 카메라, 명령과 기즈모를 관리하는 클라이언트를 반환합니다.
     * @return 이 뷰포트가 소유한 클라이언트의 비소유 주소입니다.
     */
    FEditorViewportClient* GetClient();
    /**
     * @brief PIE 연결 상태에 따라 현재 사용할 뷰포트 클라이언트를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    FViewportClient* GetActiveClient();
    /**
     * @brief PIE에 사용할 클라이언트를 연결하거나 해제하고 입력 캡처를 초기화합니다.
     * @param[in] Client 입력과 그리기를 처리할 클라이언트의 비소유 주소입니다.
     */
    void SetPlayClient(FViewportClient* Client);
    /**
     * @brief PIE 뷰포트가 마우스와 키보드 입력을 캡처했는지 반환합니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    bool IsPlayInputCaptured() const;
    /**
     * @brief 입력, 크기 변경과 피킹 요청을 연결하는 장면 뷰포트를 반환합니다.
     * @return 소유한 장면 뷰포트의 비소유 주소이며 초기화 전이나 종료 후에는 nullptr입니다.
     */
    FSceneViewport* GetSceneViewport() const;
    /**
     * @brief 장면 이미지를 보관하는 컬러 렌더 타깃을 반환합니다.
     * @return 소유한 타깃의 비소유 주소이며 준비되지 않았으면 nullptr입니다.
     */
    FRenderTarget* GetRenderTarget() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief ImGui 창과 도킹에서 다른 뷰포트와 구분하는 이름입니다.
     */
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 이 뷰포트만의 카메라, 변환 명령과 기즈모 상태를 소유합니다.
     */
    FEditorViewportClient mClient;
    ReflectProperty(mClient, "Client", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief PIE 중 입력과 뷰를 제공하는 클라이언트의 비소유 주소입니다.
     */
    FViewportClient* mPlayClient{};
    ReflectProperty(mPlayClient, "PlayClient", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief PIE 뷰포트가 입력 캡처를 소유하는지 나타냅니다.
     */
    bool mPlayInputCaptured{};
    ReflectProperty(mPlayInputCaptured, "PlayInputCaptured", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 프레임 및 리소스 통계의 뷰포트 표시 상태입니다.
     */
    FViewportStatistics mStatistics{};
    ReflectProperty(mStatistics, "Statistics", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 뷰포트 이미지의 화면상 좌측 상단 좌표입니다.
     */
    FVector2 mImageOrigin{};
    ReflectProperty(mImageOrigin, "ImageOrigin", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 현재 표시한 뷰포트 이미지의 픽셀 크기입니다.
     */
    FVector2 mImageSize{};
    ReflectProperty(mImageSize, "ImageSize", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    ImDrawList* mDrawList{};
    ReflectProperty(mDrawList, "DrawList", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mMouseCaptured{};
    ReflectProperty(mMouseCaptured, "MouseCaptured", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 컬러 타깃을 소유하고 입력, 그리기와 CPU 피킹을 클라이언트에 연결합니다.
     */
    std::unique_ptr<FSceneViewport> mViewport{};
    ReflectProperty(mViewport, "Viewport", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
