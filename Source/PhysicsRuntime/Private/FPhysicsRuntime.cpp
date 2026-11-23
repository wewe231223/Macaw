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
        mMaterials.clear();
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

        if (ResolveMaterial(PhysicsRuntime::UPhysicalMaterial::GetDefault()) == nullptr) {
            MACAW_LOG(LogPhysics, Error, "Cannot create the default PhysX material.");

            return {};
        }

        auto Scene{std::make_unique<PhysicsRuntime::FPhysicsScene>(*this, *mPhysics, Description)};

        if (!Scene->IsValid()) {
            MACAW_LOG(LogPhysics, Error, "Cannot create the PhysX scene.");

            return {};
        }

        return Scene;
    }

    physx::PxMaterial* FPhysicsRuntime::ResolveMaterial(const PhysicsRuntime::UPhysicalMaterial& Material) {
        if (mPhysics == nullptr) {
            return nullptr;
        }

        static_assert(static_cast<Uint8>(PhysicsRuntime::EPhysicalMaterialCombineMode::Average) == physx::PxCombineMode::eAVERAGE);
        static_assert(static_cast<Uint8>(PhysicsRuntime::EPhysicalMaterialCombineMode::Min) == physx::PxCombineMode::eMIN);
        static_assert(static_cast<Uint8>(PhysicsRuntime::EPhysicalMaterialCombineMode::Multiply) == physx::PxCombineMode::eMULTIPLY);
        static_assert(static_cast<Uint8>(PhysicsRuntime::EPhysicalMaterialCombineMode::Max) == physx::PxCombineMode::eMAX);

        const FMaterialKey Key{Material.GetStaticFriction(), Material.GetDynamicFriction(), Material.GetRestitution(), Material.GetFrictionCombineMode(), Material.GetRestitutionCombineMode()};

        if (const auto Existing{mMaterials.find(Key)}; Existing != mMaterials.end()) {
            return Existing->second.get();
        }

        std::unique_ptr<physx::PxMaterial, FRelease> Native{mPhysics->createMaterial(Material.GetStaticFriction(), Material.GetDynamicFriction(), Material.GetRestitution())};

        if (Native == nullptr) {
            return nullptr;
        }

        Native->setFrictionCombineMode(static_cast<physx::PxCombineMode::Enum>(Material.GetFrictionCombineMode()));
        Native->setRestitutionCombineMode(static_cast<physx::PxCombineMode::Enum>(Material.GetRestitutionCombineMode()));

        return mMaterials.emplace(Key, std::move(Native)).first->second.get();
    }
}
