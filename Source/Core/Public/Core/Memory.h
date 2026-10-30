/**
 * @file Memory.h
 * @brief 정렬된 UObject 메모리 할당과 통계를 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/FMemoryStats.h"

/**
 * @brief UObject의 정렬된 힙 할당과 메모리 및 객체 통계를 관리합니다.
 * @ingroup Core
 */
class FMemory {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FMemory);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FMemory);

    /**
     * @brief 요청한 정렬을 만족하는 힙 블록을 할당하고 사용량을 기록합니다.
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     * @param[in] Alignment 요청하는 바이트 정렬입니다.
     * @return FMemory::Free()로 해제해야 하는 정렬된 할당 주소입니다.
     */
    static void* Allocate(std::size_t Size, std::size_t Alignment);

    /**
     * @brief 정렬된 블록의 원본 주소를 해제하고 추적 중인 사용량을 차감합니다.
     * @param[in] Pointer 할당 또는 해제할 블록의 주소입니다.
     */
    static void Free(void* Pointer) noexcept;

    /**
     * @brief 생존 객체와 누적 생성 객체 수를 증가시킵니다.
     */
    static void ObjectCreated();

    /**
     * @brief 생존 객체 수를 감소시킵니다.
     */
    static void ObjectDestroyed();

    /**
     * @brief 현재 UObject 개수와 추적 중인 힙 사용량의 스냅샷을 반환합니다.
     * @return 현재 UObject 개수와 추적 중인 힙 사용량의 스냅샷입니다.
     */
    static FMemoryStats GetStats();

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
};
