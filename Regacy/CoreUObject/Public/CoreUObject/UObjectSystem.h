/**
 * @file UObjectSystem.h
 * @brief UObject의 등록, 핸들 해석과 GUID·이름·Outer 기반 검색을 제공합니다.
 */

#pragma once

#include "Core/CoreMinimal.h"
#include "CoreUObject/FObjectHandle.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FName.h"
#include "CoreUObject/UObject.h"
#include <string_view>

/**
 * @brief 객체의 소유권과 별도로 등록 정보와 식별·포함 관계를 관리합니다.
 */
namespace UObjectSystem {
    /**
     * @brief 객체를 등록하고 슬롯 인덱스와 세대가 담긴 핸들을 반환합니다.
     * @param Object 등록할 객체입니다. 이미 등록된 객체는 기존 핸들을 반환합니다.
     * @return 등록 핸들입니다. null, 파괴 중인 객체, 클래스 기본 객체, GUID 또는 이름 충돌이면 무효 핸들입니다.
     * @note 이름이 없으면 같은 Outer 안에서 고유한 이름을 생성합니다. 객체의 소유권은 인수하지 않습니다.
     */
    FObjectHandle Register(UObject* Object);

    /**
     * @brief GUID를 복원한 뒤 객체를 등록합니다.
     * @param Object 등록할 객체입니다.
     * @param InGuid 복원할 유효한 GUID입니다.
     * @return 등록 핸들입니다. 실패하면 무효 핸들을 반환하고 등록 실패 시 이전 GUID로 복원합니다.
     */
    FObjectHandle RegisterWithGuid(UObject* Object, const FGuid& InGuid);

    /**
     * @brief 객체와 핸들이 일치하면 등록을 해제하고 슬롯 세대를 증가시킵니다.
     * @param Object 등록을 해제할 객체입니다.
     * @param Handle 해당 객체의 현재 등록 핸들입니다.
     * @note 객체를 삭제하지 않습니다. 객체가 null이거나 핸들이 일치하지 않으면 아무 작업도 하지 않습니다.
     */
    void Unregister(UObject* Object, FObjectHandle Handle);

    /**
     * @brief 슬롯 인덱스와 세대를 검사해 현재 등록된 객체를 찾습니다.
     * @param Handle 해석할 핸들입니다.
     * @return 현재 객체입니다. 무효 핸들, 범위 밖 인덱스, 빈 슬롯 또는 세대 불일치이면 nullptr입니다.
     */
    UObject* Resolve(FObjectHandle Handle);

    /**
     * @brief GUID에 대응하는 등록 핸들을 찾습니다.
     * @param Guid 검색할 GUID입니다.
     * @return 등록 핸들입니다. 찾지 못하면 무효 핸들입니다.
     */
    FObjectHandle FindHandleByGuid(const FGuid& Guid);

    /**
     * @brief 객체에 저장된 등록 핸들을 반환합니다.
     * @param Object 조회할 객체입니다.
     * @return 객체의 핸들입니다. Object가 nullptr이면 무효 핸들입니다.
     */
    FObjectHandle GetHandle(const UObject* Object);

    /**
     * @brief 지정한 Outer에 직접 속한 등록 객체를 이름으로 찾습니다.
     * @param Outer 이름 검색의 기준 Outer입니다. nullptr이면 최상위 객체를 검색합니다.
     * @param Name 검색할 이름입니다.
     * @return 일치하는 객체입니다. 없으면 nullptr입니다.
     */
    UObject* FindObject(const UObject* Outer, FName Name);

    /**
     * @brief 점으로 구분한 이름 경로를 Outer부터 차례로 탐색합니다.
     * @param Path 검색할 이름 경로입니다.
     * @param Outer 첫 경로 성분을 찾을 기준입니다. nullptr이면 최상위부터 검색합니다.
     * @return 마지막 경로 객체입니다. 빈 경로, 잘못된 성분 또는 찾을 수 없는 경로이면 nullptr입니다.
     */
    UObject* FindObjectByPath(std::string_view Path, const UObject* Outer = nullptr);

    /**
     * @brief 지정한 Outer에 속한 등록 객체들을 반환합니다.
     * @param Outer 기준 Outer입니다. nullptr이면 최상위 객체만 반환합니다.
     * @param IncludeNested Outer가 null이 아닐 때 하위 Outer 관계까지 포함할지 여부입니다.
     * @return 슬롯 순서로 수집한 객체 포인터 배열입니다.
     */
    TArray<UObject*> GetObjectsWithOuter(const UObject* Outer, bool IncludeNested = false);

    /**
     * @brief 등록된 이름과 겹치지 않도록 필요하면 번호를 붙인 이름을 생성합니다.
     * @param Outer 이름의 중복을 검사할 Outer입니다.
     * @param BaseName 기준 이름입니다.
     * @param IgnoreObject 중복 검사에서 허용할 기존 객체입니다.
     * @return 사용 가능한 이름입니다. 기준 이름이 없거나 번호를 찾지 못하면 None입니다.
     * @note 이름을 예약하거나 객체를 등록하지 않습니다.
     */
    FName MakeUniqueObjectName(const UObject* Outer, FName BaseName, const UObject* IgnoreObject = nullptr);

    /**
     * @brief 객체 이름이 비어 있지 않고 경로 구분 문자를 포함하지 않는지 검사합니다.
     * @param Name 검사할 이름입니다.
     * @return 점, 콜론, 슬래시, 역슬래시와 null 문자를 포함하지 않는 이름이면 true입니다.
     */
    bool IsValidObjectName(FName Name);

    /**
     * @brief 이름과 Outer를 바꾸고 등록 인덱스와 Inner 관계를 갱신합니다.
     * @param Object 변경할 객체입니다.
     * @param NewName 새 이름입니다. 등록 객체에서 None이면 고유한 이름을 생성합니다.
     * @param NewOuter 새 Outer입니다. nullptr이면 최상위로 옮깁니다.
     * @return 변경 가능하면 true입니다. 파괴 상태, Outer 순환, 변경 정책 또는 등록 이름 충돌로 거부되면 false입니다.
     * @note 실제 변경이 있으면 OnIdentityChanged를 호출합니다.
     */
    bool Rename(UObject* Object, FName NewName, UObject* NewOuter);

    /**
     * @brief 객체 GUID와 등록된 GUID 검색 인덱스를 갱신합니다.
     * @param Object 변경할 객체입니다.
     * @param Guid 복원할 유효한 GUID입니다.
     * @return 성공하면 true입니다. null 객체, 무효 GUID 또는 등록 객체의 GUID 충돌이면 false입니다.
     */
    bool RestoreGuid(UObject* Object, const FGuid& Guid);

    /**
     * @brief 직접 속한 Inner들을 최상위로 분리하고 Object의 기존 Outer 관계도 해제합니다.
     * @param Object 관계를 해제할 유효한 객체입니다.
     * @note 등록된 Inner는 최상위 이름 충돌을 피하도록 이름을 조정하며 객체를 삭제하지 않습니다.
     */
    void DetachInners(UObject* Object);

    /**
     * @brief 현재 등록된 객체를 슬롯 순서로 수집합니다.
     * @return 빈 슬롯을 제외한 객체 포인터 배열입니다.
     */
    TArray<UObject*> GetObjects();

    /**
     * @brief 현재 등록된 객체 수를 반환합니다.
     * @return 빈 슬롯을 제외한 등록 객체 수입니다.
     */
    Uint32 GetObjectCount();
}
