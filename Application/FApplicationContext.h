/**
 * @file FApplicationContext.h
 * @brief 애플리케이션 자원의 소유 구조를 정의합니다. 공용 설정 원본은 FEditorConfigManager에서 관리합니다.
 */
#pragma once

#include <memory>
#include "Core/Channel/FMessageChannel.h"
#include "Editor/Input/FKeyboardInput.h"
#include "Editor/Input/FMouseInput.h"
#include "Editor/Panel/FEditorUIManager.h"
#include "Editor/Panel/IEditorPanel.h"
#include "Editor/View/EditorViewport.h"
#include "Editor/View/FAssetThumbnailRenderer.h"
#include "Render/Renderer.h"
#include "Editor/World/FWorldEditorContext.h"
#include "World/UWorld.h"
#include "Editor/UEditorEngine.h"

/** @brief 애플리케이션이 소유하는 엔진과 UI 자원 및 월드별 비소유 참조를 보관합니다. */
struct FApplicationContext {
    /** @brief 애플리케이션 수명의 렌더러입니다. */
    FRenderer mRenderer{};
    /** @brief 창의 마우스 입력 상태입니다. */
    FMouseInput mMouseInput{};
    /** @brief 창의 키보드 입력 상태입니다. */
    FKeyboardInput mKeyboardInput{};
    /** @brief 초기화 후 전역 접근점으로 공개되는 에디터 엔진의 소유 인스턴스입니다. */
    UEditorEngine mEngine{};
    /** @brief 엔진이 소유하는 에디터 월드의 비소유 참조입니다. */
    FWorldContext* mWorldContext{};
    /** @brief 엔진이 소유하는 편집 컨텍스트의 비소유 참조입니다. */
    FWorldEditorContext* mEditorContext{};
    /** @brief 에셋 썸네일 생성기의 소유 포인터입니다. */
    std::unique_ptr<FAssetThumbnailRenderer> mThumbnailRenderer{};
    /** @brief 입력에서 월드로 전달하는 명령 채널입니다. */
    std::unique_ptr<FMessageChannel> mWorldCommandChannel{};
    /** @brief 에디터 뷰포트의 입력과 오버레이 관리 객체입니다. */
    std::unique_ptr<EditorViewport> mEditorView{};
    /** @brief 에디터 패널 관리자의 소유 포인터입니다. */
    std::unique_ptr<FEditorUIManager> mEditorUIManager{};
    /** @brief 실행 모드에 맞는 메뉴 패널입니다. */
    std::unique_ptr<IEditorPanel> mMenuPanel{};
};
