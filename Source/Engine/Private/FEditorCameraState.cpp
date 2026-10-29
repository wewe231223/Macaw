/** @file FEditorCameraState.cpp @brief 원근 편집 카메라의 검증과 직렬화입니다. @ingroup Engine */
#include "Engine/FEditorCameraState.h"
#include "Core/Archive.h"

#include <cmath>

const FVector& FEditorCameraState::GetLocation() const {
    return mLocation;
}

const FRotator& FEditorCameraState::GetRotation() const {
    return mRotation;
}

float FEditorCameraState::GetFieldOfView() const {
    return mFieldOfView;
}

float FEditorCameraState::GetNearClip() const {
    return mNearClip;
}

float FEditorCameraState::GetFarClip() const {
    return mFarClip;
}

bool FEditorCameraState::IsValid() const {
    return mLocation.IsFinite() && mRotation.IsFinite() && std::isfinite(mFieldOfView) && mFieldOfView >= 1.0f && mFieldOfView <= 179.0f && std::isfinite(mNearClip) && std::isfinite(mFarClip) && mNearClip > 0.0f && mFarClip > mNearClip;
}

bool FEditorCameraState::operator==(const FEditorCameraState& Other) const {
    return mLocation == Other.mLocation && mRotation == Other.mRotation && mFieldOfView == Other.mFieldOfView && mNearClip == Other.mNearClip && mFarClip == Other.mFarClip;
}

bool FEditorCameraState::Set(const FVector& Location, const FRotator& Rotation, float FieldOfView, float NearClip, float FarClip) {
    FEditorCameraState Candidate{};
    Candidate.mLocation = Location;
    Candidate.mRotation = Rotation;
    Candidate.mFieldOfView = FieldOfView;
    Candidate.mNearClip = NearClip;
    Candidate.mFarClip = FarClip;
    if (!Candidate.IsValid()) {
        return false;
    }
    *this = Candidate;
    return true;
}

void FEditorCameraState::Serialize(FArchive& Archive) {
    FEditorCameraState Candidate{*this};
    Archive.Serialize("Location", Candidate.mLocation);
    Archive.Serialize("Rotation", Candidate.mRotation);
    Archive.Serialize("FOV", Candidate.mFieldOfView);
    Archive.Serialize("NearClip", Candidate.mNearClip);
    Archive.Serialize("FarClip", Candidate.mFarClip);
    if (!Archive.HasError() && !Candidate.IsValid()) {
        Archive.SetError("Invalid perspective editor camera");
    }
    if (Archive.IsLoading() && !Archive.HasError()) {
        *this = Candidate;
    }
}
