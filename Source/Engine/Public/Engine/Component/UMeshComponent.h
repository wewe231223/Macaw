#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "UMeshComponent.generated.h"

namespace Asset {
    class UMaterial;
}

namespace Engine {

    UCLASS(Abstract)
    class UMeshComponent : public Engine::UPrimitiveComponent {
        GENERATED_BODY()

    public:
        UMeshComponent() = default;
        ~UMeshComponent() override = default;

    public:
        virtual bool ResolveMesh() = 0;

        const Core::FString& GetMaterialPath() const;
        bool SetMaterialPath(const Core::FString& Path);
        Asset::UMaterial* GetMaterial() const;
        bool ResolveMaterial();

        Asset::UMaterial* GetSlotMaterial(Core::Uint32 Slot) const;
        const Core::FString& GetSlotMaterialPath(Core::Uint32 Slot) const;
        bool SetSlotMaterialPath(Core::Uint32 Slot, const Core::FString& Path);
        void ClearMaterialOverrides();
        const Core::TArray<Core::FString>& GetSlotMaterialPaths() const;
        bool SetSlotMaterialPaths(const Core::TArray<Core::FString>& Paths);

        Engine::FPrimitiveSceneProxy CreateSceneProxy() const override;
        void UnregisterComponent() override;

    private:
        void OnRegister() override;
        Asset::UMaterial* LoadMaterial(const Core::FString& Path) const;

    private:
        UPROPERTY(Edit, Serialize, Name = "Material", Getter = GetMaterialPath, Setter = SetMaterialPath)
        Core::FString mMaterialPath{};

        Asset::UMaterial* mMaterial{};

        UPROPERTY(Serialize, SerializeLate, Name = "SlotMaterials", Getter = GetSlotMaterialPaths, Setter = SetSlotMaterialPaths)
        Core::TArray<Core::FString> mMaterialOverrides{};
        // 메시 setter가 오버라이드를 비우므로 메시 경로를 복원한 뒤에 로드합니다.
    };

}
