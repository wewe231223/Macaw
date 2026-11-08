#pragma once

#include "Engine/Component/USceneComponent.h"
#include "UDecalComponent.generated.h"

namespace Asset {
    class UTexture2D;
}

namespace Engine {

    UCLASS()
    class UDecalComponent final : public Engine::USceneComponent {
        GENERATED_BODY()

    public:
        ~UDecalComponent() override;

    public:
        const Core::FString& GetTexturePath() const;
        bool SetTexturePath(const Core::FString& Path);
        const Core::FString& GetNormalMapPath() const;
        bool SetNormalMapPath(const Core::FString& Path);
        void ResolveTexture();

        float GetNormalStrength() const;
        bool SetNormalStrength(float Strength);

        const Core::FVector& GetExtent() const;
        bool SetExtent(const Core::FVector& Extent);
        Core::FMatrix GetDecalToWorld() const;

        float GetOpacity() const;
        bool SetOpacity(float Opacity);
        Core::Int32 GetSortOrder() const;
        void SetSortOrder(Core::Int32 SortOrder);

        bool SetTransform(const Core::FTransform& Transform) override;
        void OnOwnerVisibilityChanged() override;
        void UnregisterComponent() override;
        void SynchronizeDecal();

    private:
        void OnRegister() override;
        bool SetDecalTexturePath(const Core::FString& Path, bool NormalMap);
        Asset::UTexture2D* LoadTexture(const Core::FString& Path, bool NormalMap) const;

    private:
        UPROPERTY(Edit, Serialize, Name = "Texture", Getter = GetTexturePath, Setter = SetTexturePath)
        Core::FString mTexturePath{};

        UPROPERTY(Edit, Serialize, Name = "NormalMap", Getter = GetNormalMapPath, Setter = SetNormalMapPath)
        Core::FString mNormalMapPath{};

        UPROPERTY(Edit, Serialize, Getter = GetNormalStrength, Setter = SetNormalStrength)
        float mNormalStrength{1.0f};

        UPROPERTY(Edit, Serialize, Getter = GetExtent, Setter = SetExtent)
        Core::FVector mExtent{0.25f, 0.5f, 0.5f};

        UPROPERTY(Edit, Serialize, Getter = GetOpacity, Setter = SetOpacity)
        float mOpacity{1.0f};

        UPROPERTY(Edit, Serialize, Getter = GetSortOrder, Setter = SetSortOrder)
        Core::Int32 mSortOrder{};

        Asset::UTexture2D* mTexture{};
        Asset::UTexture2D* mNormalMap{};
    };

}
