/**
 * @file EClassFlags.h
 * @brief 리플렉션 클래스의 인스턴스 생성 정책 플래그를 정의합니다.
 */
#pragma once

#include "Core/Common.h"

/**
 * @brief 일반 인스턴스와 클래스 기본 객체의 생성 정책을 나타내는 비트 플래그입니다.
 */
enum class EClassFlags : Uint32 {
    /**
     * @brief 별도 클래스 정책이 없는 기본값입니다.
     */
    None = 0,
    /**
     * @brief 일반 인스턴스 생성을 금지하는 추상 클래스 표시입니다.
     */
    Abstract = 1,
    /**
     * @brief 생성 팩터리를 제공하지 않는 클래스 표시입니다.
     */
    NoFactory = 2,
    /**
     * @brief 일반 객체와 기본 객체 생성 경로를 사용하지 않는 메타데이터 클래스 표시입니다.
     */
    Metadata = 4
};

/**
 * @brief 클래스 정책 플래그를 비트 OR로 결합합니다.
 * @param Left 첫 플래그입니다.
 * @param Right 두 번째 플래그입니다.
 * @return 결합한 플래그입니다.
 */
EClassFlags operator|(EClassFlags Left, EClassFlags Right);
