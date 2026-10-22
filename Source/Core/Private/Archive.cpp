#include "Core/Archive.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FTransform.h"
#include "Core/Base/ErrorHandler.h"

#include <charconv>

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

void FArchive::Serialize(std::string_view Name, Uint64& Value) {
    FString Text{std::to_string(Value)};
    Serialize(Name, Text);

    if (IsLoading()) {
        const auto Result{std::from_chars(Text.data(), Text.data() + Text.size(), Value)};

        if (Result.ec != std::errc{} || Result.ptr != Text.data() + Text.size()) {
            SetError("Invalid uint64: " + FString{Name});
        }
    }
}

void FArchive::Serialize(std::string_view Name, FGuid& Value) {
    FString Text{Value.ToString()};
    Serialize(Name, Text);

    if (IsLoading() && !HasError() && !FGuid::Parse(Text, Value)) {
        SetError("Invalid UUID: " + FString{Name});
    }
}

void FArchive::Serialize(std::string_view Name, FVector2& Value) {
    if (BeginObject(Name)) {
        Serialize("X", Value.x);
        Serialize("Y", Value.y);

        EndObject();
    }
}

void FArchive::Serialize(std::string_view Name, FVector& Value) {
    if (BeginObject(Name)) {
        Serialize("X", Value.x);
        Serialize("Y", Value.y);
        Serialize("Z", Value.z);

        EndObject();
    }
}

void FArchive::Serialize(std::string_view Name, FVector4& Value) {
    if (BeginObject(Name)) {
        Serialize("X", Value.x);
        Serialize("Y", Value.y);
        Serialize("Z", Value.z);
        Serialize("W", Value.w);

        EndObject();
    }
}

void FArchive::Serialize(std::string_view Name, FQuat& Value) {
    if (BeginObject(Name)) {
        Serialize("X", Value.x);
        Serialize("Y", Value.y);
        Serialize("Z", Value.z);
        Serialize("W", Value.w);

        EndObject();
    }
}

void FArchive::Serialize(std::string_view Name, FRotator& Value) {
    if (BeginObject(Name)) {
        Serialize("Pitch", Value.pitch);
        Serialize("Yaw", Value.yaw);
        Serialize("Roll", Value.roll);

        EndObject();
    }
}

void FArchive::Serialize(std::string_view Name, FMatrix& Value) {
    if (BeginObject(Name)) {
        for (Int32 Row{}; Row < 4; ++Row) {
            for (Int32 Column{}; Column < 4; ++Column) {
                Serialize("M" + std::to_string(Row) + std::to_string(Column), Value(Row, Column));
            }
        }

        EndObject();
    }
}

void FArchive::Serialize(std::string_view Name, FTransform& Value) {
    if (BeginObject(Name)) {
        Serialize("Location", Value.mLocation);

        FRotator Rotation{Value.GetRotator()};
        Serialize("Rotation", Rotation);

        if (IsLoading() && !Value.SetRotator(Rotation)) {
            SetError("Invalid rotation");
        }

        Serialize("Scale", Value.mScale);

        EndObject();
    }

    if (IsLoading() && !Value.IsValid()) {
        SetError("Invalid transform: " + FString{Name});
    }
}

void FArchive::Serialize(std::string_view Name, FBox& Value) {
    if (BeginObject(Name)) {
        Serialize("Minimum", Value.mMinimum);
        Serialize("Maximum", Value.mMaximum);

        EndObject();
    }
}

void FArchive::Serialize(std::string_view Name, FRay& Value) {
    if (BeginObject(Name)) {
        Serialize("Origin", Value.mOrigin);
        Serialize("Direction", Value.mDirection);

        EndObject();
    }
}
