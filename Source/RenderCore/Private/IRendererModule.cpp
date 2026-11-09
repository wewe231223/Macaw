/**
 * @file IRendererModule.cpp
 * @brief 엔진과 에디터가 사용하는 렌더러의 수명 및 프레임 제출 인터페이스입니다.
 * @ingroup RenderCore
 */

#include "RenderCore/IRendererModule.h"

namespace RenderCore {

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    RenderCore::IRendererModule::~IRendererModule() = default;

    /**
     * @brief 현재 그림자 아틀라스의 검사 정보를 반환합니다.
     */
    RenderCore::FShadowAtlasView RenderCore::IRendererModule::GetShadowAtlasView() const {
        return {};
    }

    /**
     * @brief 콘솔에서 조절하는 그림자 렌더링 옵션을 반환합니다.
     */
    RenderCore::FShadowSettings RenderCore::IRendererModule::GetShadowSettings() const {
        return {};
    }

    /**
     * @brief 콘솔에서 조절하는 그림자 렌더링 옵션을 변경합니다.
     */
    void RenderCore::IRendererModule::SetShadowSettings(const RenderCore::FShadowSettings&) {
    }

    /**
     * @brief 폰트 메트릭으로 텍스트의 로컬 평면 경계를 계산합니다.
     */
    bool RenderCore::IRendererModule::GetTextBounds(const Core::FString&, Core::FVector2&, Core::FVector2&) const {
        return false;
    }

}
