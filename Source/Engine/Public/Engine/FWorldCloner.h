/**
 * @file FWorldCloner.h
 * @brief 월드 소유 그래프의 메모리 내 복제를 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"
#include "Engine/EWorldType.h"

class UObject;
class UWorld;
struct FObjectHandle;
class FWorldContext;
class FObjectDuplicationContext;

/**
 * @brief 월드의 소유 그래프를 새로운 객체 식별자로 메모리 내 복제합니다.
 * @ingroup Engine
 * @note 런타임 상태와 렌더 장면은 공유하지 않습니다. 모든 객체를 만든 뒤 PostDuplicate()를 호출하여 사용자 참조를 재연결합니다.
 */
class FWorldCloner {
public:
    /**
     * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
     */
    ReflectTypes(FWorldCloner);

public:
    /**
     * @brief 월드 복제기의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FWorldCloner);
    static TArray<FObjectHandle> DuplicateActors(UWorld* World, const TArray<FObjectHandle>& Actors, FString& Error);
    /**
     * @brief 월드, 영속 레벨, 액터와 컴포넌트를 독립 컨텍스트로 복제합니다.
     * @param[in] Source 콜백을 전달 중이지 않은 원본 컨텍스트입니다.
     * @param[in] Type 복제 월드의 용도입니다.
     * @param[out] Duplication 성공한 복제의 비소유 대응표이며 실패하면 비워집니다.
     * @param[out] Error 실패 원인이며 성공하면 비워집니다.
     * @return 엔진에 아직 등록되지 않은 정지 상태 컨텍스트의 단독 소유권이며 실패하면 nullptr입니다.
     * @note Transient 필드와 UObject 식별자는 복사하지 않습니다. 지원하지 않는 프로퍼티나 참조 재연결 실패는 전체 복제를 취소합니다. 렌더 장면은 엔진이 인수할 때 별도로 할당합니다.
     */
    static std::unique_ptr<FWorldContext> Clone(const FWorldContext* Source, EWorldType Type, FObjectDuplicationContext* Duplication, FString& Error);

private:
    /**
     * @brief 월드 복제기의 리플렉션 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
};
