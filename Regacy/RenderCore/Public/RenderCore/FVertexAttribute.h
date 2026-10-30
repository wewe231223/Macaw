/**
 * @file FVertexAttribute.h
 * @brief 정점 속성의 타입 매핑과 연속 데이터의 비소유 읽기 전용 뷰를 제공합니다.
 */

#pragma once

#include "Core/CoreMinimal.h"

#include <concepts>
#include <ranges>
#include <span>
#include <type_traits>

/**
 * @brief 메시 정점에서 사용하는 속성 종류입니다.
 */
enum class EVertexAttribute : Uint8 {
    /**
     * @brief 3차원 정점 위치입니다.
     */
    Position,
    /**
     * @brief 3차원 정점 법선입니다.
     */
    Normal,
    /**
     * @brief 2차원 텍스처 좌표입니다.
     */
    UV,
    /**
     * @brief 네 성분 정점 색상입니다.
     */
    Color,

    /**
     * @brief 유효한 정점 속성 범위의 끝을 나타내는 경계 값입니다.
     */
    MAX
};

/**
 * @brief 정점 속성별 원소 타입을 지정하는 특성의 기본 선언입니다.
 * @tparam Attribute 타입을 조회할 정점 속성입니다. 제공된 특수화가 있어야 합니다.
 */
template <EVertexAttribute Attribute>
struct TVertexAttributeTraits;

/**
 * @brief 위치 속성에 FVector3 타입을 연결하는 특수화입니다.
 */
template <>
struct TVertexAttributeTraits<EVertexAttribute::Position> {
    /**
     * @brief 위치 속성의 3차원 벡터 타입입니다.
     */
    using Type = FVector3;
};

/**
 * @brief 법선 속성에 FVector3 타입을 연결하는 특수화입니다.
 */
template <>
struct TVertexAttributeTraits<EVertexAttribute::Normal> {
    /**
     * @brief 법선 속성의 3차원 벡터 타입입니다.
     */
    using Type = FVector3;
};

/**
 * @brief UV 속성에 FVector2D 타입을 연결하는 특수화입니다.
 */
template <>
struct TVertexAttributeTraits<EVertexAttribute::UV> {
    /**
     * @brief UV 속성의 2차원 벡터 타입입니다.
     */
    using Type = FVector2D;
};

/**
 * @brief 색상 속성에 FColor4 타입을 연결하는 특수화입니다.
 */
template <>
struct TVertexAttributeTraits<EVertexAttribute::Color> {
    /**
     * @brief 색상 속성의 네 성분 타입입니다.
     */
    using Type = FColor4;
};

/**
 * @brief 지정한 정점 속성의 원소 타입 별칭입니다.
 * @tparam Attribute 조회할 정점 속성입니다.
 */
template <EVertexAttribute Attribute>
using TVertexAttributeElementType = typename TVertexAttributeTraits<Attribute>::Type;

/**
 * @brief 속성 종류와 읽기 전용 span을 연결하는 비소유 정점 데이터 뷰입니다.
 * @tparam Attribute 뷰가 표현하는 정점 속성입니다.
 * @note 원본 데이터는 뷰를 사용하는 동안 살아 있어야 하며 재할당으로 주소가 바뀌면 안 됩니다.
 */
template <EVertexAttribute Attribute>
struct TVertexAttributeView {
    /**
     * @brief 해당 정점 속성의 원소 타입입니다.
     */
    using ElementType = TVertexAttributeElementType<Attribute>;

    /**
     * @brief 뷰의 컴파일 시 정점 속성 종류입니다.
     */
    static constexpr EVertexAttribute AttributeType{Attribute};

    /**
     * @brief 정점 속성 원소의 비소유 읽기 전용 범위입니다.
     */
    std::span<const ElementType> mData{};
};

/**
 * @brief 정점 속성 뷰가 아닌 타입의 기본 판정입니다.
 * @tparam T 검사할 타입입니다.
 */
template <typename T>
struct TIsVertexAttributeView : std::false_type {
};

/**
 * @brief TVertexAttributeView 타입에 대한 참 판정 특수화입니다.
 * @tparam Attribute 뷰의 정점 속성 종류입니다.
 */
template <EVertexAttribute Attribute>
struct TIsVertexAttributeView<TVertexAttributeView<Attribute>> : std::true_type {
};

/**
 * @brief 참조와 cv 한정자를 제거한 타입이 정점 속성 뷰인지 제한하는 개념입니다.
 * @tparam T 검사할 타입입니다.
 */
template <typename T>
concept CVertexAttributeView = TIsVertexAttributeView<std::remove_cvref_t<T>>::value;

/**
 * @brief 연속된 원소 범위를 복사 없이 정점 속성 뷰로 만듭니다.
 * @tparam Attribute 만들 뷰의 속성 종류입니다.
 * @tparam Range 크기를 조회할 수 있고 속성 원소 타입이 정확히 일치하는 연속 범위입니다.
 * @param Data 뷰를 사용하는 동안 유효하게 유지할 원본 데이터입니다.
 * @return Data의 주소와 크기를 보관한 읽기 전용 뷰입니다.
 * @note 임시 컨테이너에서 생성한 뷰를 컨테이너 소멸 후 사용하면 안 됩니다.
 */
template <EVertexAttribute Attribute, std::ranges::contiguous_range Range>
    requires std::ranges::sized_range<Range> && std::same_as<std::ranges::range_value_t<Range>, TVertexAttributeElementType<Attribute>>
TVertexAttributeView<Attribute> MakeVertexAttribute(const Range& Data) {
    using ElementType = TVertexAttributeElementType<Attribute>;

    return TVertexAttributeView<Attribute>{std::span<const ElementType>{std::ranges::data(Data), std::ranges::size(Data)}};
}
