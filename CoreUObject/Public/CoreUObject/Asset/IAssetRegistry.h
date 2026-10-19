/**
 * @file IAssetRegistry.h
 * @brief 에셋 식별자 해석, 타입별 핸들 조회와 기본 렌더링 에셋 확보의 인터페이스를 제공합니다.
 */
#pragma once
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Base/FAssetHandle.h"
#include "Core/Base/FGuid.h"
#include "CoreUObject/UObject.h"
#include "Core/Asset/FAssetPath.h"
#include <type_traits>
#include "Core/Asset/IAssetResolver.h"

/**
 * @brief IAssetResolver의 식별자 해석에 UObject 기반 에셋 조회를 추가하는 인터페이스입니다.
 */
class IAssetRegistry : public IAssetResolver {
public:
    /**
     * @brief 파생 레지스트리를 기반 포인터로 해제할 수 있게 합니다.
     */
    virtual ~IAssetRegistry() = default;

public:
    /**
     * @brief 지정한 클래스 타입에 해당하는 에셋 핸들을 조회합니다.
     * @param AssetType 조회할 에셋 클래스 메타데이터입니다.
     * @return 구현이 제공하는 타입별 에셋 핸들 배열입니다.
     */
    virtual TArray<FAssetHandle> GetAssetHandles(const UClass& AssetType) const = 0;

    /**
     * @brief 에셋 객체를 해석하고 T와의 클래스 상속 관계를 검사합니다.
     * @tparam T StaticClass를 제공하는 UObject 파생 에셋 타입입니다.
     * @param Handle 해석할 에셋 핸들입니다.
     * @return T 호환 에셋의 비소유 포인터입니다. 해석 또는 타입 검사 실패 시 nullptr입니다.
     */
    template <typename T>
        requires std::is_base_of_v<UObject, T>
    const T* ResolveAsset(FAssetHandle Handle) const;

    /**
     * @brief 기본 정적 메시 머티리얼을 확보합니다.
     * @return 구현이 제공하는 기본 머티리얼 핸들입니다.
     */
    virtual FAssetHandle EnsureDefaultStaticMeshMaterial() const = 0;

private:
    /**
     * @brief 타입 검사 이전의 에셋 객체 해석을 파생 구현에 위임합니다.
     * @param Handle 해석할 에셋 핸들입니다.
     * @return 해석한 객체의 비소유 포인터입니다. 해석할 수 없으면 nullptr입니다.
     */
    virtual const UObject* ResolveAssetObject(FAssetHandle Handle) const = 0;
};

template <typename T>
    requires std::is_base_of_v<UObject, T>
const T* IAssetRegistry::ResolveAsset(FAssetHandle Handle) const {
    const UObject* Asset{ResolveAssetObject(Handle)};

    if (Asset == nullptr || !Asset->GetClass()->IsChildOf(T::StaticClass())) {
        return nullptr;
    }

    return static_cast<const T*>(Asset);
}
