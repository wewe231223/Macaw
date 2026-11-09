/**
 * @file FEditorCameraState.cpp
 * @brief 원근 편집 카메라의 검증과 직렬화입니다.
 * @ingroup Engine
 */

#include "Engine/FEditorCameraState.h"
#include "Core/Archive.h"

#include <cmath>

namespace Engine {

    /**
     * @brief 위치를 반환합니다.
     */
    const Core::FVector& Engine::FEditorCameraState::GetLocation() const {
        return mLocation;
    }

    /**
     * @brief 회전을 반환합니다.
     */
    const Core::FRotator& Engine::FEditorCameraState::GetRotation() const {
        return mRotation;
    }

    /**
     * @brief 시야각을 반환합니다.
     */
    float Engine::FEditorCameraState::GetFieldOfView() const {
        return mFieldOfView;
    }

    /**
     * @brief 근평면 거리를 반환합니다.
     */
    float Engine::FEditorCameraState::GetNearClip() const {
        return mNearClip;
    }

    /**
     * @brief 원평면 거리를 반환합니다.
     */
    float Engine::FEditorCameraState::GetFarClip() const {
        return mFarClip;
    }

    /**
     * @brief 카메라의 유한성과 투영 범위를 검사합니다.
     */
    bool Engine::FEditorCameraState::IsValid() const {
        return mLocation.IsFinite() && mRotation.IsFinite() && std::isfinite(mFieldOfView) && mFieldOfView >= 1.0f && mFieldOfView <= 179.0f && std::isfinite(mNearClip) && std::isfinite(mFarClip) && mNearClip > 0.0f && mFarClip > mNearClip;
    }

    /**
     * @brief 저장 대상 카메라 필드가 같은지 비교합니다.
     */
    bool Engine::FEditorCameraState::operator==(const Engine::FEditorCameraState& Other) const {
        return mLocation == Other.mLocation && mRotation == Other.mRotation && mFieldOfView == Other.mFieldOfView && mNearClip == Other.mNearClip && mFarClip == Other.mFarClip;
    }

    /**
     * @brief 검증한 카메라 상태를 원자적으로 적용합니다.
     */
    bool Engine::FEditorCameraState::Set(const Core::FVector& Location, const Core::FRotator& Rotation, float FieldOfView, float NearClip, float FarClip) {
        Engine::FEditorCameraState Candidate{};
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

    /**
     * @brief 카메라 필드를 저장하거나 복원합니다.
     */
    void Engine::FEditorCameraState::Serialize(Core::FArchive& Archive) {
        Engine::FEditorCameraState Candidate{*this};

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

}
