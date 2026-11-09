/**
 * @file FShader.cpp
 * @brief 셰이더 설명과 공유 컴파일 결과 및 갱신 세대 관리에 대한 구현입니다.
 * @ingroup RenderCore
 */

#include "RenderCore/Shader/FShader.h"
#include "Core/FRevision.h"

#include <utility>

namespace RenderCore {

    /**
     * @brief 전달받은 설정과 의존성으로 FShader의 초기 상태를 구성합니다.
     */
    RenderCore::FShader::FShader(RenderCore::FShaderDescription Description, Core::FString CacheKey, RenderCore::FShaderCompilation Compilation, bool LoadedFromCache)
		: mDescription{std::move(Description)},
		  mCacheKey{std::move(CacheKey)},
		  mCompilation{std::make_shared<const RenderCore::FShaderCompilation>(std::move(Compilation))},
		  mRevision{Core::FRevision::Next().GetValue()},
		  mLoadedFromCache{LoadedFromCache} {
    }

    /**
     * @brief 셰이더의 정규화된 컴파일 설명을 반환합니다.
     */
    const RenderCore::FShaderDescription& RenderCore::FShader::GetDescription() const {
        return mDescription;
    }

    /**
     * @brief 정규화한 셰이더 설명에서 사용하는 캐시 키를 반환합니다.
     */
    const Core::FString& RenderCore::FShader::GetCacheKey() const {
        return mCacheKey;
    }

    /**
     * @brief 현재 컴파일 결과의 셰이더 바이트코드를 비소유 뷰로 반환합니다.
     */
    std::span<const Core::Uint8> RenderCore::FShader::GetByteCode() const {
        return mCompilation->mByteCode;
    }

    /**
     * @brief 호출 후에도 현재 컴파일 결과의 수명을 유지할 공유 참조를 반환합니다.
     */
    std::shared_ptr<const RenderCore::FShaderCompilation> RenderCore::FShader::GetCompilation() const {
        return mCompilation;
    }

    /**
     * @brief 컴파일 결과를 교체할 때 증가하는 셰이더 갱신 세대를 반환합니다.
     */
    Core::Uint64 RenderCore::FShader::GetRevision() const {
        return mRevision;
    }

    /**
     * @brief 현재 컴파일 결과를 디스크 캐시에서 읽었는지 반환합니다.
     */
    bool RenderCore::FShader::IsLoadedFromCache() const {
        return mLoadedFromCache;
    }

    /**
     * @brief 새 컴파일 결과를 공유 스냅샷으로 교체하고 갱신 세대를 증가시킵니다.
     */
    void RenderCore::FShader::Commit(RenderCore::FShaderCompilation Compilation, bool LoadedFromCache) {
        mCompilation = std::make_shared<const RenderCore::FShaderCompilation>(std::move(Compilation));
        mLoadedFromCache = LoadedFromCache;
        mRevision = Core::FRevision::Next().GetValue();
    }

}
