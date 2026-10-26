/**
 * @file UStaticMeshComponent.h
 * @brief Plane, Cube 또는 Sphere 프리미티브를 표시하는 메시 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/EPrimitiveType.h"

class UMaterial;

/**
 * @brief Plane, Cube 또는 Sphere 프리미티브를 표시하는 메시 컴포넌트입니다.
 * @ingroup Engine
 */
class UStaticMeshComponent final : public UPrimitiveComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UStaticMeshComponent, UPrimitiveComponent);

public:
    /**
     * @brief UStaticMeshComponent의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    UStaticMeshComponent();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~UStaticMeshComponent() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UStaticMeshComponent);
    /**
     * @fn UClass* UStaticMeshComponent::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 표시할 프리미티브 메시 종류를 반환합니다.
     * @return 표시할 프리미티브 메시 종류입니다.
     */
    EPrimitiveType GetPrimitiveType() const override;
    /**
     * @brief 지원되는 프리미티브 종류를 검증하고 렌더 상태 갱신을 예약합니다.
     * @param[in] Type 생성, 조회 또는 표시할 타입입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool SetPrimitiveType(EPrimitiveType Type);
    /**
     * @brief 이 컴포넌트에 설정된 프로젝트 상대 메시 에셋 경로를 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const FString& GetMeshPath() const;
    /**
     * @brief 프로젝트의 메시 에셋 경로를 검증하고 컴포넌트의 렌더 상태를 갱신합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool SetMeshPath(const FString& Path);
    /**
     * @brief 프리미티브가 사용하는 정적 메시의 비소유 주소를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
     */
    UStaticMesh* GetStaticMesh() const override;
    /**
     * @brief 메시의 CPU 정점 속성·인덱스 데이터 참조를 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const FMeshData& GetMeshData() const override;
    void UnregisterComponent() override;
    bool ResolveMesh();
    const FString& GetMaterialPath() const;
    bool SetMaterialPath(const FString& Path);
    UMaterial* GetMaterial() const;
    bool ResolveMaterial();
    FPrimitiveSceneProxy CreateSceneProxy() const override;
    bool ShouldCreateRenderState() const override;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
    void OnRegister() override;
    UStaticMesh* LoadMesh(const FString& Path) const;
    UMaterial* LoadMaterial(const FString& Path) const;

private:
    /**
     * @brief 프리미티브 메시 종류입니다.
     */
    EPrimitiveType mPrimitive{EPrimitiveType::Cube};
    ReflectAccessor(mPrimitive, GetPrimitiveType, SetPrimitiveType, "Primitive", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 프로젝트 콘텐츠 폴더 기준 메시 에셋의 상대 경로입니다.
     */
    FString mMeshPath{};
    ReflectAccessor(mMeshPath, GetMeshPath, SetMeshPath, "Mesh", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    UStaticMesh* mStaticMesh{};
    ReflectProperty(mStaticMesh, "StaticMesh", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FString mMaterialPath{};
    ReflectAccessor(mMaterialPath, GetMaterialPath, SetMaterialPath, "Material", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    UMaterial* mMaterial{};
    ReflectProperty(mMaterial, "ResolvedMaterial", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
