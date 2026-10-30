/**
 * @file FMaterialAssetData.h
 * @brief 재질의 저장 가능한 설정을 보관하고 직렬화·참조 검증·런타임 재질 적용을 처리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector4.h"

namespace Serialization {
    class FMemoryArchive;
}

namespace Engine {

    class FAssetManager;
    class UMaterial;

    /**
     * @brief 재질의 저장 가능한 설정을 보관하고 직렬화·참조 검증·런타임 재질 적용을 처리합니다.
     */
    class FMaterialAssetData {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(FMaterialAssetData);

    public:
        /**
         * @brief 기본 설정으로 FMaterialAssetData 객체를 초기화합니다.
         */
        FMaterialAssetData();
        /**
         * @brief 전달받은 설정을 보관하도록 FMaterialAssetData 객체를 초기화합니다.
         * @param[in] Material 읽거나 갱신할 재질입니다.
         */
        explicit FMaterialAssetData(const Engine::UMaterial& Material);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMaterialAssetData);

        /**
         * @brief 버전 1 재질 설정을 바이너리 아카이브에 기록하거나 읽고 지원하지 않는 버전을 거부합니다.
         * @param[in,out] Archive 저장 또는 로드 모드와 오류 상태를 제공하는 아카이브입니다.
         */
        void Serialize(Serialization::FMemoryArchive& Archive);
        /**
         * @brief 저장한 재질 설정을 각 Setter의 검증을 거쳐 런타임 재질에 적용합니다.
         * @param[in,out] Material 읽거나 갱신할 재질입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 모든 값을 수락하면 true이며 하나라도 유효하지 않으면 Error를 기록하고 false입니다.
         */
        bool ApplyTo(Engine::UMaterial& Material, Core::FString& Error) const;
        /**
         * @brief 재질 이름과 비어 있지 않은 셰이더·텍스처 경로가 프로젝트 경계 안에 있는지 검증합니다.
         * @param[in] Assets 경로 경계를 확인하고 에셋을 가져올 프로젝트 에셋 관리자입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 이름과 모든 참조 경로가 유효하면 true이며 그렇지 않으면 false입니다.
         */
        bool ValidateReferences(const Engine::FAssetManager& Assets, Core::FString& Error) const;

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
         * @brief Content 폴더 기준 재질 셰이더의 상대 경로입니다.
         */
        Core::FString mShaderPath{};
        ReflectProperty(mShaderPath, "ShaderPath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 재질의 선형 RGBA 기본색입니다.
         */
        Core::FVector4 mBaseColor{};
        ReflectProperty(mBaseColor, "BaseColor", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 기본색 텍스처의 프로젝트 상대 경로이며 빈 값은 텍스처 없음을 나타냅니다.
         */
        Core::FString mBaseColorTexturePath{};
        ReflectProperty(mBaseColorTexturePath, "BaseColorTexturePath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 노멀 맵의 프로젝트 상대 경로이며 빈 값은 노멀 맵 없음을 나타냅니다.
         */
        Core::FString mNormalMapTexturePath{};
        ReflectProperty(mNormalMapTexturePath, "NormalMapTexturePath", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 재질의 정반사 기여도 배율입니다.
         */
        float mSpecularStrength{};
        ReflectProperty(mSpecularStrength, "SpecularStrength", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Phong 정반사 하이라이트의 지수입니다.
         */
        float mShininess{};
        ReflectProperty(mShininess, "Shininess", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 접선 공간 노멀 맵의 강도 배율입니다.
         */
        float mNormalStrength{};
        ReflectProperty(mNormalStrength, "NormalStrength", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 뒷면을 제거하지 않고 양면으로 그릴지 나타냅니다.
         */
        bool mTwoSided{};
        ReflectProperty(mTwoSided, "TwoSided", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
