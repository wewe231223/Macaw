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

        Engine::FPrimitiveSceneProxy CreateSceneProxy() const override;
        void UnregisterComponent() override;
        void Serialize(Core::FArchive& Archive) override;

    private:
        ReflectMembers();

        void OnRegister() override;
        Asset::UMaterial* LoadMaterial(const Core::FString& Path) const;

    private:
        Core::FString mMaterialPath{};
        ReflectAccessor(mMaterialPath, GetMaterialPath, SetMaterialPath, "Material", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        Asset::UMaterial* mMaterial{};
        ReflectProperty(mMaterial, "ResolvedMaterial", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        Core::TArray<Core::FString> mMaterialOverrides{};
        ReflectProperty(mMaterialOverrides, "MaterialOverrides", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
