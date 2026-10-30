/**
 * @file IEditorPanel.h
 * @brief 패널의 표시와 렌더 자원 수명을 정의합니다.
 */

#pragma once
#include "Render/Renderer.h"

class IEditorPanel {
public:
    virtual ~IEditorPanel() = default;

public:
    virtual void DrawPanel() = 0;

    /**
     * @brief 패널별 오프스크린 렌더링을 수행합니다.
     * @param Renderer 사용할 렌더러입니다.
     * @details 에셋 조회는 전역 서비스에서 수행합니다. 기본 구현은 아무 작업도 하지 않습니다.
     */
    virtual void RenderOffscreen(FRenderer& Renderer);

    virtual void ReleaseRenderResources();

    bool IsVisible() const;

    void SetVisible(bool BInVisible);

protected:
    bool mBVisible{true};
};
