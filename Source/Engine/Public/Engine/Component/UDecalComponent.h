/**
 * @file UDecalComponent.h
 * @brief 박스 영역에 색상·노멀 텍스처를 투영하고 데칼 프록시를 장면과 동기화합니다.
 * @ingroup Engine
 */

#pragma once

#include "CoreUObject/TObjectRef.h"

#include "Engine/Component/USceneComponent.h"
#include "UDecalComponent.generated.h"

namespace Asset {
    class UTexture2D;
}

namespace Engine {

    /**
     * @brief 박스 영역에 색상·노멀 텍스처를 투영하고 데칼 프록시를 장면과 동기화합니다.
     * @ingroup Engine
     */
    UCLASS()
    class UDecalComponent final : public Engine::USceneComponent {
        GENERATED_BODY()

    public:

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UDecalComponent() override;

    public:

        /**
         * @brief 데칼 색상 텍스처의 에셋 경로를 반환합니다.
         * @return Content 기준 경로의 읽기 전용 참조입니다.
         */
        const Core::FString& GetTexturePath() const;

        /**
         * @brief 데칼 색상 텍스처의 상대 경로를 검증하여 설정합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @return 유효한 경로이며 필요한 에셋 로드가 성공하면 true입니다.
         */
        bool SetTexturePath(const Core::FString& Path);

        /**
         * @brief 데칼 노멀 텍스처의 에셋 경로를 반환합니다.
         * @return Content 기준 경로의 읽기 전용 참조입니다.
         */
        const Core::FString& GetNormalMapPath() const;

        /**
         * @brief 데칼 노멀 텍스처의 상대 경로를 검증하여 설정합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @return 유효한 경로이며 필요한 에셋 로드가 성공하면 true입니다.
         */
        bool SetNormalMapPath(const Core::FString& Path);

        /**
         * @brief 저장된 색상·노멀 경로를 해석하고 장면 데칼을 동기화합니다.
         */
        void ResolveTexture();

        /**
         * @brief 데칼 노멀 혼합 강도를 반환합니다.
         * @return 0~8 범위의 강도입니다.
         */
        float GetNormalStrength() const;

        /**
         * @brief 0~8의 유한한 노멀 강도를 설정하고 데칼을 동기화합니다.
         * @param Strength 0~8 범위의 유한한 노멀 혼합 강도입니다.
         * @return 강도가 허용 범위이면 true입니다.
         */
        bool SetNormalStrength(float Strength);

        /**
         * @brief 데칼 투영 박스의 로컬 반크기를 반환합니다.
         * @return 컴포넌트가 보관하는 반크기의 읽기 전용 참조입니다.
         */
        const Core::FVector& GetExtent() const;

        /**
         * @brief 유한한 양의 박스 반크기를 설정하고 데칼을 동기화합니다.
         * @param Extent 모든 성분이 유한한 양수인 투영 박스 반크기입니다.
         * @return 모든 축이 유효하면 true입니다.
         */
        bool SetExtent(const Core::FVector& Extent);

        /**
         * @brief 박스 반크기와 컴포넌트 월드 변환으로 데칼 투영 행렬을 만듭니다.
         * @return 정규화된 데칼 박스에서 월드 공간으로의 변환입니다.
         */
        Core::FMatrix GetDecalToWorld() const;

        /**
         * @brief 데칼 불투명도를 반환합니다.
         * @return 0~1 범위의 불투명도입니다.
         */
        float GetOpacity() const;

        /**
         * @brief 0~1의 유한한 불투명도를 설정하고 데칼을 동기화합니다.
         * @param Opacity 0~1 범위의 유한한 불투명도입니다.
         * @return 불투명도가 허용 범위이면 true입니다.
         */
        bool SetOpacity(float Opacity);

        /**
         * @brief 데칼 겹침 정렬에 사용하는 순서를 반환합니다.
         * @return 현재 정렬 순서 값입니다.
         */
        Core::Int32 GetSortOrder() const;

        /**
         * @brief 겹치는 데칼의 정렬 순서를 설정하고 장면을 동기화합니다.
         * @param SortOrder 겹친 데칼의 정렬 순서입니다.
         */
        void SetSortOrder(Core::Int32 SortOrder);

        bool SetTransform(const Core::FTransform& Transform) override;
        void OnOwnerVisibilityChanged() override;
        void UnregisterComponent() override;

        /**
         * @brief 표시 조건과 역행렬을 검증하여 장면의 데칼 프록시를 등록하거나 제거합니다.
         */
        void SynchronizeDecal();

    private:
        void OnRegister() override;

        /**
         * @brief 절대·상위 경로를 거부하고 텍스처를 로드한 뒤 경로와 참조를 교체합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param NormalMap 노멀 텍스처이면 true, 색상 텍스처이면 false입니다.
         * @return 경로 검증과 필요한 로드에 성공하면 true입니다.
         */
        bool SetDecalTexturePath(const Core::FString& Path, bool NormalMap);

        /**
         * @brief 데칼 텍스처를 색상 또는 노멀 용도에 맞게 로드합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param NormalMap 노멀 텍스처이면 true, 색상 텍스처이면 false입니다.
         * @return 에셋 관리자가 소유하는 텍스처이며 경로가 없거나 실패하면 nullptr입니다.
         */
        Asset::UTexture2D* LoadTexture(const Core::FString& Path, bool NormalMap) const;

    private:

        /**
         * @brief 데칼 색상 텍스처의 Content 기준 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "Texture", Getter = GetTexturePath, Setter = SetTexturePath)
        Core::FString mTexturePath{};

        /**
         * @brief 데칼 노멀 텍스처의 Content 기준 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "NormalMap", Getter = GetNormalMapPath, Setter = SetNormalMapPath)
        Core::FString mNormalMapPath{};

        /**
         * @brief 데칼 노멀 혼합 강도이며 허용 범위는 0~8입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetNormalStrength, Setter = SetNormalStrength)
        float mNormalStrength{1.0f};

        /**
         * @brief 로컬 투영 박스의 양의 반크기입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetExtent, Setter = SetExtent)
        Core::FVector mExtent{0.25f, 0.5f, 0.5f};

        /**
         * @brief 데칼 불투명도이며 범위는 0~1입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetOpacity, Setter = SetOpacity)
        float mOpacity{1.0f};

        /**
         * @brief 겹치는 데칼을 정렬할 순서 값입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetSortOrder, Setter = SetSortOrder)
        Core::Int32 mSortOrder{};

        /**
         * @brief 에셋 관리자가 소유하는 데칼 색상 텍스처입니다.
         */
        CoreUObject::TObjectRef<Asset::UTexture2D> mTexture{};

        /**
         * @brief 에셋 관리자가 소유하는 데칼 노멀 텍스처입니다.
         */
        CoreUObject::TObjectRef<Asset::UTexture2D> mNormalMap{};
    };

}
