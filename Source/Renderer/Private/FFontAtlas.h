/**
 * @file FFontAtlas.h
 * @brief MSDF 폰트 메트릭을 읽어 UTF-8 텍스트를 글리프 정점으로 변환합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FVector4.h"

#include <filesystem>

namespace Renderer {

    /**
     * @brief 글리프 삼각형의 로컬 평면 위치와 UV입니다.
     */
    struct FTextVertex {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FTextVertex);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FTextVertex);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 텍스트 정점의 2D 위치입니다.
         */
        Core::FVector2 mPosition{};
        ReflectProperty(mPosition, "Position", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스트 정점의 아틀라스 UV 좌표입니다.
         */
        Core::FVector2 mUV{};
        ReflectProperty(mUV, "UV", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief 한 코드포인트의 간격·평면 경계·아틀라스 UV 메트릭입니다.
     */
    struct FFontGlyph {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FFontGlyph);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FFontGlyph);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 다음 글리프까지의 수평 이동량입니다.
         */
        float mAdvance{};
        ReflectProperty(mAdvance, "Advance", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 글리프의 로컬 평면 경계입니다.
         */
        Core::FVector4 mPlane{};
        ReflectProperty(mPlane, "Plane", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 아틀라스 안 글리프의 UV 경계입니다.
         */
        Core::FVector4 mUV{};
        ReflectProperty(mUV, "UV", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그릴 수 있는 글리프 경계의 존재 여부입니다.
         */
        bool mHasBounds{};
        ReflectProperty(mHasBounds, "HasBounds", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief MSDF 폰트 메트릭을 읽어 UTF-8 텍스트를 글리프 정점으로 변환합니다.
     */
    class FFontAtlas {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FFontAtlas);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FFontAtlas);

        /**
         * @brief 폰트 아틀라스 JSON에서 글리프·커닝·크기 메트릭을 읽습니다.
         * @param[in] Path 읽을 폰트 아틀라스 JSON 파일의 경로입니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool Load(const std::filesystem::path& Path);

        /**
         * @brief UTF-8 문자열을 디코딩하여 글리프별 삼각형 정점을 생성합니다.
         * @param[in] Text UTF-8 문자열입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::TArray<Renderer::FTextVertex> BuildVertices(const Core::FString& Text) const;

        /**
         * @brief MSDF 거리장의 픽셀 범위를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetDistanceRange() const;

        /**
         * @brief 텍스처 가로 픽셀 수를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::Uint32 GetWidth() const;

        /**
         * @brief 텍스처 세로 픽셀 수를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::Uint32 GetHeight() const;

    private:
        /**
         * @brief UTF-8 문자열을 코드포인트 배열로 변환합니다.
         * @param[in] Text UTF-8 문자열입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        static Core::TArray<Core::Uint32> Decode(const Core::FString& Text);

        /**
         * @brief 코드포인트의 글리프를 찾고 없으면 대체 글리프를 반환합니다.
         * @param[in] CodePoint 조회할 유니코드 코드포인트입니다.
         * @return 찾은 글리프 또는 대체 글리프의 비소유 주소이며, 둘 다 없으면 nullptr입니다.
         */
        const Renderer::FFontGlyph* FindGlyph(Core::Uint32 CodePoint) const;

        /**
         * @brief 두 코드포인트 사이에 적용할 수평 간격 보정값을 반환합니다.
         * @param[in] First 첫 코드포인트입니다.
         * @param[in] Second 둘째 코드포인트입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetKerning(Core::Uint32 First, Core::Uint32 Second) const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 코드포인트별 글리프 메트릭입니다.
         */
        Core::TMap<Core::Uint32, Renderer::FFontGlyph> mGlyphs{};
        ReflectProperty(mGlyphs, "Glyphs", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 코드포인트 쌍별 수평 간격 보정입니다.
         */
        Core::TMap<Core::Uint64, float> mKerning{};
        ReflectProperty(mKerning, "Kerning", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief MSDF 거리장의 픽셀 범위입니다.
         */
        float mDistanceRange{4.0f};
        ReflectProperty(mDistanceRange, "DistanceRange", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스트 줄 사이의 기준 간격입니다.
         */
        float mLineHeight{1.0f};
        ReflectProperty(mLineHeight, "LineHeight", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스처 가로 픽셀 수입니다.
         */
        Core::Uint32 mWidth{};
        ReflectProperty(mWidth, "Width", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스처 세로 픽셀 수입니다.
         */
        Core::Uint32 mHeight{};
        ReflectProperty(mHeight, "Height", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
