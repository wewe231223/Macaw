/**
 * @file FObjectRegistry.h
 * @brief 객체의 GUID 검색과 슬롯 세대 검사를 통한 핸들 해석을 담당합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/GuidHash.h"
#include "CoreUObject/FObjectHandle.h"

class UObject;

/**
 * @brief 객체의 GUID 검색과 슬롯 세대 검사를 통한 핸들 해석을 담당합니다.
 * @ingroup CoreUObject
 */
class FObjectRegistry {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FObjectRegistry);

private:
    /**
     * @brief 객체 주소와 슬롯 재사용 세대를 보관하는 레지스트리 항목입니다.
     * @ingroup CoreUObject
     */
    struct FSlot {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FSlot);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSlot);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 슬롯에 등록된 객체의 비소유 주소입니다.
         */
        UObject* mObject{};
        ReflectProperty(mObject, "Object", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

        /**
         * @brief 슬롯 재사용을 구분하는 세대입니다.
         */
        Uint32 mGeneration{1};
        ReflectProperty(mGeneration, "Generation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    };

public:
    /**
     * @brief FObjectRegistry의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    FObjectRegistry();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FObjectRegistry();
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 복사 또는 이동 생성을 금지합니다.
     */
    FObjectRegistry(const FObjectRegistry&) = delete;
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 대입을 금지합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FObjectRegistry& operator=(const FObjectRegistry&) = delete;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FObjectRegistry);

    /**
     * @brief 객체를 빈 슬롯과 GUID 인덱스에 등록하고 핸들을 부여합니다.
     * @param[in,out] Object 대상 객체입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Register(UObject& Object);
    /**
     * @brief 객체를 등록 해제하고 슬롯 세대를 올려 기존 핸들을 무효화합니다.
     * @param[in,out] Object 대상 객체입니다.
     */
    void Unregister(UObject& Object);

    /**
     * @brief 중복되지 않는 유효 GUID로 객체의 식별자를 복원합니다.
     * @param[in,out] Object 대상 객체입니다.
     * @param[in] Guid 복원 또는 조회할 영속 GUID입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool RestoreGuid(UObject& Object, const FGuid& Guid);

    /**
     * @brief 핸들의 슬롯 범위와 세대가 일치하는 객체를 조회합니다.
     * @param[in] Handle 조회 또는 제거할 객체의 슬롯 세대 핸들입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UObject* Resolve(FObjectHandle Handle) const;
    /**
     * @brief GUID에 등록된 객체를 조회합니다.
     * @param[in] Guid 복원 또는 조회할 영속 GUID입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UObject* Find(const FGuid& Guid) const;

    /**
     * @brief 현재 레지스트리에 등록된 객체 개수를 반환합니다.
     * @return 현재 레지스트리에 등록된 객체 개수입니다.
     */
    std::size_t GetObjectCount() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 객체 주소와 세대 슬롯 목록입니다.
     */
    TArray<FSlot> mSlots{};
    ReflectProperty(mSlots, "Slots", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 재사용 가능한 슬롯 인덱스 목록입니다.
     */
    TArray<Uint32> mFreeSlots{};
    ReflectProperty(mFreeSlots, "FreeSlots", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief GUID에서 등록 객체로의 비소유 인덱스입니다.
     */
    TMap<FGuid, UObject*, FGuidHash> mObjectsByGuid{};
    ReflectProperty(mObjectsByGuid, "ObjectsByGuid", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
