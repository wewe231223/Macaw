#pragma once

#include "PhysicsRuntime/FPhysicsScene.h"
#include <PxPhysicsAPI.h>
#include <memory>

namespace PhysicsRuntime {
    class FPhysicsRuntime {
    private:
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

    private:
        physx::PxDefaultAllocator mAllocator{};
        physx::PxDefaultErrorCallback mErrorCallback{};

        std::unique_ptr<physx::PxFoundation, FRelease> mFoundation{};
        std::unique_ptr<physx::PxPhysics, FRelease> mPhysics{};
        std::unique_ptr<physx::PxDefaultCpuDispatcher, FRelease> mDispatcher{};

        bool mExtensionsInitialized{};
    };

    template <typename ObjectType>
    void FPhysicsRuntime::FRelease::operator()(ObjectType* Object) const noexcept {
        Object->release();
    }
}
