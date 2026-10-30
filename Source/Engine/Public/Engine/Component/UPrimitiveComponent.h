/**
 * @file UPrimitiveComponent.h
 * @brief 장면 프록시 생성과 지연된 렌더 상태 및 변환 갱신을 제공하는 기반 클래스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Component/USceneComponent.h"
#include "Core/Math/FVector2.h"

enum class EPrimitiveType : Uint8;
struct FPrimitiveSceneProxy;
struct FBox;
struct FMeshData;
class UStaticMesh;

/**
 * @brief 장면 프록시 생성과 지연된 렌더 상태 및 변환 갱신을 제공하는 기반 클래스입니다.
 * @ingroup Engine
 */
class UPrimitiveComponent : public USceneComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UPrimitiveComponent, USceneComponent);

public:
    /**
     * @brief UPrimitiveComponent의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    UPrimitiveComponent();

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~UPrimitiveComponent() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UPrimitiveComponent, EClassFlags::Abstract);

    /**
     * @fn UClass* UPrimitiveComponent::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 컴포넌트를 월드에 연결하고 필요한 렌더 상태를 등록합니다.
     * @param[in,out] World 연결할 월드 또는 적용할 월드 행렬입니다.
     */
    void RegisterComponent(UWorld* World) override;

    /**
     * @brief 등록된 렌더 상태와 월드 연결을 해제합니다.
     */
    void UnregisterComponent() override;

    /**
     * @brief 유한하고 유효한 변환을 검증하여 공간 컴포넌트에 적용합니다.
     * @param[in] Transform 적용할 위치, 회전과 스케일입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool SetTransform(const FTransform& Transform) override;

    /**
     * @brief 프리미티브 프록시 재생성이 필요함을 표시하고 갱신을 예약합니다.
     */
    void MarkRenderStateDirty();

    /**
     * @brief 변환 업로드가 필요함을 표시하고 프레임 말 갱신을 예약합니다.
     */
    void MarkRenderTransformDirty();

    /**
     * @brief 누적된 렌더 상태 또는 변환 변경을 연결된 장면에 반영합니다.
     */
    void SendRenderUpdates();

    /**
     * @brief 표시할 프리미티브 메시 종류를 반환합니다.
     * @return 표시할 프리미티브 메시 종류입니다.
     */
    virtual EPrimitiveType GetPrimitiveType() const = 0;

    /**
     * @brief 프리미티브가 사용하는 정적 메시의 비소유 주소를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    virtual UStaticMesh* GetStaticMesh() const;

    /**
     * @brief 메시의 CPU 정점 속성·인덱스 데이터 참조를 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    virtual const FMeshData& GetMeshData() const;

    /**
     * @brief 컴포넌트의 현재 상태를 복사한 프리미티브 프록시를 만듭니다.
     * @return 연산을 적용한 새 값입니다.
     */
    virtual FPrimitiveSceneProxy CreateSceneProxy() const;

    /**
     * @brief 월드 공간의 축 정렬 경계 상자를 반환합니다.
     * @return 월드 공간의 축 정렬 경계 상자입니다.
     */
    virtual FBox GetWorldBounds() const;

    /**
     * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
     * @return 조건을 만족하면 true입니다.
     */
    virtual bool ShouldCreateRenderState() const;

    /**
     * @brief 다른 살아 있는 제어자의 변환을 덮어쓰지 않고 UV 배율과 오프셋을 적용합니다.
     * @param[in] Scale UV 또는 크기에 적용할 배율입니다.
     * @param[in] Offset UV에 더할 셀 원점입니다.
     * @param[in] Source UV 제어자의 유효한 세대 핸들입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetUVTransform(const FVector2& Scale, const FVector2& Offset, FObjectHandle Source);

    /**
     * @brief 요청한 제어자가 설정한 UV만 기본 변환으로 복원합니다.
     * @param[in] Source UV 제어자의 유효한 세대 핸들입니다.
     */
    void ClearUVTransform(FObjectHandle Source);

    /**
     * @brief 기본 UV에 곱할 셀 크기를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector2& GetUVScale() const;

    /**
     * @brief 기본 UV에 더할 셀 원점을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector2& GetUVOffset() const;

    /**
     * @brief 프리미티브의 그림자 생성 여부를 반환합니다.
     * @return 프리미티브의 그림자 생성 여부입니다.
     */
    bool GetCastShadow() const;

    /**
     * @brief 프리미티브의 그림자 생성 여부를 검증하고 변경합니다.
     * @param[in] CastShadow 프리미티브가 그림자를 생성하도록 설정하려면 true입니다.
     * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
     */
    bool SetCastShadow(bool CastShadow);

    /**
     * @brief 프리미티브의 그림자 수신 여부를 반환합니다.
     * @return 프리미티브의 그림자 수신 여부입니다.
     */
    bool GetReceiveShadow() const;

    /**
     * @brief 프리미티브의 그림자 수신 여부를 검증하고 변경합니다.
     * @param[in] ReceiveShadow 프리미티브가 그림자를 받도록 설정하려면 true입니다.
     * @return 입력값을 수락하고 상태를 갱신하면 true이며 잘못된 값이면 false입니다.
     */
    bool SetReceiveShadow(bool ReceiveShadow);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 같은 컴포넌트를 중복 예약하지 않고 월드의 갱신 큐에 넣습니다.
     */
    void QueueUpdate();

private:
    /**
     * @brief 프록시 전체 재생성이 필요한지 여부입니다.
     */
    bool mRenderStateDirty{true};
    ReflectProperty(mRenderStateDirty, "RenderStateDirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프리미티브의 그림자 생성 여부입니다.
     */
    bool mCastShadow{true};
    ReflectAccessor(mCastShadow, GetCastShadow, SetCastShadow, "CastShadow", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 프리미티브의 그림자 수신 여부입니다.
     */
    bool mReceiveShadow{true};
    ReflectAccessor(mReceiveShadow, GetReceiveShadow, SetReceiveShadow, "ReceiveShadow", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 변환 업로드가 필요한지 여부입니다.
     */
    bool mRenderTransformDirty{};
    ReflectProperty(mRenderTransformDirty, "RenderTransformDirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 월드 갱신 큐에 이미 등록되었는지 여부입니다.
     */
    bool mQueued{};
    ReflectProperty(mQueued, "Queued", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 렌더 장면에 프록시가 생성되었는지 여부입니다.
     */
    bool mRenderStateCreated{};
    ReflectProperty(mRenderStateCreated, "RenderStateCreated", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 기본 UV에 곱할 셀 크기입니다.
     */
    FVector2 mUVScale{1.0f, 1.0f};
    ReflectProperty(mUVScale, "UVScale", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 기본 UV에 더할 셀 원점입니다.
     */
    FVector2 mUVOffset{};
    ReflectProperty(mUVOffset, "UVOffset", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 현재 UV 변환을 제어하는 컴포넌트 핸들입니다.
     */
    FObjectHandle mUVSource{};
    ReflectProperty(mUVSource, "UVSource", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
