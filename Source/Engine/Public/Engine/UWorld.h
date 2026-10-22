/**
 * @file UWorld.h
 * @brief 영속 레벨, 렌더 장면과 프레임 말 컴포넌트 갱신을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"

class ULevel;
class FSceneInterface;
class AStaticMeshActor;
struct FWorldHit;
struct FRay;
enum class EPrimitiveType : Uint8;

/**
 * @brief 영속 레벨, 렌더 장면과 프레임 말 컴포넌트 갱신을 관리합니다.
 * @ingroup Engine
 */
class UWorld final : public UObject {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UWorld, UObject);

public:
    /**
     * @brief UWorld의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    UWorld();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~UWorld() override;

public:
    /**
     * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UWorld, EClassFlags::NoFactory);
    /**
     * @fn UClass* UWorld::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 아직 레벨이 없는 월드에 영속 레벨의 소유권을 연결합니다.
     * @param[in] Level 소유권을 넘겨받을 영속 레벨입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool SetPersistentLevel(std::unique_ptr<ULevel> Level);
    /**
     * @brief 월드가 소유한 영속 레벨을 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    ULevel* GetPersistentLevel() const;

    /**
     * @brief 월드에 렌더 장면을 연결하고 기존 프리미티브를 동기화합니다.
     * @param[in] Scene 연결하거나 렌더링할 장면입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool AttachScene(std::unique_ptr<FSceneInterface> Scene);
    /**
     * @brief 연결된 렌더 장면을 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    FSceneInterface* GetScene() const;

    /**
     * @brief 지정한 기본 메시 종류의 액터를 생성하여 영속 레벨에 추가합니다.
     * @param[in] Type 생성, 조회 또는 표시할 타입입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    AStaticMeshActor* SpawnPrimitive(EPrimitiveType Type);
    /**
     * @brief 핸들에 해당하는 액터의 컴포넌트를 등록 해제하고 액터를 제거합니다.
     * @param[in] Handle 조회 또는 제거할 객체의 슬롯 세대 핸들입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool DestroyActor(FObjectHandle Handle);

    /**
     * @brief 컴포넌트 핸들을 프레임 말 렌더 갱신 큐에 추가합니다.
     * @param[in] Handle 조회 또는 제거할 객체의 슬롯 세대 핸들입니다.
     */
    void EnqueueRenderUpdate(FObjectHandle Handle);
    /**
     * @brief 큐에 쌓인 컴포넌트 핸들을 해석하여 지연된 렌더 갱신을 반영합니다.
     */
    void SendAllEndOfFrameUpdates();

    /**
     * @brief 월드 프리미티브의 메시를 검사하여 광선에 가장 가까운 충돌을 찾습니다.
     * @param[in] Ray 교차를 검사하거나 역투영 결과를 기록할 광선입니다.
     * @param[in,out] Hit 교차 성공 시 충돌 정보 또는 교점을 기록합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     * @note 입력 방향을 내부에서 정규화합니다. 원점이 유한하지 않거나 방향을 정규화할 수 없으면 실패합니다.
     */
    bool Raycast(const FRay& Ray, FWorldHit& Hit) const;

    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Shutdown();

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 월드가 소유한 영속 레벨입니다.
     */
    std::unique_ptr<ULevel> mPersistentLevel{};
    ReflectProperty(mPersistentLevel, "PersistentLevel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 월드가 소유한 렌더 장면입니다.
     */
    std::unique_ptr<FSceneInterface> mScene{};
    ReflectProperty(mScene, "Scene", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 프레임 말 갱신을 기다리는 컴포넌트 핸들입니다.
     */
    TArray<FObjectHandle> mDirtyComponents{};
    ReflectProperty(mDirtyComponents, "DirtyComponents", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 기본 액터 이름 생성용 일련번호입니다.
     */
    Uint32 mSpawnSerial{};
    ReflectProperty(mSpawnSerial, "SpawnSerial", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
