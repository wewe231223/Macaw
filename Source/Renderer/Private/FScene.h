/**
 * @file FScene.h
 * @brief 프리미티브 프록시와 객체 상수 버퍼를 소유하는 Direct3D 11 렌더 장면입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FSceneInterface.h"
#include "Core/GuidHash.h"
#include "FScenePrimitive.h"

#include <d3d11.h>
#include <wrl/client.h>

/**
 * @brief 프리미티브 프록시와 객체 상수 버퍼를 소유하는 Direct3D 11 렌더 장면입니다.
 * @ingroup Renderer
 */
class FScene final : public FSceneInterface {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FScene, FSceneInterface);

public:
    /**
     * @brief FScene의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in,out] Device 리소스 생성에 사용할 Direct3D 11 장치입니다.
     * @param[in,out] Context 월드 컨텍스트 또는 Direct3D 11 컨텍스트입니다.
     */
    FScene(ID3D11Device* Device, ID3D11DeviceContext* Context);
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FScene() override;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FScene);

    /**
     * @brief 프록시의 식별자로 렌더 장면에 프리미티브를 추가하거나 교체합니다.
     * @param[in] Proxy 장면에 복사할 프리미티브 상태입니다.
     */
    void AddPrimitive(const FPrimitiveSceneProxy& Proxy) override;
    /**
     * @brief 지정한 프리미티브의 월드 행렬과 경계 상자를 갱신합니다.
     * @param[in] Id 장면 프리미티브의 식별자입니다.
     * @param[in] World 연결할 월드 또는 적용할 월드 행렬입니다.
     * @param[in] Bounds 프리미티브의 월드 또는 로컬 경계 상자입니다.
     */
    void UpdatePrimitiveTransform(const FGuid& Id, const FMatrix& World, const FBox& Bounds) override;
    /**
     * @brief 식별자에 해당하는 프리미티브와 관련 GPU 리소스를 제거합니다.
     * @param[in] Id 장면 프리미티브의 식별자입니다.
     */
    void RemovePrimitive(const FGuid& Id) override;

    /**
     * @brief 현재 장면에 등록된 프리미티브 개수를 반환합니다.
     * @return 현재 장면에 등록된 프리미티브 개수입니다.
     */
    std::size_t GetPrimitiveCount() const override;
    /**
     * @brief 프리미티브 추가, 제거 또는 변환 갱신 시 증가하는 장면 개정 번호를 반환합니다.
     * @return 프리미티브 추가, 제거 또는 변환 갱신 시 증가하는 장면 개정 번호입니다.
     */
    Uint64 GetRevision() const override;

    /**
     * @brief 장면이 소유한 렌더 프리미티브를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const TArray<FScenePrimitive>& GetPrimitives() const;
    /**
     * @brief 누적 변환 업로드 횟수를 반환하고 카운터를 초기화합니다.
     * @return 연산을 적용한 새 값입니다.
     */
    Uint32 TakeTransformUpdates() const;

private:
    /**
     * @brief 프리미티브의 객체 상수 버퍼를 현재 프록시 값으로 갱신합니다.
     * @param[in,out] Primitive Primitive에 적용할 입력 값입니다.
     */
    void Upload(FScenePrimitive& Primitive);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief Direct3D 11 장치입니다.
     */
    ID3D11Device* mDevice{};
    ReflectProperty(mDevice, "Device", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief Direct3D 11 즉시 컨텍스트입니다.
     */
    ID3D11DeviceContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 장면이 소유한 렌더 프리미티브입니다.
     */
    TArray<FScenePrimitive> mPrimitives{};
    ReflectProperty(mPrimitives, "Primitives", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 프리미티브 GUID에서 밀집 배열 인덱스로의 조회 맵입니다.
     */
    TMap<FGuid, std::size_t, FGuidHash> mIndices{};
    ReflectProperty(mIndices, "Indices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 프리미티브 추가·제거 또는 변환 갱신 시 증가하는 장면 개정 번호입니다.
     */
    Uint64 mRevision{};
    ReflectProperty(mRevision, "Revision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 소비 전 누적된 객체 상수 버퍼 갱신 수입니다.
     */
    mutable Uint32 mTransformUpdates{};
    ReflectProperty(mTransformUpdates, "TransformUpdates", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
