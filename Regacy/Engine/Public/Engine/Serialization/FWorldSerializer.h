/**
 * @file FWorldSerializer.h
 * @brief 월드 액터와 컴포넌트의 저장, 단계별 로드 검증, GUID 재매핑과 액터 복제를 제공합니다.
 */
#pragma once

#include "Core/Common.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FName.h"
#include <memory>

class AActor;
class FArchive;
class UWorld;

/**
 * @brief 형식 버전 3의 액터 데이터와 객체 식별 테이블을 처리하는 월드 직렬화 도우미입니다.
 * @note 이름 기반 형식에서는 액터 데이터로 식별 테이블을 읽고, 순차 형식에서는 별도의 ObjectTable을 사용합니다.
 */
class FWorldSerializer final {
private:
    /**
     * @brief 객체 생성과 로드 결과 검증에 사용하는 GUID·클래스 이름 레코드입니다.
     */
    struct FObjectRecord {
        /**
         * @brief 저장된 객체의 GUID 또는 복제 시 재매핑된 GUID입니다.
         */
        FGuid mGuid{};
        /**
         * @brief 객체를 생성할 등록 리플렉션 클래스 이름입니다.
         */
        FName mTypeName{};
        FName mDefaultComponentName{};
    };

    struct FActorRecord {
        /**
         * @brief 액터의 GUID와 클래스 정보입니다.
         */
        FObjectRecord mObject{};
        /**
         * @brief 액터에 속한 컴포넌트의 GUID와 클래스 목록입니다.
         */
        TArray<FObjectRecord> mComponents{};
    };

    /**
     * @brief 월드 로드 동안 플레이를 종료하고 로딩 상태를 설정하는 범위 도우미입니다.
     */
    class FLoadScope final {
    public:
        /**
         * @brief 이전 플레이 상태를 저장하고 레벨 전환 사유로 플레이를 종료합니다.
         * @param World 로딩 상태를 설정할 월드입니다.
         */
        explicit FLoadScope(UWorld& World);
        /**
         * @brief 로딩 상태를 해제하고 액터 Tick을 갱신한 뒤 이전에 플레이 중이었으면 다시 시작합니다.
         */
        ~FLoadScope();
        /**
         * @brief 월드 로드 범위의 중복 복원을 방지하기 위해 복사 생성을 금지합니다.
         */
        FLoadScope(const FLoadScope&) = delete;
        /**
         * @brief 월드 로드 범위의 복사 대입을 금지합니다.
         */
        FLoadScope& operator=(const FLoadScope&) = delete;

    private:
        /**
         * @brief 로딩 상태와 플레이 상태를 관리할 월드 참조입니다.
         */
        UWorld& mWorld;
        /**
         * @brief 범위 시작 전에 월드가 플레이 중이었는지 기록합니다.
         */
        bool mWasPlaying{};
    };

    /**
     * @brief 아카이브의 GUID 재매핑 테이블을 범위 동안 교체하고 복원하는 도우미입니다.
     */
    class FObjectGuidScope final {
    public:
        /**
         * @brief 기존 GUID 재매핑 테이블을 저장하고 새 테이블을 적용합니다.
         * @param Archive 설정을 교체할 아카이브입니다.
         * @param Remapping 범위 동안 유효한 비소유 재매핑 테이블입니다. nullptr를 허용합니다.
         */
        FObjectGuidScope(FArchive& Archive, const TMap<FGuid, FGuid>* Remapping);
        /**
         * @brief 아카이브의 이전 GUID 재매핑 테이블을 복원합니다.
         */
        ~FObjectGuidScope();
        /**
         * @brief GUID 설정 범위의 중복 복원을 방지하기 위해 복사 생성을 금지합니다.
         */
        FObjectGuidScope(const FObjectGuidScope&) = delete;
        /**
         * @brief GUID 설정 범위의 복사 대입을 금지합니다.
         */
        FObjectGuidScope& operator=(const FObjectGuidScope&) = delete;

    private:
        /**
         * @brief GUID 재매핑 설정을 관리하는 아카이브 참조입니다.
         */
        FArchive& mArchive;
        /**
         * @brief 범위 종료 시 복원할 이전 비소유 재매핑 테이블입니다.
         */
        const TMap<FGuid, FGuid>* mPreviousRemapping{};
    };

public:
    /**
     * @brief 월드가 직렬화 진입을 허용하는 상태인지 검사합니다.
     * @param World 검사할 월드입니다.
     * @return 초기화되어 있고 IsBusy가 false이면 true입니다.
     */
    static bool CanSerialize(const UWorld& World);
    /**
     * @brief 생성이 완료되고 파괴 중이 아닌 액터를 아카이브에 저장합니다.
     * @param World 직렬화 가능한 월드입니다.
     * @param Archive 오류가 없는 저장 아카이브입니다.
     * @return 객체 테이블과 액터 데이터 저장에 성공하면 true입니다.
     * @note 지연 액터 삭제를 먼저 처리하고 아카이브의 에셋 해석기를 월드 레지스트리로 설정합니다.
     */
    static bool Save(UWorld& World, FArchive& Archive);
    /**
     * @brief 로드 범위에서 객체 정보를 검증하고 액터를 생성·로드·설치합니다.
     * @param World 직렬화 가능한 대상 월드입니다.
     * @param Archive 오류가 없는 로드 아카이브입니다.
     * @return 형식, 객체 정보, 데이터 검증과 설치가 모두 성공하면 true입니다.
     * @note 설치 전에는 임시 객체로 로드합니다. 설치 단계 실패 시 기존 액터를 복원하지 않으며 월드가 비워질 수 있습니다.
     */
    static bool Load(UWorld& World, FArchive& Archive);
    /**
     * @brief 선택 액터와 컴포넌트를 새 GUID로 복제해 같은 월드에 추가합니다.
     * @param World 직렬화 가능한 대상 월드입니다.
     * @param Actors 현재 월드에 속하고 생성이 완료되었으며 파괴 중이 아닌 액터 목록입니다.
     * @return 입력 순서의 복제 액터 비소유 포인터입니다. 빈 입력, 검증·직렬화·생성·설치 실패 시 빈 배열입니다.
     * @note 복제 집합 내부 참조는 복제 객체로 재매핑하고 기존 액터와 충돌하지 않는 이름을 생성합니다.
     */
    static TArray<AActor*> DuplicateActors(UWorld& World, const TArray<AActor*>& Actors);

private:
    /**
     * @brief 씬 파일 검증 후 내부 로드 범위와 로드 단계를 사용할 수 있게 합니다.
     */
    friend class FSceneSerializer;
    /**
     * @brief 원본 월드를 전달하는 내부 복제 로드 경로에 접근할 수 있게 합니다.
     */
    friend class FWorldCloner;

    /**
     * @brief 객체 테이블을 검증하고 선택적 GUID 재매핑 후 임시 액터를 로드·설치합니다.
     * @param World 로드 대상 월드입니다. 에셋 레지스트리가 있어야 합니다.
     * @param SourceWorld 복제 원본입니다. nullptr이면 원래 GUID를 사용하는 일반 로드입니다.
     * @return 전체 로드 단계와 설치가 성공하면 true입니다.
     * @pre 호출자가 월드의 로드 범위와 진입 상태를 관리해야 합니다.
     */
    static bool LoadInternal(UWorld& World, FArchive& Archive, const UWorld* SourceWorld = nullptr);
    /**
     * @brief 객체의 GUID와 타입 이름을 직렬화하고 식별 정보 형식을 검사합니다.
     * @param Archive 사용할 아카이브입니다.
     * @param[in,out] Record 저장할 레코드 또는 로드 결과입니다.
     * @return 아카이브 오류가 없고 GUID와 타입 이름이 유효하면 true입니다.
     */
    static bool SerializeObjectRecord(FArchive& Archive, FObjectRecord& Record, Uint32 FormatVersion);
    /**
     * @brief 액터와 각 컴포넌트의 식별 테이블을 저장하거나 읽습니다.
     * @param Archive 이름 기반이면 Actors, 순차 형식이면 ObjectTable 범위를 사용할 아카이브입니다.
     * @param[in,out] Records 저장할 식별 테이블 또는 로드 결과입니다.
     * @return 모든 객체 레코드와 배열 범위를 처리하고 오류가 없으면 true입니다.
     */
    static bool SerializeObjectTable(FArchive& Archive, TArray<FActorRecord>& Records, Uint32 FormatVersion);
    /**
     * @brief GUID 중복, 생성 가능한 클래스와 기반 타입 호환성을 검사합니다.
     * @param World GUID 재사용을 허용할 기존 월드입니다.
     * @param Records 검사할 액터·컴포넌트 식별 테이블입니다.
     * @param AllowExistingGuids 재매핑 전 기존 등록 GUID 충돌을 허용할지 여부입니다.
     * @return 모든 레코드가 유효하면 true입니다. 기본 정책에서는 대상 월드 밖의 등록 객체와 GUID가 충돌하면 false입니다.
     */
    static bool ValidateObjectTable(const UWorld& World, const TArray<FActorRecord>& Records, bool AllowExistingGuids = false);
    /**
     * @brief 원본 월드·레벨 참조를 대상에 연결하고 액터·컴포넌트에 새 GUID를 생성합니다.
     * @param SourceWorld 원본 월드입니다.
     * @param World 복제 대상 월드입니다.
     * @param[in,out] Records 새 GUID로 교체할 레코드입니다.
     * @param[in,out] Remapping 원본에서 대상 GUID로의 대응을 추가할 테이블입니다.
     * @return 중복 없는 GUID 생성과 테이블 추가가 모두 성공하면 true입니다.
     * @note 실패 시 이미 생성한 레코드와 매핑을 되돌리지 않습니다.
     */
    static bool RemapObjectTable(const UWorld& SourceWorld, const UWorld& World, TArray<FActorRecord>& Records, TMap<FGuid, FGuid>& Remapping);
    /**
     * @param Record 생성할 액터와 컴포넌트의 검증된 클래스·GUID 정보입니다.
     * @pre 각 클래스가 등록되어 있고 액터·컴포넌트 기반 타입과 호환되어야 합니다.
     * @return 생성 액터의 소유권입니다. 객체 생성이나 컴포넌트 GUID 복원이 실패하면 nullptr입니다.
     */
    static std::unique_ptr<AActor> CreateAndLoadActor(const FActorRecord& Record, FArchive& Archive);
    /**
     * @param Actors 로드한 유효한 액터 목록입니다.
     * @param Records 대응하는 식별 레코드 목록입니다.
     * @pre 두 목록의 개수와 순서가 대응해야 합니다.
     * @return 모든 액터와 컴포넌트 정보가 레코드에 일치하고 이름이 유효하면 true입니다.
     */
    static bool ValidateLoadedActors(const TArray<std::unique_ptr<AActor>>& Actors, const TArray<FActorRecord>& Records);
    /**
     * @brief 기존 액터를 교체하고 등록, 참조 해석, 초기화와 피킹 구조 갱신을 수행합니다.
     * @param World 액터를 설치할 월드입니다.
     * @param Actors 월드로 소유권을 전달할 검증된 액터 목록입니다.
     * @return 등록과 참조 해석이 성공하면 true입니다. 실패하면 설치 액터를 비웁니다.
     * @note 피킹 가속 구조 재구축 실패는 경고를 기록하며 함수 자체의 실패로 처리하지 않습니다.
     */
    static bool InstallActors(UWorld& World, TArray<std::unique_ptr<AActor>> Actors);
};
