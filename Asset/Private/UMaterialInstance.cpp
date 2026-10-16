#include "pch.h"
#include "Asset/UMaterialInstance.h"
#include "Asset/UMaterialDefinition.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FJsonFile.h"
#include "FMaterialSerialization.h"

bool UMaterialInstance::FData::IsValid() const {
    if (mParent.IsNull() || !FMaterialParameterValue{mParent}.IsValid()) {
        return false;
    }

    TSet<Uint8> CoreFields{};
    TSet<FGuid> Parameters{};

    for (const FMaterialCoreOverride& Override : mCoreOverrides) {
        FMaterialGPUData Data{};

        if (!CoreFields.insert(static_cast<Uint8>(Override.mParameter)).second || !ApplyMaterialCoreOverride(Data, Override)) {
            return false;
        }
    }

    for (const FMaterialParameterOverride& Override : mParameterOverrides) {
        if (!Override.mId.IsValid() || !Override.mValue.IsValid() || !Parameters.insert(Override.mId).second) {
            return false;
        }
    }

    return true;
}

void UMaterialInstance::FData::Serialize(FArchive& Archive) {
    if (!RequireMaterialFields(Archive, {"Parent", "CoreOverrides", "ParameterOverrides"})) {
        return;
    }

    Archive.SerializeStruct("Parent", mParent);
    SerializeNativeValue(Archive, "CoreOverrides", mCoreOverrides);
    SerializeNativeValue(Archive, "ParameterOverrides", mParameterOverrides);

    if (!IsValid()) {
        Archive.SetError();
    }
}

UMaterialInstance::UMaterialInstance() {
    InitializeClass(StaticClass());
}

UMaterialInstance::~UMaterialInstance() = default;

bool UMaterialInstance::Initialize(const std::filesystem::path& Path) {
    rapidjson::Document Document{};

    if (!LoadMaterialDocument(Path, "MaterialInstance", Document)) {
        return false;
    }

    FData Candidate{};
    FArchiveJson Archive{Document};

    Candidate.Serialize(Archive);

    if (Archive.HasError() || !UAsset::Initialize(Path)) {
        return false;
    }

    mData = std::move(Candidate);
    MarkGPUDataDirty();
    return true;
}

bool UMaterialInstance::Save(const std::filesystem::path& Path) const {
    if (!mData.IsValid()) {
        return false;
    }

    rapidjson::Document Document{};

    InitializeMaterialDocument("MaterialInstance", Document);

    FData Data{mData};
    FArchiveJson Archive{Document, Document.GetAllocator()};

    Data.Serialize(Archive);
    return !Archive.HasError() && FJsonFile::Save(Path, Document);
}

bool UMaterialInstance::SetParent(const FMaterialAssetReference& Parent) {
    if (Parent.IsNull() || !FMaterialParameterValue{Parent}.IsValid()) {
        return false;
    }

    mData.mParent = Parent;
    MarkGPUDataDirty();
    return true;
}

const FMaterialAssetReference& UMaterialInstance::GetParent() const {
    return mData.mParent;
}

bool UMaterialInstance::SetCoreOverride(EMaterialCoreParameter Parameter, const FMaterialParameterValue& Value) {
    const FMaterialCoreOverride Override{Parameter, Value};
    FMaterialGPUData Data{};

    if (!ApplyMaterialCoreOverride(Data, Override)) {
        return false;
    }

    const auto Position{std::ranges::find(mData.mCoreOverrides, Parameter, &FMaterialCoreOverride::mParameter)};

    if (Position == mData.mCoreOverrides.end()) {
        mData.mCoreOverrides.push_back(Override);
    } else {
        *Position = Override;
    }

    MarkGPUDataDirty();
    return true;
}

bool UMaterialInstance::SetParameterOverride(const IAssetRegistry& Registry, const FGuid& Id, const FMaterialParameterValue& Value) {
    const UMaterialDefinition* Parent{Registry.ResolveAsset<UMaterialDefinition>(mData.mParent.Resolve(Registry))};
    const FMaterialParameterDefinition* Parameter{Parent != nullptr ? Parent->FindParameter(Id) : nullptr};

    if (Parameter == nullptr || !Value.IsValid() || Parameter->mDefaultValue.GetType() != Value.GetType()) {
        return false;
    }

    const auto Position{std::ranges::find(mData.mParameterOverrides, Id, &FMaterialParameterOverride::mId)};

    if (Position == mData.mParameterOverrides.end()) {
        mData.mParameterOverrides.push_back(FMaterialParameterOverride{Id, Value});
    } else {
        Position->mValue = Value;
    }

    MarkGPUDataDirty();
    return true;
}

bool UMaterialInstance::ClearCoreOverride(EMaterialCoreParameter Parameter) {
    if (std::erase_if(mData.mCoreOverrides, [Parameter](const FMaterialCoreOverride& Override) {
        return Override.mParameter == Parameter;
    }) == 0) {
        return false;
    }

    MarkGPUDataDirty();
    return true;
}

bool UMaterialInstance::ClearParameterOverride(const FGuid& Id) {
    if (std::erase_if(mData.mParameterOverrides, [&Id](const FMaterialParameterOverride& Override) {
        return Override.mId == Id;
    }) == 0) {
        return false;
    }

    MarkGPUDataDirty();
    return true;
}

std::span<const FMaterialCoreOverride> UMaterialInstance::GetCoreOverrides() const {
    return mData.mCoreOverrides;
}

std::span<const FMaterialParameterOverride> UMaterialInstance::GetParameterOverrides() const {
    return mData.mParameterOverrides;
}

bool UMaterialInstance::BuildResolvedData(const IAssetRegistry& Registry, FResolvedMaterialData& OutData) const {
    OutData = {};

    const UMaterialDefinition* Parent{Registry.ResolveAsset<UMaterialDefinition>(mData.mParent.Resolve(Registry))};
    FResolvedMaterialData Candidate{};

    if (Parent == nullptr || !Parent->BuildResolvedData(Registry, Candidate)) {
        return false;
    }

    for (const FMaterialCoreOverride& Override : mData.mCoreOverrides) {
        if (!ApplyMaterialCoreOverride(Candidate.mGPUData, Override)) {
            return false;
        }
    }

    for (const FMaterialParameterOverride& Override : mData.mParameterOverrides) {
        const auto Position{std::ranges::find(Candidate.mParameters, Override.mId, &FMaterialParameterOverride::mId)};

        if (Position == Candidate.mParameters.end() || Position->mValue.GetType() != Override.mValue.GetType()) {
            return false;
        }

        Position->mValue = Override.mValue;
    }

    OutData = std::move(Candidate);
    return true;
}

bool UMaterialInstance::SetData(const FData& Data) {
    if (!Data.IsValid()) {
        return false;
    }

    for (const FMaterialParameterOverride& Override : Data.mParameterOverrides) {
        const auto Position{std::ranges::find(mData.mParameterOverrides, Override.mId, &FMaterialParameterOverride::mId)};

        if (Position == mData.mParameterOverrides.end() || Position->mValue.GetType() != Override.mValue.GetType()) {
            return false;
        }
    }

    mData = Data;
    MarkGPUDataDirty();
    return true;
}

void UMaterialInstance::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    FData Candidate{mData};

    Candidate.Serialize(Archive);

    if (!Archive.HasError() && Archive.IsLoading()) {
        mData = std::move(Candidate);
        MarkGPUDataDirty();
    }
}
