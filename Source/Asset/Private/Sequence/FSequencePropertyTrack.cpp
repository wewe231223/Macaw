/**
 * @file FSequencePropertyTrack.cpp
 * @brief 바인딩 대상의 float 프로퍼티를 제어하는 트랙에 대한 선언입니다.
 * @ingroup Asset
 */

#include "Asset/Sequence/FSequencePropertyTrack.h"
#include "Core/Archive.h"

#include <utility>

namespace Asset {

    /**
     * @brief 트랙의 구체 타입을 반환합니다.
     */
    Asset::ESequenceTrackType Asset::FSequencePropertyTrack::GetType() const {
        return Asset::ESequenceTrackType::Property;
    }

    /**
     * @brief 아카이브 모드에 따라 트랙 내용을 읽거나 저장합니다.
     */
    void Asset::FSequencePropertyTrack::Serialize(Core::FArchive& Archive) {
        Archive.Serialize("Property", mPropertyName);

        if (Archive.HasError()) {
            return;
        }

        if (mPropertyName.empty()) {
            Archive.SetError("Sequence property track has no property name");

            return;
        }

        if (Archive.BeginObject("Channel")) {
            mChannel.Serialize(Archive);
            Archive.EndObject();
        }
    }

    /**
     * @brief 제어할 프로퍼티의 리플렉션 이름을 반환합니다.
     */
    const Core::FString& Asset::FSequencePropertyTrack::GetPropertyName() const {
        return mPropertyName;
    }

    /**
     * @brief 제어할 프로퍼티의 리플렉션 이름을 설정합니다.
     */
    void Asset::FSequencePropertyTrack::SetPropertyName(Core::FString PropertyName) {
        mPropertyName = std::move(PropertyName);
    }

    /**
     * @brief 프로퍼티 값의 채널을 반환합니다.
     */
    Asset::FSequenceChannel& Asset::FSequencePropertyTrack::GetChannel() {
        return mChannel;
    }

    /**
     * @brief 프로퍼티 값의 채널을 반환합니다.
     */
    const Asset::FSequenceChannel& Asset::FSequencePropertyTrack::GetChannel() const {
        return mChannel;
    }

}
