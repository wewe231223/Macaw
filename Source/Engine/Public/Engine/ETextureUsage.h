/**
 * @file ETextureUsage.h
 * @brief 텍스처의 GPU 사용 용도 플래그와 비트 연산에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

/**
 * @brief GPU 텍스처를 읽거나 출력에 사용할 용도를 비트 플래그로 나타냅니다.
 */
enum class ETextureUsage : Uint8 {
    /**
     * @brief GPU 사용 용도를 지정하지 않은 상태입니다.
     */
    None = 0,
    /**
     * @brief 셰이더 리소스 뷰를 통한 텍스처 읽기를 허용합니다.
     */
    ShaderResource = 1,
    /**
     * @brief 색상 렌더 타깃으로의 출력을 허용합니다.
     */
    RenderTarget = 2,
    /**
     * @brief 깊이·스텐실 버퍼로의 출력을 허용합니다.
     */
    DepthStencil = 4
};

/**
 * @brief 이 열거형의 리플렉션 메타데이터를 반환합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */
ReflectEnum(ETextureUsage, None, ShaderResource, RenderTarget, DepthStencil);
/**
 * @brief 타입 태그로 ETextureUsage의 열거형 메타데이터를 조회합니다.
 * @fn UEnum* GetNativeEnum(TNativeEnum<ETextureUsage>)
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */

/**
 * @brief 텍스처 사용 용도 플래그를 비트 OR로 결합합니다.
 * @param[in] Left 비교하거나 결합할 왼쪽 값입니다.
 * @param[in] Right 비교하거나 결합할 오른쪽 값입니다.
 * @return 비트 연산을 적용한 텍스처 사용 용도 플래그입니다.
 */
ETextureUsage operator|(ETextureUsage Left, ETextureUsage Right);
/**
 * @brief 텍스처 사용 용도 플래그에서 공통으로 설정된 비트를 구합니다.
 * @param[in] Left 비교하거나 결합할 왼쪽 값입니다.
 * @param[in] Right 비교하거나 결합할 오른쪽 값입니다.
 * @return 비트 연산을 적용한 텍스처 사용 용도 플래그입니다.
 */
ETextureUsage operator&(ETextureUsage Left, ETextureUsage Right);
