/**
 * @file FSequenceBinding.cpp
 * @brief 시퀀스 안에서 애니메이션 대상을 가리키는 바인딩에 대한 선언입니다.
 * @ingroup Asset
 */

#include "Asset/Sequence/FSequenceBinding.h"
#include "Asset/Sequence/FSequencePropertyTrack.h"
#include "Asset/Sequence/FSequenceTransformTrack.h"
#include "Core/Archive.h"

#include <string>

namespace Asset {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 저장된 타입 이름에 맞는 시퀀스 트랙을 생성합니다.
         * @param[in] Type 생성·조회·변환할 타입입니다.
         * @return 타입에 맞는 트랙이며 미지원 타입이면 nullptr입니다.
         */
        std::unique_ptr<Asset::FSequenceTrack> CreateTrack(const Core::FString& Type) {
            if (Type == "Transform") {
                return std::make_unique<Asset::FSequenceTransformTrack>();
            }

            if (Type == "Property") {
                return std::make_unique<Asset::FSequencePropertyTrack>();
            }

            return nullptr;
        }

        /**
         * @brief 트랙 종류에 대응하는 저장용 이름을 반환합니다.
         * @param[in] Type 생성·조회·변환할 타입입니다.
         * @return 트랙 종류에 대응하는 저장용 이름입니다.
         */
        Core::FString ToTrackTypeName(Asset::ESequenceTrackType Type) {
            return Type == Asset::ESequenceTrackType::Transform ? "Transform" : "Property";
        }
    }

    /**
     * @brief 씬의 대상과 연결하는 바인딩 ID를 반환합니다.
     */
    const Core::FGuid& Asset::FSequenceBinding::GetId() const {
        return mId;
    }

    /**
     * @brief 바인딩의 표시 이름을 반환합니다.
     */
    const Core::FString& Asset::FSequenceBinding::GetName() const {
        return mName;
    }

    /**
     * @brief 바인딩이 소유한 트랙 목록을 반환합니다.
     */
    const Core::TArray<std::unique_ptr<Asset::FSequenceTrack>>& Asset::FSequenceBinding::GetTracks() const {
        return mTracks;
    }

    /**
     * @brief 바인딩의 표시 이름을 설정합니다.
     */
    void Asset::FSequenceBinding::SetName(Core::FString Name) {
        mName = std::move(Name);
    }

    /**
     * @brief 변환 트랙을 조회합니다.
     */
    Asset::FSequenceTransformTrack* Asset::FSequenceBinding::FindTransformTrack() const {
        for (const auto& Track : mTracks) {
            if (Track->GetType() == Asset::ESequenceTrackType::Transform) {
                return static_cast<Asset::FSequenceTransformTrack*>(Track.get());
            }
        }

        return nullptr;
    }

    /**
     * @brief 변환 트랙을 조회하고 없으면 새로 만듭니다.
     */
    Asset::FSequenceTransformTrack& Asset::FSequenceBinding::FindOrAddTransformTrack() {
        if (Asset::FSequenceTransformTrack* Track{FindTransformTrack()}; Track != nullptr) {
            return *Track;
        }

        auto Track{std::make_unique<Asset::FSequenceTransformTrack>()};
        Asset::FSequenceTransformTrack& Result{*Track};

        mTracks.push_back(std::move(Track));

        return Result;
    }

    /**
     * @brief 지정한 프로퍼티의 트랙을 조회하고 없으면 새로 만듭니다.
     */
    Asset::FSequencePropertyTrack& Asset::FSequenceBinding::FindOrAddPropertyTrack(const Core::FString& PropertyName) {
        for (const auto& Track : mTracks) {
            if (Track->GetType() == Asset::ESequenceTrackType::Property && static_cast<Asset::FSequencePropertyTrack*>(Track.get())->GetPropertyName() == PropertyName) {
                return *static_cast<Asset::FSequencePropertyTrack*>(Track.get());
            }
        }

        auto Track{std::make_unique<Asset::FSequencePropertyTrack>()};
        Asset::FSequencePropertyTrack& Result{*Track};
        Result.SetPropertyName(PropertyName);
        mTracks.push_back(std::move(Track));

        return Result;
    }

    /**
     * @brief 트랙을 제거합니다.
     */
    void Asset::FSequenceBinding::RemoveTrack(const Asset::FSequenceTrack* Track) {
        std::erase_if(mTracks, [Track](const std::unique_ptr<Asset::FSequenceTrack>& Candidate) {
            return Candidate.get() == Track;
        });
    }

    /**
     * @brief 아카이브 모드에 따라 ID, 이름과 트랙을 읽거나 저장합니다.
     */
    void Asset::FSequenceBinding::Serialize(Core::FArchive& Archive) {
        Archive.Serialize("Id", mId);
        Archive.Serialize("Name", mName);

        Core::Uint32 Count{static_cast<Core::Uint32>(mTracks.size())};

        if (Archive.HasError() || !Archive.BeginArray("Tracks", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            mTracks.clear();
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Core::FString Type{Archive.IsLoading() ? Core::FString{} : ToTrackTypeName(mTracks[Index]->GetType())};

            Archive.Serialize("Type", Type);

            if (Archive.IsLoading() && !Archive.HasError()) {
                auto Track{CreateTrack(Type)};

                if (Track == nullptr) {
                    Archive.SetError("Unknown sequence track type: " + Type);
                } else {
                    mTracks.push_back(std::move(Track));
                }
            }

            if (!Archive.HasError()) {
                mTracks[Index]->Serialize(Archive);
            }

            Archive.EndObject();
        }

        Archive.EndArray();
    }

}
