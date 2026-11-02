/**
 * @file FSequenceBinding.h
 * @brief 시퀀스 안에서 애니메이션 대상을 가리키는 바인딩에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Sequence/FSequenceTrack.h"
#include "Core/Guid.h"

#include <memory>

namespace Asset {

    class FSequencePropertyTrack;
    class FSequenceTransformTrack;

    /**
     * @brief 시퀀스 안에서 애니메이션 대상을 가리키고 트랙을 소유합니다.
     * @ingroup Asset
     * @note 실제 월드 객체는 씬의 시퀀스 액터가 바인딩 ID로 연결합니다.
     */
    class FSequenceBinding {
    public:
        /**
         * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
         */
        ReflectTypes(FSequenceBinding);

    public:
        /**
         * @brief 정적 구조체 메타데이터를 반환합니다.
         * @return 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSequenceBinding);

        /**
         * @brief 씬의 대상과 연결하는 바인딩 ID를 반환합니다.
         * @return 바인딩 ID의 참조입니다.
         */
        const Core::FGuid& GetId() const;

        /**
         * @brief 바인딩의 표시 이름을 반환합니다.
         * @return 표시 이름의 참조입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 바인딩이 소유한 트랙 목록을 반환합니다.
         * @return 트랙 목록의 참조입니다.
         */
        const Core::TArray<std::unique_ptr<Asset::FSequenceTrack>>& GetTracks() const;

        /**
         * @brief 바인딩의 표시 이름을 설정합니다.
         * @param[in] Name 표시 이름입니다.
         */
        void SetName(Core::FString Name);

        /**
         * @brief 변환 트랙을 조회합니다.
         * @return 첫 변환 트랙의 비소유 주소이며 없으면 nullptr입니다.
         */
        Asset::FSequenceTransformTrack* FindTransformTrack() const;

        /**
         * @brief 변환 트랙을 조회하고 없으면 새로 만듭니다.
         * @return 바인딩이 소유한 변환 트랙의 참조입니다.
         */
        Asset::FSequenceTransformTrack& FindOrAddTransformTrack();

        /**
         * @brief 지정한 프로퍼티의 트랙을 조회하고 없으면 새로 만듭니다.
         * @param[in] PropertyName 제어할 프로퍼티의 리플렉션 이름입니다.
         * @return 바인딩이 소유한 프로퍼티 트랙의 참조입니다.
         */
        Asset::FSequencePropertyTrack& FindOrAddPropertyTrack(const Core::FString& PropertyName);

        /**
         * @brief 트랙을 제거합니다.
         * @param[in] Track 제거할 트랙의 비소유 주소입니다.
         */
        void RemoveTrack(const Asset::FSequenceTrack* Track);

        /**
         * @brief 아카이브 모드에 따라 ID, 이름과 트랙을 읽거나 저장합니다.
         * @param[in,out] Archive 값을 읽거나 쓸 아카이브입니다.
         */
        void Serialize(Core::FArchive& Archive);

    private:
        /**
         * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 씬의 대상과 연결하는 바인딩 ID입니다.
         */
        Core::FGuid mId{};
        ReflectProperty(mId, "Id", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 편집 화면에 표시할 이름입니다.
         */
        Core::FString mName{};
        ReflectProperty(mName, "Name", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 바인딩이 소유한 트랙 목록입니다.
         */
        Core::TArray<std::unique_ptr<Asset::FSequenceTrack>> mTracks{};
        ReflectProperty(mTracks, "Tracks", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
