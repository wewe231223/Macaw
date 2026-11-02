/**
 * @file FSequenceTrack.h
 * @brief 바인딩에 속한 시퀀스 트랙의 공통 인터페이스입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Sequence/ESequenceTrackType.h"
#include "Core/ReflectionMacros.h"

namespace Core {
    class FArchive;
}

namespace Asset {

    /**
     * @brief 바인딩에 속한 시퀀스 트랙의 공통 인터페이스입니다.
     * @ingroup Asset
     */
    class FSequenceTrack {
    public:
        /**
         * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
         */
        ReflectTypes(FSequenceTrack);

    public:
        /**
         * @brief 파생 트랙을 기반 타입으로 해제할 수 있게 합니다.
         */
        virtual ~FSequenceTrack();

    public:
        /**
         * @brief 정적 구조체 메타데이터를 반환합니다.
         * @return 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSequenceTrack);

        /**
         * @brief 트랙의 구체 타입을 반환합니다.
         * @return 트랙 타입입니다.
         */
        virtual Asset::ESequenceTrackType GetType() const = 0;

        /**
         * @brief 아카이브 모드에 따라 트랙 내용을 읽거나 저장합니다.
         * @param[in,out] Archive 값을 읽거나 쓸 아카이브입니다.
         */
        virtual void Serialize(Core::FArchive& Archive) = 0;

    private:
        /**
         * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
         */
        ReflectMembers();
    };

}
