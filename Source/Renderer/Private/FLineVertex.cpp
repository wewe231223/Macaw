/**
 * @file FLineVertex.cpp
 * @brief 픽셀 두께를 가진 선분의 GPU 인스턴스 형식을 정의합니다.
 * @ingroup Renderer
 */

#include "Renderer/FLineVertex.h"
#include "Engine/FDrawLine.h"

namespace Renderer {

    Renderer::FLineVertex::FLineVertex(const Engine::FDrawLine& Line)
		: mStartAndWidth{Line.mStart, Line.mThickness},
		  mEnd{Line.mEnd, 0.0f},
		  mColor{Line.mColor, 1.0f} {
    }

}
