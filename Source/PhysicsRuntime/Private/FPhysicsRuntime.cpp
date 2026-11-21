#include "PhysicsRuntime/FPhysicsRuntime.h"
#include "Core/Literals/UnitLiterals.h"
#include "Core/Log/Log.h"

namespace PhysicsRuntime {
    DEFINE_LOG_CATEGORY_STATIC(LogPhysics, Log, All);

    FPhysicsRuntime::FPhysicsRuntime() = default;

    FPhysicsRuntime::~FPhysicsRuntime() {
        Shutdown();
    }

    bool FPhysicsRuntime::Initialize() {
        if (mDispatcher != nullptr) {
            return true;
        }

        mFoundation.reset(PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback));

        if (mFoundation == nullptr) {
            MACAW_LOG(LogPhysics, Error, "Cannot create the PhysX foundation.");

            return false;
        }

        mPhysics.reset(PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, physx::PxTolerancesScale{}));

        if (mPhysics == nullptr) {
            MACAW_LOG(LogPhysics, Error, "Cannot create the PhysX SDK instance.");
            Shutdown();

            return false;
        }

        mExtensionsInitialized = PxInitExtensions(*mPhysics, nullptr);

        if (!mExtensionsInitialized) {
            MACAW_LOG(LogPhysics, Error, "Cannot initialize the PhysX extensions.");
            Shutdown();

            return false;
        }

        mDispatcher.reset(physx::PxDefaultCpuDispatcherCreate(1));

        if (mDispatcher == nullptr) {
            MACAW_LOG(LogPhysics, Error, "Cannot create the PhysX CPU dispatcher.");
            Shutdown();

            return false;
        }

        MACAW_LOG(LogPhysics, Log, "PhysX initialized.");

        return true;
    }

    void FPhysicsRuntime::Shutdown() {
        mDispatcher.reset();

        if (mExtensionsInitialized) {
            PxCloseExtensions();
            mExtensionsInitialized = false;
        }

        mPhysics.reset();
        mFoundation.reset();
    }

    std::unique_ptr<PhysicsRuntime::FPhysicsScene> FPhysicsRuntime::CreateScene() {
        using namespace Core::Literals;

        if (mPhysics == nullptr || mDispatcher == nullptr) {
            MACAW_LOG(LogPhysics, Error, "Cannot create a PhysX scene before initializing the runtime.");

            return {};
        }

        constexpr Core::FAcceleration StandardGravity{9.81_mps2};
        physx::PxSceneDesc Description{mPhysics->getTolerancesScale()};
        Description.gravity = physx::PxVec3{0.0f, 0.0f, -(StandardGravity / 1_mps2)};
        Description.cpuDispatcher = mDispatcher.get();
        Description.filterShader = physx::PxDefaultSimulationFilterShader;

        std::unique_ptr<PhysicsRuntime::FPhysicsScene> Scene{ std::make_unique<PhysicsRuntime::FPhysicsScene>(*mPhysics, Description) };

        if (!Scene->IsValid()) {
            MACAW_LOG(LogPhysics, Error, "Cannot create the PhysX scene.");

            return {};
        }

        return Scene;
    }
}
