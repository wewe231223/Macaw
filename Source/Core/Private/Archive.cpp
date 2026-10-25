/**
 * @file Archive.cpp
 * @brief 로드·저장 공통 아카이브와 복합 수학 값 직렬화를 제공합니다.
 * @ingroup Core
 */

#include "Core/Archive.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FTransform.h"
#include "Core/Base/ErrorHandler.h"

#include <charconv>

/**
 * @brief FArchive의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FArchive::FArchive(EArchiveMode Mode)
	: mMode{Mode} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FArchive::~FArchive() = default;

/**
 * @brief 아카이브가 로드 모드인지 검사합니다.
 */
bool FArchive::IsLoading() const {
    return mMode == EArchiveMode::Loading;
}

bool FArchive::IsDuplicating() const {
    return mObjectGuidRemapping != nullptr;
}

void FArchive::SetObjectGuidRemapping(const TMap<FGuid, FGuid, FGuidHash>* Remapping) {
    mObjectGuidRemapping = Remapping;
}

void FArchive::SerializeObjectGuid(std::string_view Name, FGuid& Value) {
    Serialize(Name, Value);

    if (IsLoading() && !HasError()) {
        RemapObjectReference(Value);
    }
}

bool FArchive::RemapObjectReference(FGuid& Value) {
    if (mObjectGuidRemapping == nullptr || !Value.IsValid()) {
        return true;
    }

    const auto Position{mObjectGuidRemapping->find(Value)};

    if (Position == mObjectGuidRemapping->end() || !Position->second.IsValid()) {
        SetError("Object reference is outside the duplication graph");
        return false;
    }

    Value = Position->second;
    return true;
}

/**
 * @brief 아카이브에 오류 메시지가 기록되어 있는지 검사합니다.
 */
bool FArchive::HasError() const {
    return !mError.empty();
}

/**
 * @brief 처음 발생한 오류 메시지를 반환합니다.
 */
const FString& FArchive::GetError() const {
    return mError;
}

/**
 * @brief 아카이브에 처음 발생한 오류 메시지를 보관합니다.
 */
void FArchive::SetError(const FString& Message) {
    if (mError.empty()) {
        mError = Message;
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
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

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FArchive::Serialize(std::string_view Name, FGuid& Value) {
    FString Text{Value.ToString()};
    Serialize(Name, Text);

    if (IsLoading() && !HasError() && !FGuid::Parse(Text, Value)) {
        SetError("Invalid UUID: " + FString{Name});
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FArchive::Serialize(std::string_view Name, FVector2& Value) {
    if (BeginObject(Name)) {
        Serialize("X", Value.x);
        Serialize("Y", Value.y);

        EndObject();
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FArchive::Serialize(std::string_view Name, FVector& Value) {
    if (BeginObject(Name)) {
        Serialize("X", Value.x);
        Serialize("Y", Value.y);
        Serialize("Z", Value.z);

        EndObject();
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FArchive::Serialize(std::string_view Name, FVector4& Value) {
    if (BeginObject(Name)) {
        Serialize("X", Value.x);
        Serialize("Y", Value.y);
        Serialize("Z", Value.z);
        Serialize("W", Value.w);

        EndObject();
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FArchive::Serialize(std::string_view Name, FQuat& Value) {
    if (BeginObject(Name)) {
        Serialize("X", Value.x);
        Serialize("Y", Value.y);
        Serialize("Z", Value.z);
        Serialize("W", Value.w);

        EndObject();
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FArchive::Serialize(std::string_view Name, FRotator& Value) {
    if (BeginObject(Name)) {
        Serialize("Pitch", Value.pitch);
        Serialize("Yaw", Value.yaw);
        Serialize("Roll", Value.roll);

        EndObject();
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
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

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
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

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FArchive::Serialize(std::string_view Name, FBox& Value) {
    if (BeginObject(Name)) {
        Serialize("Minimum", Value.mMinimum);
        Serialize("Maximum", Value.mMaximum);

        EndObject();
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FArchive::Serialize(std::string_view Name, FRay& Value) {
    if (BeginObject(Name)) {
        Serialize("Origin", Value.mOrigin);
        Serialize("Direction", Value.mDirection);

        EndObject();
    }
}
