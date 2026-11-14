#include "Engine/Cascade/UParticleLODLevel.h"
#include "Engine/Cascade/UParticleEmitter.h"
#include "Engine/Cascade/UParticleModule.h"
#include "Engine/Cascade/UParticleModuleRequired.h"
#include "Core/Archive.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/NewObject.h"

#include <utility>

using namespace Engine;
using namespace Core;
using namespace CoreUObject;

namespace {

    constexpr Uint32 MaximumModules{256};

}

UParticleLODLevel::UParticleLODLevel()
	: mRequiredModule{NewObject<UParticleModuleRequired>()} {
    SetName("LODLevel");
    mRequiredModule->SetOuter(this);
}

UParticleLODLevel::~UParticleLODLevel() {
    ClearModules();
    mRequiredModule->SetOuter(nullptr);
}

bool UParticleLODLevel::GetEnabled() const {
    return mEnabled;
}

void UParticleLODLevel::SetEnabled(bool Enabled) {
    mEnabled = Enabled;
}

TObjectRef<UParticleModuleRequired> UParticleLODLevel::GetRequiredModule() {
    return mRequiredModule.get();
}

TObjectRef<const UParticleModuleRequired> UParticleLODLevel::GetRequiredModule() const {
    return mRequiredModule.get();
}

bool UParticleLODLevel::SetRequiredModule(std::unique_ptr<UParticleModuleRequired> Module) {
    if (Module == nullptr || (Module->GetOuter() != nullptr && Module->GetOuter() != this) || !Module->SetOuter(this)) {
        return false;
    }

    mRequiredModule->SetOuter(nullptr);
    mRequiredModule = std::move(Module);
    InvalidateEmitterBuildData();

    return true;
}

bool UParticleLODLevel::AddModule(std::unique_ptr<UParticleModule> Module) {
    if (Module == nullptr || Cast<UParticleModuleRequired>(Module.get()) != nullptr || mModules.size() >= MaximumModules) {
        return false;
    }

    if ((Module->GetOuter() != nullptr && Module->GetOuter() != this) || !Module->SetOuter(this)) {
        return false;
    }

    mModules.push_back(std::move(Module));
    UpdateModuleLists();

    return true;
}

bool UParticleLODLevel::RemoveModule(TObjectRef<const UParticleModule> Module) {
    for (auto Iterator{mModules.begin()}; Iterator != mModules.end(); ++Iterator) {
        if (Iterator->get() == Module) {
            (*Iterator)->SetOuter(nullptr);
            mModules.erase(Iterator);
            UpdateModuleLists();

            return true;
        }
    }

    return false;
}

void UParticleLODLevel::ClearModules() {
    for (const auto& Module : mModules) {
        Module->SetOuter(nullptr);
    }

    mModules.clear();
    UpdateModuleLists();
}

TObjectRef<UParticleModule> UParticleLODLevel::GetModule(Index_t Index) {
    return Index >= 0 && static_cast<std::size_t>(Index) < mModules.size() ? mModules[Index].get() : nullptr;
}

TObjectRef<const UParticleModule> UParticleLODLevel::GetModule(Index_t Index) const {
    return Index >= 0 && static_cast<std::size_t>(Index) < mModules.size() ? mModules[Index].get() : nullptr;
}

std::size_t UParticleLODLevel::GetModuleCount() const {
    return mModules.size();
}

TArray<TObjectRef<UParticleModule>> UParticleLODLevel::GetModules() const {
    TArray<TObjectRef<UParticleModule>> Modules{};

    Modules.reserve(mModules.size());

    for (const auto& Module : mModules) {
        Modules.push_back(Module.get());
    }

    return Modules;
}

void UParticleLODLevel::UpdateModuleLists() {
    InvalidateEmitterBuildData();
    mSpawnModules.clear();
    mUpdateModules.clear();
    mFinalUpdateModules.clear();

    for (const auto& Module : mModules) {
        if (!Module->mEnabled) {
            continue;
        }

        if (Module->mSpawnModule) {
            mSpawnModules.push_back(Module.get());
        }

        if (Module->mUpdateModule) {
            mUpdateModules.push_back(Module.get());
        }

        if (Module->mFinalUpdateModule) {
            mFinalUpdateModules.push_back(Module.get());
        }
    }
}

const TArray<TObjectRef<UParticleModule>>& UParticleLODLevel::GetSpawnModules() const {
    return mSpawnModules;
}

const TArray<TObjectRef<UParticleModule>>& UParticleLODLevel::GetUpdateModules() const {
    return mUpdateModules;
}

const TArray<TObjectRef<UParticleModule>>& UParticleLODLevel::GetFinalUpdateModules() const {
    return mFinalUpdateModules;
}

void UParticleLODLevel::Serialize(FArchive& Archive) {
    if (Archive.HasError()) {
        return;
    }

    bool Enabled{mEnabled};

    Archive.Serialize("Enabled", Enabled);

    if (!Archive.BeginObject("RequiredModule")) {
        return;
    }

    auto Required{Archive.IsLoading() ? CreateModuleFromArchive(Archive) : nullptr};
    UParticleModule* RequiredModule{Archive.IsLoading() ? Required.get() : mRequiredModule.get()};

    if (RequiredModule != nullptr) {
        RequiredModule->SerializeModule(Archive);
    }

    Archive.EndObject();

    if (Archive.HasError()) {
        return;
    }

    if (Archive.IsLoading() && Cast<UParticleModuleRequired>(Required.get()) == nullptr) {
        Archive.SetError("Invalid required particle module class");

        return;
    }

    TArray<std::unique_ptr<UParticleModule>> Modules{};

    SerializeModules(Archive, Archive.IsLoading() ? Modules : mModules);

    if (Archive.IsLoading() && !Archive.HasError()) {
        ClearModules();
        mRequiredModule->SetOuter(nullptr);
        mRequiredModule.reset(static_cast<UParticleModuleRequired*>(Required.release()));
        mModules = std::move(Modules);
        mEnabled = Enabled;
        AdoptModules();
    }
}

std::unique_ptr<UParticleModule> UParticleLODLevel::CreateModuleFromArchive(FArchive& Archive) {
    FString Type{};

    Archive.Serialize("Type", Type);

    if (Archive.HasError()) {
        return nullptr;
    }

    const UClass* Class{FReflectionRegistry::FindClass(Type)};

    if (Class == nullptr || !Class->IsChildOf(UParticleModule::StaticClass())) {
        Archive.SetError("Unknown particle module class: " + Type);

        return nullptr;
    }

    std::unique_ptr<UObject> Object{Class->CreateInstance()};

    if (Cast<UParticleModule>(Object.get()) == nullptr) {
        Archive.SetError("Cannot create particle module class: " + Type);

        return nullptr;
    }

    return std::unique_ptr<UParticleModule>{static_cast<UParticleModule*>(Object.release())};
}

void UParticleLODLevel::SerializeModules(FArchive& Archive, TArray<std::unique_ptr<UParticleModule>>& Modules) {
    Uint32 Count{static_cast<Uint32>(Modules.size())};

    if (!Archive.BeginArray("Modules", Count)) {
        return;
    }

    if (Count > MaximumModules) {
        Archive.SetError("Too many particle modules");
    }

    for (Index_t Index{}; Index < static_cast<Index_t>(Count) && !Archive.HasError(); ++Index) {
        if (!Archive.BeginObject(std::to_string(Index))) {
            break;
        }

        auto Loaded{Archive.IsLoading() ? CreateModuleFromArchive(Archive) : nullptr};
        UParticleModule* Module{Archive.IsLoading() ? Loaded.get() : Modules[Index].get()};

        if (Module != nullptr) {
            Module->SerializeModule(Archive);
        }

        Archive.EndObject();

        if (Archive.IsLoading() && !Archive.HasError()) {
            if (Cast<UParticleModuleRequired>(Loaded.get()) != nullptr) {
                Archive.SetError("Required module must use the RequiredModule slot");
            } else {
                Modules.push_back(std::move(Loaded));
            }
        }
    }

    Archive.EndArray();
}

void UParticleLODLevel::AdoptModules() {
    mRequiredModule->SetOuter(this);

    for (const auto& Module : mModules) {
        Module->SetOuter(this);
    }

    UpdateModuleLists();
}

void UParticleLODLevel::InvalidateEmitterBuildData() {
    if (auto* Emitter{Cast<UParticleEmitter>(GetOuter())}) {
        Emitter->InvalidateBuildData();
    }
}
