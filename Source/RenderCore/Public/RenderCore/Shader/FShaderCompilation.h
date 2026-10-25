#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>

struct FShaderDependency {
public:
    ReflectTypes(FShaderDependency);

public:
    ReflectStruct(FShaderDependency);

    std::filesystem::path mPath{};
    ReflectProperty(mPath, "Path", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::filesystem::file_time_type mWriteTime{};
    ReflectProperty(mWriteTime, "WriteTime", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint64 mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mExists{};
    ReflectProperty(mExists, "Exists", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

private:
    ReflectMembers();
};

struct FShaderCompilation {
public:
    ReflectTypes(FShaderCompilation);

public:
    ReflectStruct(FShaderCompilation);

    TArray<Uint8> mByteCode{};
    ReflectProperty(mByteCode, "ByteCode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FShaderDependency> mDependencies{};
    ReflectProperty(mDependencies, "Dependencies", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::filesystem::file_time_type mBuildTime{};
    ReflectProperty(mBuildTime, "BuildTime", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

private:
    ReflectMembers();
};
