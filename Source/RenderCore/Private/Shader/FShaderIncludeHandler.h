#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"
#include "RenderCore/Shader/FShaderDescription.h"

class FShaderIncludeHandler final : public ID3DInclude {
public:
    ReflectTypes(FShaderIncludeHandler);

private:
    struct FOpenFile {
    public:
        ReflectTypes(FOpenFile);

    public:
        ReflectStruct(FOpenFile);

        std::filesystem::path mPath{};
        ReflectProperty(mPath, "Path", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        TArray<Uint8> mData{};
        ReflectProperty(mData, "Data", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    private:
        ReflectMembers();
    };

public:
    FShaderIncludeHandler(const FShaderDescription& Description, const void* RootData, const FShaderDependency& RootDependency);

public:
    ReflectStruct(FShaderIncludeHandler);
    HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR FileName, LPCVOID ParentData, LPCVOID* Data, UINT* Bytes) override;
    HRESULT STDMETHODCALLTYPE Close(LPCVOID Data) override;
    const TArray<FShaderDependency>& GetDependencies() const;
    const FString& GetDiagnostics() const;

private:
    ReflectMembers();
    bool TrackDependency(const FShaderDependency& Dependency);

private:
    const FShaderDescription* mDescription{};
    ReflectProperty(mDescription, "Description", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    const void* mRootData{};
    ReflectProperty(mRootData, "RootData", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FShaderDependency> mDependencies{};
    ReflectProperty(mDependencies, "Dependencies", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TMap<const void*, FOpenFile> mOpenFiles{};
    ReflectProperty(mOpenFiles, "OpenFiles", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FString mDiagnostics{};
    ReflectProperty(mDiagnostics, "Diagnostics", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
