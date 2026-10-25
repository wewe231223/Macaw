/**
 * @file ETextureUsage.cpp
 * @brief 텍스처의 GPU 사용 용도 플래그와 비트 연산에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/ETextureUsage.h"

/**
 * @brief 텍스처 사용 용도 플래그를 비트 OR로 결합합니다.
 * @param[in] Left 비교하거나 결합할 왼쪽 값입니다.
 * @param[in] Right 비교하거나 결합할 오른쪽 값입니다.
 * @return 비트 연산을 적용한 텍스처 사용 용도 플래그입니다.
 */
ETextureUsage operator|(ETextureUsage Left, ETextureUsage Right) {
    return static_cast<ETextureUsage>(static_cast<Uint8>(Left) | static_cast<Uint8>(Right));
}

/**
 * @brief 텍스처 사용 용도 플래그에서 공통으로 설정된 비트를 구합니다.
 * @param[in] Left 비교하거나 결합할 왼쪽 값입니다.
 * @param[in] Right 비교하거나 결합할 오른쪽 값입니다.
 * @return 비트 연산을 적용한 텍스처 사용 용도 플래그입니다.
 */
ETextureUsage operator&(ETextureUsage Left, ETextureUsage Right) {
    return static_cast<ETextureUsage>(static_cast<Uint8>(Left) & static_cast<Uint8>(Right));
}
