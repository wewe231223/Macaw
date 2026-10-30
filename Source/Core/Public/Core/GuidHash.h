/**
 * @file GuidHash.h
 * @brief GUID의 해시 컨테이너 연동을 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"

namespace Core {

    /**
     * @brief FGuid를 해시 컨테이너의 키로 사용하기 위한 해시 함수 객체입니다.
     * @ingroup Core
     */
    struct FGuidHash {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FGuidHash);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FGuidHash);

        /**
         * @brief GUID의 해시를 계산합니다.
         * @param[in] Value 입력 값입니다.
         * @return GUID 바이트로 계산한 해시 값입니다.
         */
        std::size_t operator()(const Core::FGuid& Value) const noexcept;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();
    };

}
