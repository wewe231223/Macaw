#include "Core/Archive.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FTransform.h"
#include "Core/Base/ErrorHandler.h"

FArchive::FArchive(EArchiveMode Mode)
	: mMode{Mode} {
}

FArchive::~FArchive() = default;

bool FArchive::IsLoading() const {
    return mMode == EArchiveMode::Loading;
}

bool FArchive::HasError() const {
    return !mError.empty();
}

const FString& FArchive::GetError() const {
    return mError;
}

void FArchive::SetError(const FString& Message) {
    if (mError.empty()) {
        mError = Message;
    }

    ErrorHandler::Fatal("Archive", mError);
}

void FArchive::Serialize(std::string_view Name, FGuid& Value) {
    FString Text{Value.ToString()};
    Serialize(Name, Text);

    if (IsLoading() && !HasError() && !FGuid::Parse(Text, Value)) {
        SetError("Invalid UUID: " + FString{Name});
    }
}

void FArchive::Serialize(std::string_view Name, FVector& Value) {
    if (BeginObject(Name)) {
        Serialize("X", Value.mX);
        Serialize("Y", Value.mY);
        Serialize("Z", Value.mZ);

        EndObject();
    }
}

void FArchive::Serialize(std::string_view Name, FRotator& Value) {
    if (BeginObject(Name)) {
        Serialize("Pitch", Value.mPitch);
        Serialize("Yaw", Value.mYaw);
        Serialize("Roll", Value.mRoll);

        EndObject();
    }
}

void FArchive::Serialize(std::string_view Name, FTransform& Value) {
    if (BeginObject(Name)) {
        Serialize("Location", Value.mLocation);
        Serialize("Rotation", Value.mRotation);
        Serialize("Scale", Value.mScale);

        EndObject();
    }

    if (IsLoading() && !Value.IsValid()) {
        SetError("Invalid transform: " + FString{Name});
    }
}
