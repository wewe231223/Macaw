#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <d3dcompiler.h>
#include <filesystem>

struct FShaderMacro {
public:
    ReflectTypes(FShaderMacro);

public:
    ReflectStruct(FShaderMacro);

    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FString mValue{"1"};
    ReflectProperty(mValue, "Value", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

private:
    ReflectMembers();
};

struct FShaderDescription {
public:
    ReflectTypes(FShaderDescription);

public:
    ReflectStruct(FShaderDescription);
    bool Normalize(FString& Diagnostics);
    FString GetIdentity() const;
    FString GetCacheKey() const;

    std::filesystem::path mSourcePath{};
    ReflectProperty(mSourcePath, "SourcePath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FString mEntryPoint{};
    ReflectProperty(mEntryPoint, "EntryPoint", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FString mTargetProfile{};
    ReflectProperty(mTargetProfile, "TargetProfile", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FShaderMacro> mMacros{};
    ReflectProperty(mMacros, "Macros", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<std::filesystem::path> mIncludeDirectories{};
    ReflectProperty(mIncludeDirectories, "IncludeDirectories", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mCompileFlags{D3DCOMPILE_ENABLE_STRICTNESS};
    ReflectProperty(mCompileFlags, "CompileFlags", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

private:
    ReflectMembers();
};
