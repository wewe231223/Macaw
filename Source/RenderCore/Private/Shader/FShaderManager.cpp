/**
 * @file FShaderManager.cpp
 * @brief 셰이더 캐시 로드와 변경 감지 및 재컴파일에 대한 구현입니다.
 * @ingroup RenderCore
 */

#include "RenderCore/Shader/FShaderManager.h"
#include "Core/LoadingStatus.h"

#include <utility>

namespace RenderCore {

    /**
     * @brief 전달받은 설정과 의존성으로 FShaderManager의 초기 상태를 구성합니다.
     */
    RenderCore::FShaderManager::FShaderManager(std::filesystem::path CacheDirectory)
		: mCache{std::move(CacheDirectory)} {
    }

    /**
     * @brief 셰이더를 메모리 캐시에서 조회하거나 디스크 캐시·컴파일로 준비합니다.
     */
    std::shared_ptr<const RenderCore::FShader> RenderCore::FShaderManager::Load(const RenderCore::FShaderDescription& Description, Core::FString& Diagnostics) {
        RenderCore::FShaderDescription Normalized{Description};

        if (!Normalized.Normalize(Diagnostics)) {
            return nullptr;
        }

        const Core::FString Key{Normalized.GetCacheKey()};

        if (Key.empty()) {
            Diagnostics = "Cannot compute shader cache key";

            return nullptr;
        }

        const auto Existing{mShaders.find(Key)};

        if (Existing != mShaders.end()) {
            return Existing->second;
        }

        RenderCore::FShaderCompilation Compilation{};
        bool LoadedFromCache{};

        if (!LoadCompilation(Normalized, Compilation, LoadedFromCache, Diagnostics)) {
            return nullptr;
        }

        auto Shader{std::make_shared<RenderCore::FShader>(std::move(Normalized), Key, std::move(Compilation), LoadedFromCache)};

        mShaders.emplace(Key, Shader);

        return Shader;
    }

    /**
     * @brief 셰이더의 의존성 스냅샷과 파일 상태를 비교하여 변경을 감지합니다.
     */
    bool RenderCore::FShaderManager::HasChanges(const RenderCore::FShader& Shader) const {
        return !mCache.IsCurrent(*Shader.GetCompilation());
    }

    /**
     * @brief 변경된 셰이더를 재컴파일하고 갱신에 성공한 셰이더를 모읍니다.
     */
    bool RenderCore::FShaderManager::ReloadChanged(Core::TArray<std::shared_ptr<const RenderCore::FShader>>& ChangedShaders, Core::FString& Diagnostics) {
        ChangedShaders.clear();
        Diagnostics.clear();

        bool Success{true};

        for (const auto& Entry : mShaders) {
            const std::shared_ptr<RenderCore::FShader>& Shader{Entry.second};

            if (!HasChanges(*Shader)) {
                continue;
            }

            RenderCore::FShaderCompilation Compilation{};
            bool LoadedFromCache{};
            Core::FString Message{};

            if (!LoadCompilation(Shader->GetDescription(), Compilation, LoadedFromCache, Message)) {
                Success = false;
            } else {
                Shader->Commit(std::move(Compilation), LoadedFromCache);
                ChangedShaders.push_back(Shader);
            }

            if (!Message.empty()) {
                Diagnostics += Message + "\n";
            }
        }

        return Success;
    }

    /**
     * @brief 관리 중인 셰이더의 공유 참조를 모두 제거합니다.
     */
    void RenderCore::FShaderManager::Clear() {
        mShaders.clear();
    }

    /**
     * @brief 이후 셰이더 로드·저장에 사용할 캐시 폴더를 변경합니다.
     */
    void RenderCore::FShaderManager::SetCacheDirectory(std::filesystem::path Directory) {
        Clear();

        mCache = RenderCore::FShaderCache{std::move(Directory)};
    }

    /**
     * @brief 유효한 캐시를 먼저 조회하고 사용할 수 없으면 셰이더를 컴파일합니다.
     */
    bool RenderCore::FShaderManager::LoadCompilation(const RenderCore::FShaderDescription& Description, RenderCore::FShaderCompilation& Compilation, bool& LoadedFromCache, Core::FString& Diagnostics) const {
        LoadedFromCache = mCache.Load(Description, Compilation, Diagnostics);

        if (LoadedFromCache) {
            return true;
        }

        Core::LoadingStatus::ReportFile("Compiling", Description.mSourcePath);

        if (!mCompiler.Compile(Description, Compilation, Diagnostics)) {
            return false;
        }

        Core::FString CacheDiagnostics{};

        if (!mCache.Save(Description, Compilation, CacheDiagnostics)) {
            Diagnostics += "\n" + CacheDiagnostics;
        }

        return true;
    }

}
