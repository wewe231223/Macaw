/**
 * @file FObjectDuplicationContext.h
 * @brief 복제 객체의 참조 재연결을 위한 비소유 대응표를 정의합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

class UObject;

/**
 * @brief 복제 직후 참조 재연결에 사용할 원본과 복제본의 대응표입니다.
 * @ingroup CoreUObject
 * @note 모든 주소는 비소유이며 원본이나 복제본의 수명을 연장하지 않습니다. 관련 객체가 제거되기 전에만 사용해야 합니다.
 */
class FObjectDuplicationContext {
public:
    /**
     * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
     */
    ReflectTypes(FObjectDuplicationContext);

public:
    /**
     * @brief 복제 대응표의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FObjectDuplicationContext);
    /**
     * @brief 서로 다른 동일 클래스 객체의 대응 관계를 등록합니다.
     * @param[in] Source 복제 원본입니다.
     * @param[in,out] Duplicate 원본에 대응하는 복제본입니다.
     * @return 클래스가 같고 원본 키가 중복되지 않아 등록되면 true입니다.
     */
    bool Add(const UObject* Source, UObject* Duplicate);
    /**
     * @brief 원본 주소에 대응하는 복제본을 조회합니다.
     * @param[in] Source 조회할 원본의 비소유 주소입니다.
     * @return 등록된 복제본의 비소유 주소이며 대응 관계가 없으면 nullptr입니다.
     */
    UObject* FindDuplicate(const UObject* Source) const;
    /**
     * @brief 등록된 모든 대응 관계를 반환합니다.
     * @return 이 컨텍스트가 보유한 비소유 주소 대응표의 참조입니다.
     */
    const TMap<const UObject*, UObject*>& GetObjects() const;
    /**
     * @brief 객체를 삭제하지 않고 대응 관계만 비웁니다.
     */
    void Clear();

private:
    /**
     * @brief 대응표의 리플렉션 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 원본 주소를 복제본 주소로 연결하는 비소유 대응표입니다.
     */
    TMap<const UObject*, UObject*> mObjects{};
    ReflectProperty(mObjects, "Objects", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
