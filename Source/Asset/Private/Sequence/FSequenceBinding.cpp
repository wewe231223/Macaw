#include "Asset/Sequence/FSequenceBinding.h"
#include "Asset/Sequence/FSequencePropertyTrack.h"
#include "Asset/Sequence/FSequenceTransformTrack.h"
#include "Core/Archive.h"

#include <string>

namespace Asset {

    namespace {
        std::unique_ptr<Asset::FSequenceTrack> CreateTrack(const Core::FString& Type) {
            if (Type == "Transform") {
                return std::make_unique<Asset::FSequenceTransformTrack>();
            }

            if (Type == "Property") {
                return std::make_unique<Asset::FSequencePropertyTrack>();
            }

            return nullptr;
        }

        Core::FString ToTrackTypeName(Asset::ESequenceTrackType Type) {
            return Type == Asset::ESequenceTrackType::Transform ? "Transform" : "Property";
        }
    }

    const Core::FGuid& Asset::FSequenceBinding::GetId() const {
        return mId;
    }

    const Core::FString& Asset::FSequenceBinding::GetName() const {
        return mName;
    }

    const Core::TArray<std::unique_ptr<Asset::FSequenceTrack>>& Asset::FSequenceBinding::GetTracks() const {
        return mTracks;
    }

    void Asset::FSequenceBinding::SetName(Core::FString Name) {
        mName = std::move(Name);
    }

    Asset::FSequenceTransformTrack* Asset::FSequenceBinding::FindTransformTrack() const {
        for (const auto& Track : mTracks) {
            if (Track->GetType() == Asset::ESequenceTrackType::Transform) {
                return static_cast<Asset::FSequenceTransformTrack*>(Track.get());
            }
        }

        return nullptr;
    }

    Asset::FSequenceTransformTrack& Asset::FSequenceBinding::FindOrAddTransformTrack() {
        if (Asset::FSequenceTransformTrack* Track{FindTransformTrack()}; Track != nullptr) {
            return *Track;
        }

        auto Track{std::make_unique<Asset::FSequenceTransformTrack>()};
        Asset::FSequenceTransformTrack& Result{*Track};
        mTracks.push_back(std::move(Track));

        return Result;
    }

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

    void Asset::FSequenceBinding::RemoveTrack(const Asset::FSequenceTrack* Track) {
        std::erase_if(mTracks, [Track](const std::unique_ptr<Asset::FSequenceTrack>& Candidate) {
            return Candidate.get() == Track;
        });
    }

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
