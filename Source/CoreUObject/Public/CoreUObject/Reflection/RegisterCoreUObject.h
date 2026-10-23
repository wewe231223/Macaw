/**
 * @file RegisterCoreUObject.h
 * @brief Core 및 CoreUObject 모듈의 리플렉션 등록 진입점입니다.
 * @ingroup CoreUObject
 */

#pragma once

/**
 * @brief 모듈별 생성된 리플렉션 등록 진입점입니다.
 * @ingroup CoreUObject
 */
namespace ReflectionModules {
    /**
     * @brief Core와 CoreUObject의 생성된 리플렉션 등록을 실행합니다.
     */
    void RegisterCoreUObject();
}
