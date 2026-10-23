/**
 * @file FAssetPath.h
 * @brief 에셋의 논리 경로 타입과 해시 함수를 정의한다.
 */
#pragma once

#include "Core/Common.h"
#include "Core/STL.h"

/**
 * @brief 에셋을 식별하는 논리 경로 문자열을 보관한다.
 * @details 생성자는 전달된 문자열을 그대로 보관한다. 유효성은 문자열이 비어 있는지로 판단하며, 비교와 해시는 경로 문자열을 기준으로 수행한다.
 */
struct FAssetPath {
    /** @brief 에셋의 논리 경로 문자열. 빈 문자열은 유효하지 않은 경로를 나타낸다. */
    FString mPath{};

    /** @brief 빈 문자열을 가진 유효하지 않은 에셋 경로를 생성한다. */
    FAssetPath() = default;

    /**
     * @brief 경로 문자열을 복사하여 에셋 경로를 생성한다.
     * @param InPath 보관할 논리 경로 문자열.
     */
    explicit FAssetPath(const FString& InPath);

    /**
     * @brief 경로 문자열을 이동하여 에셋 경로를 생성한다.
     * @param InPath 보관할 논리 경로 문자열. 문자열의 소유권을 이동한다.
     */
    explicit FAssetPath(FString&& InPath);

    /**
     * @brief 에셋 경로 문자열이 비어 있지 않은지 확인한다.
     * @return mPath가 비어 있지 않으면 true, 비어 있으면 false.
     * @details 경로 형식이나 실제 에셋의 존재 여부는 검사하지 않는다.
     */
    bool IsValid() const;

    /**
     * @brief 에셋 경로의 유효성을 불리언 값으로 변환한다.
     * @return IsValid()의 결과.
     */
    explicit operator bool() const;

    /**
     * @brief 두 에셋 경로의 문자열을 비교한다.
     * @param Other 비교할 에셋 경로.
     * @return 대소문자를 포함한 경로 문자열이 같으면 true, 다르면 false.
     */
    bool operator==(const FAssetPath& Other) const = default;
};

/**
 * @brief 엔진 값 타입의 표준 해시 특수화를 선언하는 네임스페이스입니다.
 */
namespace std {
    /** @brief FAssetPath를 해시 기반 컨테이너의 키로 사용할 수 있도록 해시 함수를 제공한다. */
    template <>
    struct hash<FAssetPath> {
        /**
         * @brief 에셋 경로 문자열의 해시 값을 계산한다.
         * @param AssetPath 해시 값을 계산할 에셋 경로.
         * @return AssetPath.mPath에 std::hash<FString>을 적용한 값.
         */
        std::size_t operator()(const FAssetPath& AssetPath) const noexcept;
    };
}
