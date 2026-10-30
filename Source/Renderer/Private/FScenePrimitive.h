/**
 * @file FScenePrimitive.h
 * @brief 렌더 장면의 프리미티브 프록시와 GPU 객체 상수 버퍼입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "FObjectConstants.h"

#include <d3d11.h>
#include <wrl/client.h>

/**
 * @brief 렌더 장면의 프리미티브 프록시와 GPU 객체 상수 버퍼입니다.
 * @ingroup Renderer
 */
struct FScenePrimitive {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FScenePrimitive);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FScenePrimitive);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 엔진에서 복사한 프리미티브 프록시입니다.
     */
    FPrimitiveSceneProxy mProxy{};
    ReflectProperty(mProxy, "Proxy", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 객체별 GPU 상수 버퍼입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11Buffer> mObjectBuffer{};
    ReflectProperty(mObjectBuffer, "ObjectBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프리미티브 드로에 사용하는 월드·UV·그림자 상수의 CPU 사본입니다.
     */
    FObjectConstants mObjectConstants{};
    ReflectProperty(mObjectConstants, "ObjectConstants", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
