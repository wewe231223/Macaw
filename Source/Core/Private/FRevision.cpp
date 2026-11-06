/**
 * @file FRevision.cpp
 * @brief 변경 감지 리비전 카운터를 갱신합니다.
 * @ingroup Core
 */

#include "Core/FRevision.h"

namespace Core {

    /**
     * @brief 지정한 정수 값으로 번호를 만듭니다.
     */
    Core::FRevision::FRevision(Core::Uint64 Value)
		: mValue{Value} {
    }

    /**
     * @brief 전역 카운터에서 이전에 발급한 적 없는 새 번호를 발급합니다.
     */
    Core::FRevision Core::FRevision::Next() {
        static Core::Uint64 Counter{};

        return Core::FRevision{++Counter};
    }

    /**
     * @brief 발급된 번호인지 검사합니다.
     */
    bool Core::FRevision::IsValid() const {
        return mValue != 0;
    }

    /**
     * @brief 번호의 정수 값을 반환합니다.
     */
    Core::Uint64 Core::FRevision::GetValue() const {
        return mValue;
    }

}
