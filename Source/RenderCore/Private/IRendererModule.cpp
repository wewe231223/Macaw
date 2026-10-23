/**
 * @file IRendererModule.cpp
 * @brief 엔진과 에디터가 사용하는 렌더러의 수명 및 프레임 제출 인터페이스입니다.
 * @ingroup RenderCore
 */

#include "RenderCore/IRendererModule.h"

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
IRendererModule::~IRendererModule() = default;
