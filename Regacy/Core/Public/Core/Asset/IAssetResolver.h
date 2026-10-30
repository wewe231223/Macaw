/**
 * @file IAssetResolver.h
 * @brief 에셋 경로, 영속 GUID 및 런타임 핸들의 조회 인터페이스를 정의한다.
 */

#pragma once
#include "Core/Base/FAssetHandle.h"
#include "Core/Base/FGuid.h"
#include "Core/Asset/FAssetPath.h"

/**
 * @brief 에셋의 식별 정보와 런타임 핸들 사이의 조회 기능을 제공한다.
 * @details 파생 클래스는 경로 또는 영속 GUID로 핸들을 찾고, 핸들에 대응하는 경로 및 영속 GUID를 반환한다.
 * @details 반환하는 포인터의 대상은 리졸버 구현이 소유하며, 유효 기간은 구현의 데이터 관리 방식에 따라 달라진다.
 */
class IAssetResolver {
public:
    /** @brief 파생 리졸버를 인터페이스 포인터로 안전하게 소멸한다. */
    virtual ~IAssetResolver() = default;

public:
    /**
     * @brief 논리 경로에 대응하는 에셋 핸들을 조회한다.
     * @param AssetPath 조회할 에셋의 논리 경로.
     * @return 대응하는 에셋 핸들. 에셋을 찾을 수 없으면 유효하지 않은 핸들.
     */
    virtual FAssetHandle FindAsset(const FAssetPath& AssetPath) const = 0;

    /**
     * @brief 영속 GUID에 대응하는 에셋 핸들을 조회한다.
     * @param PersistentGuid 조회할 에셋의 영속 식별 GUID.
     * @return 대응하는 에셋 핸들. 에셋을 찾을 수 없으면 유효하지 않은 핸들.
     */
    virtual FAssetHandle FindAsset(const FGuid& PersistentGuid) const = 0;

    /**
     * @brief 에셋 핸들에 대응하는 논리 경로를 조회한다.
     * @param Handle 경로를 조회할 에셋 핸들.
     * @return 리졸버가 소유하는 경로의 읽기 전용 포인터. 핸들에 대응하는 유효한 경로가 없으면 nullptr.
     * @details 반환된 포인터의 소유권은 호출자에게 이전되지 않는다. 리졸버의 데이터 변경이나 소멸로 무효화될 수 있으므로 장기간 보관할 경로는 복사한다.
     */
    virtual const FAssetPath* GetAssetPath(FAssetHandle Handle) const = 0;

    /**
     * @brief 에셋 핸들에 대응하는 영속 GUID를 조회한다.
     * @param Handle GUID를 조회할 에셋 핸들.
     * @return 리졸버가 소유하는 GUID의 읽기 전용 포인터. 핸들에 대응하는 유효한 GUID가 없으면 nullptr.
     * @details 반환된 포인터의 소유권은 호출자에게 이전되지 않는다. 리졸버의 데이터 변경이나 소멸로 무효화될 수 있으므로 장기간 보관할 GUID는 복사한다.
     */
    virtual const FGuid* GetAssetGuid(FAssetHandle Handle) const = 0;
};
