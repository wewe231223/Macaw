#pragma once

#include "PhysicsRuntime/FPhysicsScene.h"
#include "PhysicsRuntime/UPhysicalMaterial.h"
#include <PxPhysicsAPI.h>
#include <map>
#include <memory>
#include <tuple>

namespace PhysicsRuntime {
    class FPhysicsRuntime {
    private:
        using FMaterialKey = std::tuple<float, float, float, PhysicsRuntime::EPhysicalMaterialCombineMode, PhysicsRuntime::EPhysicalMaterialCombineMode>;

        struct FRelease {
            template <typename ObjectType>
            void operator()(ObjectType* Object) const noexcept;
        };

    public:
        FPhysicsRuntime();
        ~FPhysicsRuntime();

    public:
        bool Initialize();
        void Shutdown();

        std::unique_ptr<PhysicsRuntime::FPhysicsScene> CreateScene();
        physx::PxMaterial* ResolveMaterial(const PhysicsRuntime::UPhysicalMaterial& Material);

    private:
        physx::PxDefaultAllocator mAllocator{};
        physx::PxDefaultErrorCallback mErrorCallback{};

        std::unique_ptr<physx::PxFoundation, FRelease> mFoundation{};
        std::unique_ptr<physx::PxPhysics, FRelease> mPhysics{};
        std::unique_ptr<physx::PxDefaultCpuDispatcher, FRelease> mDispatcher{};
        std::map<FMaterialKey, std::unique_ptr<physx::PxMaterial, FRelease>> mMaterials{};

        bool mExtensionsInitialized{};
    };

    template <typename ObjectType>
    void FPhysicsRuntime::FRelease::operator()(ObjectType* Object) const noexcept {
        Object->release();
    }
}
