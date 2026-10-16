#include "pch.h"
#include "Asset/UMaterialDefinition.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FJsonFile.h"
#include "FMaterialSerialization.h"

bool UMaterialDefinition::FData::IsValid() const {
    if (!IsValidMaterialData(mGPUData) || !FMaterialParameterValue{mPipeline}.IsValid() || (mBlendMode != EMaterialBlendMode::Opaque && mBlendMode != EMaterialBlendMode::Translucent)) {
        return false;
    }

    TSet<FGuid> Ids{};
    TSet<FName> Names{FName{"BaseColor"}, FName{"Opacity"}, FName{"Emissive"}, FName{"NormalScale"}, FName{"Metallic"}, FName{"Roughness"}, FName{"Specular"}, FName{"AmbientOcclusion"}, FName{"OpacityMaskClipValue"}};

    for (const FMaterialParameterDefinition& Parameter : mParameters) {
        if (!Parameter.IsValid() || !Ids.insert(Parameter.mId).second || !Names.insert(Parameter.mName).second) {
            return false;
        }
    }

    return true;
}

void UMaterialDefinition::FData::Serialize(FArchive& Archive) {
    if (!RequireMaterialFields(Archive, {"Core", "Parameters", "Pipeline", "BlendMode", "TwoSided"})) {
        return;
    }

    Archive.BeginObjectScope("Core");
    SerializeMaterialData(Archive, mGPUData);
    Archive.EndObjectScope();
    SerializeNativeValue(Archive, "Parameters", mParameters);
    Archive.SerializeStruct("Pipeline", mPipeline);

    Uint8 BlendMode{static_cast<Uint8>(mBlendMode)};

    Archive.Serialize("BlendMode", BlendMode);
    mBlendMode = static_cast<EMaterialBlendMode>(BlendMode);
    Archive.Serialize("TwoSided", mTwoSided);

    if (!IsValid()) {
        Archive.SetError();
    }
}

UMaterialDefinition::UMaterialDefinition() {
    InitializeClass(StaticClass());
}

UMaterialDefinition::~UMaterialDefinition() = default;

bool UMaterialDefinition::Initialize(const std::filesystem::path& Path) {
    rapidjson::Document Document{};

    if (!LoadMaterialDocument(Path, "MaterialDefinition", Document)) {
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

bool UMaterialDefinition::Save(const std::filesystem::path& Path) const {
    if (!mData.IsValid()) {
        return false;
    }

    rapidjson::Document Document{};

    InitializeMaterialDocument("MaterialDefinition", Document);

    FData Data{mData};
    FArchiveJson Archive{Document, Document.GetAllocator()};

    Data.Serialize(Archive);
    return !Archive.HasError() && FJsonFile::Save(Path, Document);
}

bool UMaterialDefinition::Configure(const FMaterialGPUData& GPUData, std::span<const FMaterialParameterDefinition> Parameters, const FMaterialAssetReference& Pipeline, EMaterialBlendMode BlendMode, bool TwoSided) {
    FData Candidate{};

    Candidate.mGPUData = GPUData;
    Candidate.mParameters.assign(Parameters.begin(), Parameters.end());
    Candidate.mPipeline = Pipeline;
    Candidate.mBlendMode = BlendMode;
    Candidate.mTwoSided = TwoSided;

    return SetData(Candidate);
}

bool UMaterialDefinition::SetGPUData(const FMaterialGPUData& Data) {
    if (!IsValidMaterialData(Data)) {
        return false;
    }

    mData.mGPUData = Data;
    MarkGPUDataDirty();
    return true;
}

bool UMaterialDefinition::SetParameterDefault(const FGuid& Id, const FMaterialParameterValue& Value) {
    const auto Position{std::ranges::find(mData.mParameters, Id, &FMaterialParameterDefinition::mId)};

    if (Position == mData.mParameters.end() || Position->mDefaultValue.GetType() != Value.GetType() || !Value.IsValid()) {
        return false;
    }

    Position->mDefaultValue = Value;
    MarkGPUDataDirty();
    return true;
}

const FMaterialGPUData& UMaterialDefinition::GetGPUData() const {
    return mData.mGPUData;
}

std::span<const FMaterialParameterDefinition> UMaterialDefinition::GetParameters() const {
    return mData.mParameters;
}

const FMaterialParameterDefinition* UMaterialDefinition::FindParameter(const FGuid& Id) const {
    const auto Position{std::ranges::find(mData.mParameters, Id, &FMaterialParameterDefinition::mId)};

    return Position != mData.mParameters.end() ? &*Position : nullptr;
}

bool UMaterialDefinition::BuildResolvedData(const IAssetRegistry& Registry, FResolvedMaterialData& OutData) const {
    OutData = {};

    OutData.mGPUData = mData.mGPUData;
    OutData.mPipeline = mData.mPipeline;
    OutData.mBlendMode = mData.mBlendMode;
    OutData.mTwoSided = mData.mTwoSided;

    for (const FMaterialParameterDefinition& Parameter : mData.mParameters) {
        OutData.mParameters.push_back(FMaterialParameterOverride{Parameter.mId, Parameter.mDefaultValue});
    }

    return true;
}

bool UMaterialDefinition::SetData(const FData& Data) {
    if (!Data.IsValid()) {
        return false;
    }

    mData = Data;
    MarkGPUDataDirty();
    return true;
}

void UMaterialDefinition::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    FData Candidate{mData};

    Candidate.Serialize(Archive);

    if (!Archive.HasError() && Archive.IsLoading()) {
        mData = std::move(Candidate);
        MarkGPUDataDirty();
    }
}
