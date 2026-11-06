/**
 * @file FSequenceTransformTrack.cpp
 * @brief 바인딩 대상의 변환을 제어하는 트랙에 대한 선언입니다.
 * @ingroup Asset
 */

#include "Asset/Sequence/FSequenceTransformTrack.h"
#include "Core/Archive.h"

#include <string>

namespace Asset {

    /**
     * @brief 트랙의 구체 타입을 반환합니다.
     */
    Asset::ESequenceTrackType Asset::FSequenceTransformTrack::GetType() const {
        return Asset::ESequenceTrackType::Transform;
    }

    /**
     * @brief 아카이브 모드에 따라 트랙 내용을 읽거나 저장합니다.
     */
    void Asset::FSequenceTransformTrack::Serialize(Core::FArchive& Archive) {
        Core::Uint32 Count{static_cast<Core::Uint32>(mSections.size())};

        if (!Archive.BeginArray("Sections", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            mSections.clear();
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            if (Archive.IsLoading()) {
                mSections.emplace_back();
            }

            mSections[Index].Serialize(Archive);
            Archive.EndObject();
        }

        Archive.EndArray();
    }

    /**
     * @brief 편집 대상인 첫 섹션을 반환하고 섹션이 없으면 새로 만듭니다.
     */
    Asset::FSequenceTransformSection& Asset::FSequenceTransformTrack::FindOrAddEditSection() {
        if (mSections.empty()) {
            mSections.emplace_back();
        }

        return mSections.front();
    }

    /**
     * @brief 모든 섹션이 지정한 시간을 포함하도록 넓힙니다.
     */
    void Asset::FSequenceTransformTrack::ExpandSections(Core::Int32 Tick) {
        for (Asset::FSequenceTransformSection& Section : mSections) {
            Section.ExpandToInclude(Tick);
        }
    }

    /**
     * @brief 지정한 시간에 적용할 섹션을 찾습니다.
     */
    const Asset::FSequenceTransformSection* Asset::FSequenceTransformTrack::FindSection(double Tick) const {
        for (auto Section{mSections.rbegin()}; Section != mSections.rend(); ++Section) {
            if (Section->Contains(Tick)) {
                return &*Section;
            }
        }

        return nullptr;
    }

}
