/**
 * @file FShaderIncludeHandler.cpp
 * @brief 셰이더 include 탐색과 열린 파일 및 의존성 추적에 대한 구현입니다.
 * @ingroup RenderCore
 */

#include "FShaderIncludeHandler.h"
#include "Core/FilePath.h"
#include "FShaderFile.h"

#include <algorithm>
#include <limits>

namespace RenderCore {

    RenderCore::FShaderIncludeHandler::FShaderIncludeHandler(const RenderCore::FShaderDescription& Description, const void* RootData, const RenderCore::FShaderDependency& RootDependency)
		: mDescription{&Description},
		  mRootData{RootData},
		  mDependencies{RootDependency} {
    }

    HRESULT STDMETHODCALLTYPE RenderCore::FShaderIncludeHandler::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR FileName, LPCVOID ParentData, LPCVOID* Data, UINT* Bytes) {
        if (FileName == nullptr || Data == nullptr || Bytes == nullptr) {
            return E_INVALIDARG;
        }

        *Data = nullptr;
        *Bytes = 0;

        const std::filesystem::path Requested{Core::FilePath::FromUtf8(FileName)};
        Core::TArray<std::filesystem::path> Candidates{};

        if (Requested.is_absolute()) {
            Candidates.push_back(Requested);
        } else {
            if (IncludeType == D3D_INCLUDE_LOCAL) {
                const auto Parent{mOpenFiles.find(ParentData)};

                if (ParentData != nullptr && ParentData != mRootData && Parent != mOpenFiles.end()) {
                    Candidates.push_back(Parent->second.mPath.parent_path() / Requested);
                }

                Candidates.push_back(mDescription->mSourcePath.parent_path() / Requested);
            }

            for (const std::filesystem::path& Directory : mDescription->mIncludeDirectories) {
                Candidates.push_back(Directory / Requested);
            }

            if (IncludeType == D3D_INCLUDE_SYSTEM) {
                Candidates.push_back(mDescription->mSourcePath.parent_path() / Requested);
            }
        }

        for (const std::filesystem::path& Candidate : Candidates) {
            const std::filesystem::path Path{Candidate.lexically_normal()};
            RenderCore::FShaderDependency Dependency{};

            if (!RenderCore::ShaderFile::Inspect(Path, Dependency)) {
                mDiagnostics = "Cannot inspect shader include: " + Core::FilePath::ToGenericUtf8(Path);
                return E_FAIL;
            }

            if (!Dependency.mExists) {
                if (!TrackDependency(Dependency)) {
                    return E_FAIL;
                }

                continue;
            }

            FOpenFile File{};
            File.mPath = Path;

            if (!RenderCore::ShaderFile::Read(Path, File.mData, Dependency, mDiagnostics) || !TrackDependency(Dependency) || File.mData.size() > std::numeric_limits<UINT>::max()) {
                return E_FAIL;
            }

            *Bytes = static_cast<UINT>(File.mData.size());
            File.mData.push_back(0);
            *Data = File.mData.data();
            mOpenFiles.emplace(*Data, std::move(File));

            return S_OK;
        }

        mDiagnostics = "Shader include not found: " + Core::FString{FileName};

        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    HRESULT STDMETHODCALLTYPE RenderCore::FShaderIncludeHandler::Close(LPCVOID Data) {
        return mOpenFiles.erase(Data) == 1 ? S_OK : E_INVALIDARG;
    }

    const Core::TArray<RenderCore::FShaderDependency>& RenderCore::FShaderIncludeHandler::GetDependencies() const {
        return mDependencies;
    }

    const Core::FString& RenderCore::FShaderIncludeHandler::GetDiagnostics() const {
        return mDiagnostics;
    }

    bool RenderCore::FShaderIncludeHandler::TrackDependency(const RenderCore::FShaderDependency& Dependency) {
        for (const RenderCore::FShaderDependency& Existing : mDependencies) {
            if (Existing.mPath == Dependency.mPath) {
                if (Existing.mExists != Dependency.mExists || Existing.mWriteTime != Dependency.mWriteTime || Existing.mSize != Dependency.mSize) {
                    mDiagnostics = "Shader dependency changed during compilation: " + Core::FilePath::ToGenericUtf8(Dependency.mPath);
                    return false;
                }

                return true;
            }
        }

        mDependencies.push_back(Dependency);

        return true;
    }

}
