#include "pch.h"
#include "Asset/FMaterialParameter.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "FMaterialSerialization.h"
#include <type_traits>

namespace {
    bool IsMaterialReferenceValid(const FMaterialAssetReference& Reference) {
        return Reference.mPath.mPath.empty() || (Reference.mPath.mPath.starts_with("/Game/") && Reference.mPath.mPath.find("..") == FString::npos && Reference.mPath.mPath.find('\\') == FString::npos);
    }
}

bool FMaterialAssetReference::IsNull() const {
    return !mGuid.IsValid() && !mPath;
}

FAssetHandle FMaterialAssetReference::Resolve(const IAssetRegistry& Registry) const {
    const FAssetHandle Handle{mGuid.IsValid() ? Registry.FindAsset(mGuid) : FAssetHandle{}};

    return Handle ? Handle : Registry.FindAsset(mPath);
}

const FString& FMaterialAssetReference::GetPath() const {
    return mPath.mPath;
}

bool FMaterialAssetReference::SetPath(const FString& Path) {
    FMaterialAssetReference Candidate{*this};

    Candidate.mPath = FAssetPath{Path};

    if (!IsMaterialReferenceValid(Candidate)) {
        return false;
    }

    if (mPath.mPath != Path) {
        Candidate.mGuid = {};
    }

    *this = std::move(Candidate);
    return true;
}

void FMaterialAssetReference::Serialize(FArchive& Archive) {
    if (!RequireMaterialFields(Archive, {"Guid", "Path"})) {
        return;
    }

    Archive.Serialize("Guid", mGuid);
    Archive.Serialize("Path", mPath.mPath);

    if (!IsMaterialReferenceValid(*this)) {
        Archive.SetError();
    }
}

FMaterialParameterValue::FMaterialParameterValue(float Value)
	: mValue{Value} {
}

FMaterialParameterValue::FMaterialParameterValue(const FVector2& Value)
	: mValue{Value} {
}

FMaterialParameterValue::FMaterialParameterValue(const FVector3& Value)
	: mValue{Value} {
}

FMaterialParameterValue::FMaterialParameterValue(const FVector4& Value)
	: mValue{Value} {
}

FMaterialParameterValue::FMaterialParameterValue(Int32 Value)
	: mValue{Value} {
}

FMaterialParameterValue::FMaterialParameterValue(Uint32 Value)
	: mValue{Value} {
}

FMaterialParameterValue::FMaterialParameterValue(const FMaterialAssetReference& Value)
	: mValue{Value} {
}

EMaterialParameterType FMaterialParameterValue::GetType() const {
    return static_cast<EMaterialParameterType>(mValue.index());
}

const FMaterialParameterValue::FValue& FMaterialParameterValue::GetValue() const {
    return mValue;
}

bool FMaterialParameterValue::SetValue(const FValue& Value) {
    if (mValue.index() != Value.index()) {
        return false;
    }

    FMaterialParameterValue Candidate{*this};

    Candidate.mValue = Value;

    if (!Candidate.IsValid()) {
        return false;
    }

    mValue = std::move(Candidate.mValue);
    return true;
}

bool FMaterialParameterValue::IsValid() const {
    return std::visit([](const auto& Value) {
        using TValue = std::decay_t<decltype(Value)>;

        if constexpr (std::is_same_v<TValue, float>) {
            return std::isfinite(Value);
        } else if constexpr (std::is_same_v<TValue, FVector2>) {
            return std::isfinite(Value.mX) && std::isfinite(Value.mY);
        } else if constexpr (std::is_same_v<TValue, FVector3>) {
            return std::isfinite(Value.mX) && std::isfinite(Value.mY) && std::isfinite(Value.mZ);
        } else if constexpr (std::is_same_v<TValue, FVector4>) {
            return std::isfinite(Value.mX) && std::isfinite(Value.mY) && std::isfinite(Value.mZ) && std::isfinite(Value.mW);
        } else if constexpr (std::is_same_v<TValue, FMaterialAssetReference>) {
            return IsMaterialReferenceValid(Value);
        } else {
            return true;
        }
    }, mValue);
}

void FMaterialParameterValue::Serialize(FArchive& Archive) {
    if (!RequireMaterialFields(Archive, {"Type", "Value"})) {
        return;
    }

    Uint8 Type{static_cast<Uint8>(GetType())};

    Archive.Serialize("Type", Type);

    if (Archive.HasError() || Type > static_cast<Uint8>(EMaterialParameterType::Texture2D)) {
        Archive.SetError();
        return;
    }

    FMaterialParameterValue Candidate{*this};

    if (Archive.IsLoading()) {
        switch (static_cast<EMaterialParameterType>(Type)) {
            case EMaterialParameterType::Float:
                Candidate.mValue = float{};
                break;
            case EMaterialParameterType::Float2:
                Candidate.mValue = FVector2{};
                break;
            case EMaterialParameterType::Float3:
                Candidate.mValue = FVector3{};
                break;
            case EMaterialParameterType::Float4:
                Candidate.mValue = FVector4{};
                break;
            case EMaterialParameterType::Int:
                Candidate.mValue = Int32{};
                break;
            case EMaterialParameterType::Uint:
                Candidate.mValue = Uint32{};
                break;
            case EMaterialParameterType::Texture2D:
                Candidate.mValue = FMaterialAssetReference{};
                break;
        }
    }

    std::visit([&Archive](auto& Value) {
        using TValue = std::decay_t<decltype(Value)>;

        if constexpr (std::is_same_v<TValue, FMaterialAssetReference>) {
            Archive.SerializeStruct("Value", Value);
        } else {
            Archive.Serialize("Value", Value);
        }
    }, Candidate.mValue);

    if (!Candidate.IsValid()) {
        Archive.SetError();
    }

    if (!Archive.HasError() && Archive.IsLoading()) {
        mValue = std::move(Candidate.mValue);
    }
}

bool FMaterialParameterDefinition::IsValid() const {
    if (!mId.IsValid() || mName.IsNone() || !mDefaultValue.IsValid()) {
        return false;
    }

    const FString Name{mName.ToString()};

    for (std::size_t Index{}; Index < Name.size(); ++Index) {
        const char Character{Name[Index]};
        const bool Letter{(Character >= 'A' && Character <= 'Z') || (Character >= 'a' && Character <= 'z') || Character == '_'};
        const bool Digit{Character >= '0' && Character <= '9'};

        if (!Letter && (Index == 0 || !Digit)) {
            return false;
        }
    }

    return true;
}

void FMaterialParameterDefinition::Serialize(FArchive& Archive) {
    if (!RequireMaterialFields(Archive, {"Id", "Name", "Default", "Group"})) {
        return;
    }

    Archive.Serialize("Id", mId);
    Archive.Serialize("Name", mName);
    Archive.SerializeStruct("Default", mDefaultValue);
    Archive.Serialize("Group", mGroup);

    if (!IsValid()) {
        Archive.SetError();
    }
}

void FMaterialParameterOverride::Serialize(FArchive& Archive) {
    if (!RequireMaterialFields(Archive, {"Id", "Value"})) {
        return;
    }

    Archive.Serialize("Id", mId);
    Archive.SerializeStruct("Value", mValue);

    if (!mId.IsValid() || !mValue.IsValid()) {
        Archive.SetError();
    }
}

void FMaterialCoreOverride::Serialize(FArchive& Archive) {
    if (!RequireMaterialFields(Archive, {"Parameter", "Value"})) {
        return;
    }

    Uint8 Parameter{static_cast<Uint8>(mParameter)};

    Archive.Serialize("Parameter", Parameter);
    mParameter = static_cast<EMaterialCoreParameter>(Parameter);
    Archive.SerializeStruct("Value", mValue);

    FMaterialGPUData Data{};

    if (!ApplyMaterialCoreOverride(Data, *this)) {
        Archive.SetError();
    }
}

bool ApplyMaterialCoreOverride(FMaterialGPUData& Data, const FMaterialCoreOverride& Override) {
    const std::size_t Index{static_cast<std::size_t>(Override.mParameter)};

    if (Index >= static_cast<std::size_t>(EMaterialCoreParameter::Count) || !Override.mValue.IsValid()) {
        return false;
    }

    FMaterialGPUData Candidate{Data};

    if (Override.mParameter == EMaterialCoreParameter::BaseColor || Override.mParameter == EMaterialCoreParameter::Emissive) {
        const FVector3* Value{std::get_if<FVector3>(&Override.mValue.GetValue())};

        if (Value == nullptr) {
            return false;
        }

        if (Override.mParameter == EMaterialCoreParameter::BaseColor) {
            Candidate.mBaseColor = *Value;
        } else {
            Candidate.mEmissive = *Value;
        }
    } else {
        constexpr std::array<float FMaterialGPUData::*, static_cast<std::size_t>(EMaterialCoreParameter::Count)> Fields{nullptr, &FMaterialGPUData::mOpacity, nullptr, &FMaterialGPUData::mNormalScale, &FMaterialGPUData::mMetallic, &FMaterialGPUData::mRoughness, &FMaterialGPUData::mSpecular, &FMaterialGPUData::mAmbientOcclusion, &FMaterialGPUData::mOpacityMaskClipValue};
        const float* Value{std::get_if<float>(&Override.mValue.GetValue())};

        if (Value == nullptr) {
            return false;
        }

        Candidate.*Fields[Index] = *Value;
    }

    if (!IsValidMaterialData(Candidate)) {
        return false;
    }

    Data = Candidate;
    return true;
}

const FMaterialParameterValue* FResolvedMaterialData::FindParameter(const FGuid& Id) const {
    const auto Position{std::ranges::find(mParameters, Id, &FMaterialParameterOverride::mId)};

    return Position != mParameters.end() ? &Position->mValue : nullptr;
}
