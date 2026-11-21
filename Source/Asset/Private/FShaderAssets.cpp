#include "Asset/FAssetManager.h"

#include "Asset/UMaterial.h"
#include "RenderCore/Shader/UShader.h"
#include "Asset/UStaticMesh.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/USkeleton.h"
#include "Asset/UTexture2D.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/Animation/UBlendSpace.h"
#include "Asset/Animation/UBlendSpace1D.h"
#include "Asset/Animation/UAnimGraph.h"
#include "Asset/Sequence/ULevelSequence.h"
#include "Core/FilePath.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "FAssetImportScope.h"
#include "Serialization/MemoryArchive.h"

using namespace Asset;
using namespace RenderCore;

Core::FGuid FAssetManager::GetSaveIdentity(const std::filesystem::path& Path, const Core::FGuid& Guid) const {
    EAssetType Type{};
    Core::FGuid ExistingGuid{Core::FGuid::Null()};
    Core::FString Error{};
    FAssetFile::GetIdentity(Path, Type, ExistingGuid, Error);

    return ExistingGuid.IsValid() ? ExistingGuid : Guid;
}

bool FAssetManager::ValidateAssetIdentity(const std::filesystem::path& Path, const Core::FGuid& Guid, Core::FString& Error) {
    Error.clear();
    const auto Target{ResolvePath(Path, Error)};

    if (Target.empty() || !Guid.IsValid() || !RefreshAssetRegistry(Error)) {
        if (Error.empty()) {
            Error = "Invalid asset identity";
        }

        return false;
    }

    const auto Existing{mAssetPaths.find(GetSaveIdentity(Target, Guid))};

    if (Existing != mAssetPaths.end() && Core::FilePath::ToLower(ResolvePath(Existing->second, Error)) != Core::FilePath::ToLower(Target)) {
        Error = "Cannot save the same asset GUID to multiple files";

        return false;
    }

    return true;
}

bool FAssetManager::RefreshAssetRegistry(Core::FString& Error) {
    std::error_code Code{};

    if (!std::filesystem::exists(mContentRoot, Code) && !Code) {
        Error.clear();
        mAssetPaths.clear();

        return true;
    }

    const auto Paths{GetAssetPaths(Error)};

    if (!Error.empty()) {
        return false;
    }

    Core::TMap<Core::FGuid, std::filesystem::path, Core::FGuidHash> Candidates{};

    for (const auto& Path : Paths) {
        EAssetType Type{};
        Core::FGuid Guid{Core::FGuid::Null()};
        Core::FString HeaderError{};

        if (!FAssetFile::GetIdentity(ResolvePath(Path, HeaderError), Type, Guid, HeaderError) || !Guid.IsValid()) {
            continue;
        }

        if (UShader::FindBuiltin(Guid) != nullptr || !Candidates.emplace(Guid, Path).second) {
            Error = "Duplicate asset GUID: " + Guid.ToString();

            return false;
        }
    }

    mAssetPaths = std::move(Candidates);

    return true;
}

std::filesystem::path FAssetManager::FindAssetPath(const Core::FGuid& Guid, Core::FString& Error) {
    Error.clear();

    if (!Guid.IsValid()) {
        Error = "Invalid asset GUID";

        return {};
    }

    if (!RefreshAssetRegistry(Error)) {
        return {};
    }

    const auto Found{mAssetPaths.find(Guid)};

    if (Found == mAssetPaths.end()) {
        Error = "Cannot find asset GUID: " + Guid.ToString();

        return {};
    }

    return Found->second;
}

CoreUObject::UObject* FAssetManager::LoadAsset(const Core::FGuid& Guid, Core::FString& Error, const CoreUObject::UClass* ExpectedClass) {
    Error.clear();

    if (auto* Builtin{UShader::FindBuiltin(Guid)}) {
        if (ExpectedClass != nullptr && !Builtin->IsA(ExpectedClass)) {
            Error = "The asset type does not match the object reference";

            return nullptr;
        }

        return Builtin;
    }

    const auto Path{FindAssetPath(Guid, Error)};

    if (Path.empty()) {
        return nullptr;
    }

    EAssetType Type{};

    if (!FAssetFile::GetType(ResolvePath(Path, Error), Type, Error)) {
        return nullptr;
    }

    CoreUObject::UObject* Object{};
    const CoreUObject::UClass* ActualClass{Type == EAssetType::Shader ? UShader::StaticClass() : Type == EAssetType::Material ? UMaterial::StaticClass() : Type == EAssetType::PhysicalMaterial ? PhysicsRuntime::UPhysicalMaterial::StaticClass() : Type == EAssetType::Mesh ? UStaticMesh::StaticClass() : Type == EAssetType::SkeletalMesh ? USkeletalMesh::StaticClass() : Type == EAssetType::Skeleton ? USkeleton::StaticClass() : Type == EAssetType::Texture ? UTexture2D::StaticClass() : Type == EAssetType::AnimSequence ? UAnimSequence::StaticClass() : Type == EAssetType::Sequence ? ULevelSequence::StaticClass() : Type == EAssetType::BlendSpace ? UBlendSpace1D::StaticClass() : Type == EAssetType::AnimGraph ? UAnimGraph::StaticClass() : nullptr};

    if (ExpectedClass != nullptr && (ActualClass == nullptr || !ActualClass->IsChildOf(ExpectedClass))) {
        Error = "The asset type does not match the object reference";

        return nullptr;
    }

    switch (Type) {
    case EAssetType::Shader:
        Object = LoadShader(Path, Error);
        break;
    case EAssetType::Material:
        Object = LoadMaterial(Path, Error);
        break;
    case EAssetType::PhysicalMaterial:
        Object = LoadPhysicalMaterial(Path, Error);
        break;
    case EAssetType::Mesh:
        Object = LoadMesh(Path, Error);
        break;
    case EAssetType::SkeletalMesh:
        Object = LoadSkeletalMesh(Path, Error);
        break;
    case EAssetType::Skeleton:
        Object = LoadSkeleton(Path, Error);
        break;
    case EAssetType::Texture:
        Object = LoadTexture(Path, Error);
        break;
    case EAssetType::AnimSequence:
        Object = LoadAnimSequence(Path, Error);
        break;
    case EAssetType::Sequence:
        Object = LoadSequence(Path, Error);
        break;
    case EAssetType::BlendSpace:
        Object = LoadBlendSpace(Path, Error);
        break;
    case EAssetType::AnimGraph:
        Object = LoadAnimGraph(Path, Error);
        break;
    default:
        Error = "This asset type requires its owning module's loader";
        break;
    }

    if (Object != nullptr && Object->GetGuid() != Guid) {
        Error = "The loaded asset GUID does not match its file identity";

        return nullptr;
    }

    return Object;
}

UShader* FAssetManager::LoadShader(const std::filesystem::path& Path, Core::FString& Error) {
    Error.clear();
    const auto Target{ResolvePath(Path, Error)};
    const Core::FString Key{Core::FilePath::ToGenericUtf8(Core::FilePath::ToLower(Core::FilePath::FromUtf8(GetRelativePath(Path, Error))))};

    if (Target.empty() || Key.empty()) {
        return nullptr;
    }

    EAssetType Type{};
    Core::FGuid Guid{Core::FGuid::Null()};

    if (!FAssetFile::GetIdentity(Target, Type, Guid, Error) || Type != EAssetType::Shader || !Guid.IsValid() || UShader::FindBuiltin(Guid) != nullptr) {
        if (Error.empty()) {
            Error = "Invalid shader asset identity or type";
        }

        return nullptr;
    }

    const auto Existing{mShaders.find(Key)};

    if (Existing != mShaders.end() && Existing->second->GetGuid() == Guid) {
        return Existing->second.get();
    }

    if (Existing != mShaders.end()) {
        mRetired.push_back(std::move(Existing->second));
        mShaders.erase(Existing);
    }

    for (auto Iterator{mShaders.begin()}; Iterator != mShaders.end(); ++Iterator) {
        if (Iterator->second->GetGuid() == Guid) {
            const auto OldPath{ResolvePath(Core::FilePath::FromUtf8(Iterator->first), Error)};
            std::error_code Code{};

            if (std::filesystem::exists(OldPath, Code) || Code) {
                Error = "Duplicate shader asset GUID: " + Guid.ToString();

                return nullptr;
            }

            auto Shader{std::move(Iterator->second)};
            mShaders.erase(Iterator);
            UShader* Result{Shader.get()};
            mShaders.emplace(Key, std::move(Shader));

            return Result;
        }
    }

    Core::TArray<Uint8> Payload{};

    if (!FAssetFile::Load(Target, EAssetType::Shader, Payload, Error)) {
        return nullptr;
    }

    auto Shader{CoreUObject::NewObject<UShader>()};
    Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Uint8>&>(Payload)};
    Uint32 Version{};
    Archive.Serialize("Version", Version);

    if (Version != 1) {
        Error = "Unsupported shader asset version";

        return nullptr;
    }

    Shader->Serialize(Archive);

    if (Archive.HasError() || Archive.GetRemainingBytes() != 0 || Shader->GetGuid() != Guid || ResolvePath(Core::FilePath::FromUtf8(Shader->GetSourcePath()), Error).empty()) {
        if (Error.empty()) {
            Error = Archive.HasError() ? Archive.GetError() : "Invalid shader asset data or identity";
        }

        return nullptr;
    }

    UShader* Result{Shader.get()};

    mShaders.emplace(Key, std::move(Shader));

    return Result;
}

bool FAssetManager::SaveShader(const std::filesystem::path& Path, UShader& Shader, Core::FString& Error) {
    Error.clear();
    const auto Target{ResolvePath(Path, Error)};
    const auto Source{Core::FilePath::FromUtf8(Shader.GetSourcePath())};

    if (Source.has_root_path()) {
        const Core::FString Relative{GetRelativePath(Source, Error)};

        if (Relative.empty() || !Shader.SetSourcePath(Relative)) {
            return false;
        }
    }

    if (Target.empty() || Shader.IsBuiltin() || ResolvePath(Core::FilePath::FromUtf8(Shader.GetSourcePath()), Error).empty() || !ValidateAssetIdentity(Target, Shader.GetGuid(), Error)) {
        if (Error.empty()) {
            Error = "Cannot save a builtin shader as a project asset";
        }

        return false;
    }

    if (GetSaveIdentity(Target, Shader.GetGuid()) != Shader.GetGuid()) {
        Error = "Cannot replace a shader asset with a different GUID";

        return false;
    }

    Core::TArray<Uint8> Payload{};
    Serialization::FMemoryArchive Archive{Payload};
    Uint32 Version{1};
    Archive.Serialize("Version", Version);
    Shader.Serialize(Archive);

    if (Archive.HasError()) {
        Error = Archive.GetError();

        return false;
    }

    if (!FAssetFile::Save(Target, EAssetType::Shader, Payload, Error, Shader.GetGuid())) {
        return false;
    }

    const Core::FString Key{Core::FilePath::ToGenericUtf8(Core::FilePath::ToLower(Core::FilePath::FromUtf8(GetRelativePath(Path, Error))))};
    const auto Cached{mShaders.find(Key)};

    if (Cached != mShaders.end() && Cached->second.get() != &Shader) {
        if (Cached->second->GetGuid() == Shader.GetGuid()) {
            Cached->second->SetName(Shader.GetName());
            Cached->second->SetSourcePath(Shader.GetSourcePath());
            Cached->second->SetEntryPoint(Shader.GetEntryPoint());
            Cached->second->SetUnlit(Shader.IsUnlit());
            Cached->second->SetWritesGBuffer(Shader.WritesGBuffer());
        } else {
            mRetired.push_back(std::move(Cached->second));
            mShaders.erase(Cached);
        }
    }

    mAssetPaths[Shader.GetGuid()] = Core::FilePath::FromUtf8(GetRelativePath(Path, Error));

    return true;
}

Core::FString FAssetManager::GetShaderPath(const UShader* Shader) const {
    for (const auto& Entry : mShaders) {
        if (Entry.second.get() == Shader) {
            return Entry.first;
        }
    }

    return {};
}

UShader* FAssetManager::LoadShaderSource(const Core::FString& SourcePath, Core::FString& Error) {
    Error.clear();

    if (auto* Builtin{UShader::FindBuiltin(SourcePath)}) {
        return Builtin;
    }

    const auto Source{ResolvePath(Core::FilePath::FromUtf8(SourcePath), Error)};
    std::error_code Code{};

    if (Source.empty() || !std::filesystem::is_regular_file(Source, Code) || Code) {
        if (Error.empty()) {
            Error = "Cannot read the shader source";
        }

        return nullptr;
    }

    const auto Paths{GetAssetPaths(Error)};

    if (!Error.empty()) {
        return nullptr;
    }

    for (const auto& Path : Paths) {
        EAssetType Type{};
        Core::FString HeaderError{};

        if (FAssetFile::GetType(ResolvePath(Path, HeaderError), Type, HeaderError) && Type == EAssetType::Shader) {
            UShader* Shader{LoadShader(Path, Error)};

            if (Shader == nullptr) {
                return nullptr;
            }

            if (Core::FilePath::ToLower(Core::FilePath::FromUtf8(Shader->GetSourcePath())) == Core::FilePath::ToLower(Core::FilePath::FromUtf8(SourcePath).lexically_normal())) {
                return Shader;
            }
        }
    }

    auto Shader{CoreUObject::NewObject<UShader>()};
    Shader->SetName(Core::FilePath::ToUtf8(Source.stem()));
    const auto Target{MakeUniqueAssetPath(Source.parent_path(), Shader->GetName() + "_Shader", Error)};

    if (Target.empty() || !Shader->SetSourcePath(SourcePath) || !SaveShader(Target, *Shader, Error)) {
        if (Error.empty()) {
            Error = "Invalid shader source path";
        }

        return nullptr;
    }

    return LoadShader(Target, Error);
}

bool FAssetManager::ImportShader(const std::filesystem::path& Source, const std::filesystem::path& Directory, std::filesystem::path& ImportedPath, Core::FString& Error) {
    Error.clear();
    ImportedPath.clear();
    const auto Parent{Directory.empty() ? mContentRoot : ResolvePath(Directory, Error)};
    const auto Target{Parent.empty() ? std::filesystem::path{} : MakeUniqueAssetPath(Parent, Core::FilePath::ToUtf8(Source.stem()) + "_Shader", Error)};
    std::error_code Code{};

    if (Target.empty() || Source.extension() != L".hlsl" || !std::filesystem::is_regular_file(Source, Code) || Code) {
        if (Error.empty()) {
            Error = "Cannot read the shader import source";
        }

        return false;
    }

    const auto SourceFolder{Target.parent_path() / Target.stem()};
    std::filesystem::create_directories(Parent, Code);

    if (Code || !std::filesystem::create_directory(SourceFolder, Code)) {
        Error = "Cannot create the shader source directory";

        return false;
    }

    Asset::FAssetImportScope Scope{SourceFolder};
    const auto SourceTarget{SourceFolder / Source.filename()};

    if (!Code) {
        std::filesystem::copy_file(Source, SourceTarget, std::filesystem::copy_options::none, Code);
    }

    if (Code) {
        Error = "Cannot copy the shader source: " + Code.message();

        return false;
    }

    auto Shader{CoreUObject::NewObject<UShader>()};
    Shader->SetName(Core::FilePath::ToUtf8(Source.stem()));

    if (!Shader->SetSourcePath(GetRelativePath(SourceTarget, Error)) || !SaveShader(Target, *Shader, Error)) {
        return false;
    }

    ImportedPath = Core::FilePath::FromUtf8(GetRelativePath(Target, Error));
    Scope.Commit();

    return true;
}
