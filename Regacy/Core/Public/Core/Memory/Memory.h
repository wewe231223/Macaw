/**
 * @file Memory.h
 * @brief 정렬과 통계 태그를 지원하는 엔진 메모리 할당 및 해제 인터페이스를 정의한다.
 */

#pragma once

#include <cstddef>
#include "Core/Stat/Stat.h"

/**
 * @brief 할당 정보를 내부 헤더에 보관하고 요청 크기와 태그에 따라 메모리 통계를 갱신한다.
 */
namespace Memory {
    /**
     * @brief 메모리 할당 용도를 구분하는 통계 태그의 별칭.
     */
    using EMemoryTag = Stat::EMemoryTag;

    /**
     * @brief 지정한 정렬을 만족하는 메모리를 할당하고 통계를 기록한다.
     * @param Size 요청한 바이트 수. 0이면 최소 1바이트의 저장 공간을 확보하되 통계에는 요청 크기 0을 기록한다.
     * @param Alignment 0이 아닌 2의 거듭제곱 정렬 크기. 내부 할당 헤더에 필요한 정렬보다 작으면 그 정렬까지 높인다.
     * @param Tag 할당 용도를 구분할 통계 태그. 기본값은 Unknown이다.
     * @return 정렬된 사용 영역의 포인터. Free()로 해제해야 한다.
     * @details 통계에는 내부 헤더와 정렬 여유 공간을 제외한 요청 크기를 기록한다. 잘못된 정렬, 크기 오버플로 또는 할당 실패는 치명적 오류로 보고한다.
     */
    void* Allocate(std::size_t Size, std::size_t Alignment, EMemoryTag Tag = EMemoryTag::Unknown);

    /**
     * @brief 엔진 메모리 할당을 해제하고 저장된 크기와 태그로 통계를 갱신한다.
     * @param Ptr Allocate()가 반환한 아직 해제하지 않은 포인터. nullptr이면 아무 작업도 하지 않는다.
     * @details 사용 영역 앞의 내부 헤더에서 원본 할당 주소, 요청 크기 및 태그를 읽어 해제한다.
     */
    void Free(void* Ptr) noexcept;
}
