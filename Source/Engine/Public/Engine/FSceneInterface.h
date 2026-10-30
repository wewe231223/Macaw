/**
 * @file FSceneInterface.h
 * @brief 엔진이 렌더 장면의 프리미티브를 추가, 갱신, 제거하는 추상 인터페이스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"

struct FPrimitiveSceneProxy;
struct FLightSceneProxy;
struct FMatrix;
struct FBox;

/**
 * @brief 엔진이 렌더 장면의 프리미티브를 추가, 갱신, 제거하는 추상 인터페이스입니다.
 * @ingroup Engine
 */
class FSceneInterface {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FSceneInterface);

public:
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    virtual ~FSceneInterface();

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FSceneInterface);

    /**
     * @brief 광원 프록시의 GUID로 렌더 장면에 광원을 추가하거나 교체합니다.
     * @param[in] Proxy 장면에 복사할 광원 상태입니다.
     */
    virtual void AddLight(const FLightSceneProxy& Proxy);

    /**
     * @brief 지정한 GUID의 광원을 렌더 장면에서 제거합니다.
     * @param[in] Id 제거할 광원의 GUID입니다.
     */
    virtual void RemoveLight(const FGuid& Id);

    /**
     * @brief 프록시의 GUID로 렌더 장면에 프리미티브를 추가하거나 교체합니다.
     * @param[in] Proxy 장면에 복사할 렌더 프록시입니다.
     */
    virtual void AddPrimitive(const FPrimitiveSceneProxy& Proxy) = 0;

    /**
     * @brief 지정한 프리미티브의 월드 행렬과 경계 상자를 갱신합니다.
     * @param[in] Id 장면 프리미티브의 식별자입니다.
     * @param[in] World 연결할 월드 또는 적용할 월드 행렬입니다.
     * @param[in] Bounds 프리미티브의 월드 또는 로컬 경계 상자입니다.
     */
    virtual void UpdatePrimitiveTransform(const FGuid& Id, const FMatrix& World, const FBox& Bounds) = 0;

    /**
     * @brief 식별자에 해당하는 프리미티브와 관련 GPU 리소스를 제거합니다.
     * @param[in] Id 장면 프리미티브의 식별자입니다.
     */
    virtual void RemovePrimitive(const FGuid& Id) = 0;

    /**
     * @brief 현재 장면에 등록된 프리미티브 개수를 반환합니다.
     * @return 현재 장면에 등록된 프리미티브 개수입니다.
     */
    virtual std::size_t GetPrimitiveCount() const = 0;

    /**
     * @brief 프리미티브 추가, 제거 또는 변환 갱신 시 증가하는 장면 개정 번호를 반환합니다.
     * @return 프리미티브 추가, 제거 또는 변환 갱신 시 증가하는 장면 개정 번호입니다.
     */
    virtual Uint64 GetRevision() const = 0;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
};
