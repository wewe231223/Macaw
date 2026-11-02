/**
 * @file FPassTarget.h
 * @brief 패스가 그릴 색상·깊이 뷰와 래스터 영역을 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <d3d11.h>
#include <optional>

namespace Renderer {

    /**
     * @brief 패스가 그릴 색상·깊이 뷰와 래스터 영역입니다.
     */
    struct FPassTarget {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FPassTarget);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FPassTarget);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 색상을 기록할 렌더 타깃 뷰이며, 깊이 전용 패스는 비어 있습니다.
         */
        ID3D11RenderTargetView* mColor{};
        ReflectProperty(mColor, "Color", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이 검사와 기록에 사용할 깊이·스텐실 뷰입니다.
         */
        ID3D11DepthStencilView* mDepth{};
        ReflectProperty(mDepth, "Depth", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 클립 공간을 타깃 픽셀로 옮기는 뷰포트입니다.
         */
        D3D11_VIEWPORT mViewport{};
        ReflectProperty(mViewport, "Viewport", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 아틀라스 타일처럼 기록 영역을 제한할 시저 사각형이며, 없으면 제한하지 않습니다.
         */
        std::optional<D3D11_RECT> mScissor{};
        ReflectProperty(mScissor, "Scissor", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
