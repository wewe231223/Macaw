/**
 * @file EClassFlags.cpp
 * @brief 리플렉션 클래스의 인스턴스 생성 정책을 지정하는 비트 플래그입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/EClassFlags.h"

/**
 * @brief 두 플래그 집합의 비트 OR 결합을 반환합니다.
 */
EClassFlags operator|(EClassFlags Left, EClassFlags Right) {
    return static_cast<EClassFlags>(static_cast<Uint32>(Left) | static_cast<Uint32>(Right));
}
