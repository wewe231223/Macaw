/** @file FObjViewerApplication.h @brief 프로젝트 브라우저 없이 실행하는 OBJ 뷰어입니다. @ingroup App */
#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "Editor/FImGuiUIScale.h"

#include <filesystem>

class FAssetManager;
class FMeshViewer;

/** @brief 에디터와 같은 임포터·렌더러·메시 뷰어를 사용하는 독립 실행 호스트입니다. */
class FObjViewerApplication {
public:
    /** @brief 리플렉션 소유 타입의 별칭입니다. */
    ReflectTypes(FObjViewerApplication);

public:
    /** @brief 비어 있는 독립 뷰어 호스트를 구성합니다. */
    FObjViewerApplication();
    /** @brief 뷰어, ImGui와 GPU 자원을 역순으로 정리합니다. */
    ~FObjViewerApplication();

public:
    /** @brief 정적 타입 메타데이터를 반환합니다. @return 등록된 구조체 메타데이터입니다. */
    ReflectStruct(FObjViewerApplication);
    /** @brief 독립 뷰어의 창과 이벤트 루프를 실행합니다. @return 정상 종료는 0이며 초기화 실패는 1입니다. */
    int Run();

private:
    /** @brief 생성된 리플렉션 멤버 등록 함수의 접근을 허용합니다. */
    ReflectMembers();

private:
    /** @brief 외부 메시를 뷰어 Content에 임포트하고 엽니다. @param Source 원본 OBJ 등의 경로입니다. @return 임포트와 미리보기에 성공하면 true입니다. */
    bool OpenSource(const std::filesystem::path& Source);
    /** @brief 원본 메시 파일을 선택합니다. @return 선택한 경로이며 취소하면 빈 경로입니다. */
    std::filesystem::path ChooseSource();
    /** @brief ImGui와 렌더러를 안전하게 해제합니다. */
    void Shutdown();

private:
    /** @brief 독립 실행 창과 메시지 처리기입니다. */
    FWindowsApplication mPlatform{};
    ReflectProperty(mPlatform, "Platform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 뷰어 전용 렌더러입니다. */
    FRenderer mRenderer{};
    ReflectProperty(mRenderer, "Renderer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 임포트한 뷰어 에셋의 소유 관리자입니다. */
    std::unique_ptr<FAssetManager> mAssets{};
    ReflectProperty(mAssets, "Assets", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 에디터에서도 사용하는 메시 미리보기입니다. */
    std::unique_ptr<FMeshViewer> mViewer{};
    ReflectProperty(mViewer, "Viewer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 최근 임포트 오류입니다. */
    FString mError{};
    ReflectProperty(mError, "Error", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FImGuiUIScale mUIScale{};
    ReflectProperty(mUIScale, "UIScale", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief ImGui 컨텍스트의 생성 상태입니다. */
    bool mContextCreated{};
    ReflectProperty(mContextCreated, "ContextCreated", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief Win32 백엔드 초기화 상태입니다. */
    bool mWin32Ready{};
    ReflectProperty(mWin32Ready, "Win32Ready", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief Direct3D 백엔드 초기화 상태입니다. */
    bool mDxReady{};
    ReflectProperty(mDxReady, "DxReady", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
