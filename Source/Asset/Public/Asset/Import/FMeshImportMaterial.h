/**
 * @file FMeshImportMaterial.h
 * @brief 메시 임포트에서 읽은 재질 이름·기본색·양면 설정과 텍스처 참조입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Math/FVector4.h"
#include "Asset/Import/FMeshImportTexture.h"

namespace Asset {

    /**
     * @brief 메시 임포트에서 읽은 재질 이름·기본색·양면 설정과 텍스처 참조입니다.
     */
    class FMeshImportMaterial {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FMeshImportMaterial);

    public:
        /**
         * @brief 기본 설정으로 FMeshImportMaterial 객체를 초기화합니다.
         */
        FMeshImportMaterial() = default;
        /**
         * @brief 전달받은 설정을 보관하도록 FMeshImportMaterial 객체를 초기화합니다.
         * @param[in] Name 조회하거나 지정할 이름입니다.
         * @param[in] BaseColor 원본 재질의 기본 RGBA 색상입니다.
         * @param[in] TwoSided 원본 재질의 양면 렌더링 설정입니다.
         * @param[in] BaseColorTexture 원본 재질의 기본색 텍스처 참조입니다.
         * @param[in] NormalMapTexture 원본 재질의 노멀 맵 참조입니다.
         */
        FMeshImportMaterial(Core::FString Name, const Core::FVector4& BaseColor, bool TwoSided, Asset::FMeshImportTexture BaseColorTexture, Asset::FMeshImportTexture NormalMapTexture);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMeshImportMaterial);

        /**
         * @brief 임포트한 원본 재질 이름을 반환합니다.
         * @return 소유 객체가 보관하는 이름의 읽기 전용 문자열 참조입니다.
         */
        const Core::FString& GetName() const;
        /**
         * @brief 원본 재질에서 가져온 RGBA 기본색을 반환합니다.
         * @return 보관 중인 기본색의 읽기 전용 참조입니다.
         */
        const Core::FVector4& GetBaseColor() const;
        /**
         * @brief 원본 재질의 양면 렌더링 설정을 반환합니다.
         * @return 뒷면 제거를 끄는 재질이면 true입니다.
         */
        bool IsTwoSided() const;

        /**
         * @brief 원본 재질의 기본색 텍스처 참조를 반환합니다.
         * @return 텍스처 출처와 경로·내장 참조를 가진 읽기 전용 값 객체 참조입니다.
         */
        const Asset::FMeshImportTexture& GetBaseColorTexture() const;
        /**
         * @brief 원본 재질의 노멀 맵 참조를 반환합니다.
         * @return 텍스처 출처와 경로·내장 참조를 가진 읽기 전용 값 객체 참조입니다.
         */
        const Asset::FMeshImportTexture& GetNormalMapTexture() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 저장하거나 원본 파일에서 읽은 재질 이름입니다.
         */
        Core::FString mName{};
        ReflectProperty(mName, "Name", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 재질의 선형 RGBA 기본색입니다.
         */
        Core::FVector4 mBaseColor{1.0f, 1.0f, 1.0f, 1.0f};
        ReflectProperty(mBaseColor, "BaseColor", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 뒷면을 제거하지 않고 양면으로 그릴지 나타냅니다.
         */
        bool mTwoSided{};
        ReflectProperty(mTwoSided, "TwoSided", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 원본 재질의 기본색 텍스처 출처와 참조입니다.
         */
        Asset::FMeshImportTexture mBaseColorTexture{};
        ReflectProperty(mBaseColorTexture, "BaseColorTexture", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 원본 재질의 노멀 맵 출처와 참조입니다.
         */
        Asset::FMeshImportTexture mNormalMapTexture{};
        ReflectProperty(mNormalMapTexture, "NormalMapTexture", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
