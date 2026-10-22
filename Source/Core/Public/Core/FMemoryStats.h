/**
 * @file FMemoryStats.h
 * @brief 객체 개수와 추적 중인 힙 사용량의 스냅샷입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <cstddef>

/**
 * @brief 객체 개수와 추적 중인 힙 사용량의 스냅샷입니다.
 * @ingroup Core
 */
struct FMemoryStats {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FMemoryStats);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FMemoryStats);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 현재 생존 중인 UObject 개수입니다.
     */
    std::size_t mLiveObjects{};
    ReflectProperty(mLiveObjects, "LiveObjects", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 지금까지 생성한 UObject 개수입니다.
     */
    std::size_t mCreatedObjects{};
    ReflectProperty(mCreatedObjects, "CreatedObjects", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 현재 추적 중인 힙 할당 바이트 수입니다.
     */
    std::size_t mHeapBytes{};
    ReflectProperty(mHeapBytes, "HeapBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 추적 중인 힙 사용량의 최댓값입니다.
     */
    std::size_t mPeakHeapBytes{};
    ReflectProperty(mPeakHeapBytes, "PeakHeapBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
