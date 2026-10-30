/**
 * @file FPrimitiveDrawInterface.h
 * @brief 에디터 보조 선분과 메시를 프레임 단위로 수집합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FDrawLine.h"
#include "Engine/FDrawMesh.h"
#include "Engine/FDrawText.h"

namespace Core {
    struct FBox;
}

namespace Engine {

    /**
     * @brief 에디터 보조 선분과 메시를 프레임 단위로 수집합니다.
     * @ingroup Engine
     */
    class FPrimitiveDrawInterface {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FPrimitiveDrawInterface);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FPrimitiveDrawInterface);

        /**
         * @brief 주어진 좌표와 색상으로 보조 선분을 프레임 목록에 추가합니다.
         * @param[in] Start 선분 시작점입니다.
         * @param[in] End 선분 끝점입니다.
         * @param[in] Color 선분에 사용할 RGB 색상입니다.
         * @param[in] DepthTest 깊이 검사를 적용할지 지정합니다.
         * @param[in] Thickness 유한한 양수인 픽셀 단위 선 두께입니다.
         */
        void DrawLine(const Core::FVector& Start, const Core::FVector& End, const Core::FVector& Color, bool DepthTest = false, float Thickness = 1.0f);

        /**
         * @brief AABB의 열두 모서리를 지정한 픽셀 두께의 선분으로 수집합니다.
         * @param[in] Bounds 표시하거나 읽을 경계 상자입니다.
         * @param[in] Color 적용할 색상입니다.
         * @param[in] DepthTest 장면 깊이 검사를 적용할지 지정합니다.
         * @param[in] Thickness 픽셀 단위 선 두께입니다.
         */
        void DrawBox(const Core::FBox& Bounds, const Core::FVector& Color, bool DepthTest = true, float Thickness = 1.5f);

        /**
         * @brief 현재 프레임의 보조 선분과 메시 목록을 비웁니다.
         */
        void Clear();

        /**
         * @brief 깊이 검사와 우선순위를 가진 에디터 메시 그리기 요청을 추가합니다.
         * @param[in] Mesh 그릴 에디터 메시 요청입니다.
         */
        void DrawMesh(const Engine::FDrawMesh& Mesh);

        /**
         * @brief 이번 프레임의 에디터 메시 그리기 요청들을 반환합니다.
         * @return 소유 그리기 인터페이스가 유지하는 요청 배열의 참조입니다.
         */
        const Core::TArray<Engine::FDrawMesh>& GetMeshes() const;

        /**
         * @brief 이번 프레임에 표시할 텍스트 요청을 수집합니다.
         * @param[in] Text 그릴 텍스트 요청입니다.
         */
        void DrawString(const Engine::FDrawText& Text);

        /**
         * @brief 현재 프레임의 텍스트 요청 목록을 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const Core::TArray<Engine::FDrawText>& GetTexts() const;

        /**
         * @brief 현재 프레임에 수집한 보조 선분을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::TArray<Engine::FDrawLine>& GetLines() const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 현재 프레임에 수집한 보조 선분입니다.
         */
        Core::TArray<Engine::FDrawLine> mLines{};
        ReflectProperty(mLines, "Lines", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 프레임에 표시할 에디터 메시와 피킹 의미의 목록입니다.
         */
        Core::TArray<Engine::FDrawMesh> mMeshes{};
        ReflectProperty(mMeshes, "Meshes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 프레임의 텍스트 요청 목록입니다.
         */
        Core::TArray<Engine::FDrawText> mTexts{};
        ReflectProperty(mTexts, "Texts", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
