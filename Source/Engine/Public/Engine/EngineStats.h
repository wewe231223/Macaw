/**
 * @file EngineStats.h
 * @brief 엔진·월드 처리의 시간과 개수를 측정하는 통계 항목입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Stats.h"

namespace Engine {

    namespace EngineStats {
        /**
         * @brief 엔진 Tick 처리 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat EngineTick;

        /**
         * @brief 월드 렌더 장면 동기화 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat Synchronize;

        /**
         * @brief 월드 Tick 처리 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat WorldTick;

        /**
         * @brief 프리미티브 렌더 갱신 처리 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat RenderUpdates;

        /**
         * @brief 월드 레이캐스트 처리 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat Raycast;

        /**
         * @brief 호출한 액터 Tick 수를 측정하는 통계 항목입니다.
         */
        extern Core::FStat ActorTicks;

        /**
         * @brief 호출한 컴포넌트 Tick 수를 측정하는 통계 항목입니다.
         */
        extern Core::FStat ComponentTicks;

        /**
         * @brief 호출한 월드 Tick 수를 측정하는 통계 항목입니다.
         */
        extern Core::FStat WorldTicks;

        /**
         * @brief 엔진에 등록된 월드 수를 측정하는 통계 항목입니다.
         */
        extern Core::FStat Worlds;

        /**
         * @brief 등록 월드의 액터 수를 측정하는 통계 항목입니다.
         */
        extern Core::FStat Actors;

        /**
         * @brief 등록 월드의 컴포넌트 수를 측정하는 통계 항목입니다.
         */
        extern Core::FStat Components;

        /**
         * @brief 공유 CPU 메시 지오메트리의 바이트 수를 측정하는 통계 항목입니다.
         */
        extern Core::FStat SharedGeometry;

    }

}
