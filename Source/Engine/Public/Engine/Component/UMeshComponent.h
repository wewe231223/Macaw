#pragma once

#include "Engine/Component/UPrimitiveComponent.h"

namespace Asset {
    class UMaterial;
}

namespace Engine {

    class UMeshComponent : public Engine::UPrimitiveComponent {
    public:
        ReflectTypes(UMeshComponent, Engine::UPrimitiveComponent);

    public:
        UMeshComponent() = default;
        ~UMeshComponent() override = default;

    public:
        ReflectClass(UMeshComponent, CoreUObject::EClassFlags::Abstract);

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
        ReflectMembers();

        void OnRegister() override;
        Asset::UMaterial* LoadMaterial(const Core::FString& Path) const;

    private:
        Core::FString mMaterialPath{};
        ReflectAccessor(mMaterialPath, GetMaterialPath, SetMaterialPath, "Material", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        Asset::UMaterial* mMaterial{};

        Core::TArray<Core::FString> mMaterialOverrides{};
        // 메시 setter가 오버라이드를 비우므로 메시 경로를 복원한 뒤에 로드합니다.
        ReflectArrayAccessor(mMaterialOverrides, GetSlotMaterialPaths, SetSlotMaterialPaths, "SlotMaterials", Core::EPropertyFlags::Serialize | Core::EPropertyFlags::SerializeLate);
    };

}
