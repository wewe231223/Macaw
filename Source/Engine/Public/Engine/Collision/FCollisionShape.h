/**
 * @file FCollisionShape.h
 * @brief 구·상자·캡슐의 로컬 치수를 담는 충돌 형상 값입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/TNativeEnum.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FQuat.h"

/**
 * @brief 검사할 충돌 형상 종류입니다.
 */
enum class ECollisionShape : Uint8 {
    /** @brief 충돌 형상을 사용하지 않습니다. */
    None,
    /** @brief 축별 반길이를 갖는 상자 형상입니다. */
    Box,
    /** @brief 반지름을 갖는 구 형상입니다. */
    Sphere,
    /** @brief 선분 양 끝의 반구를 포함하는 캡슐 형상입니다. */
    Capsule
};

/**
 * @brief 해당 열거형의 정적 리플렉션 메타데이터를 반환합니다.
 * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
 */
ReflectEnum(ECollisionShape, None, Box, Sphere, Capsule);

/** @fn UEnum* GetNativeEnum(TNativeEnum<ECollisionShape>)
 * @brief 타입 태그로 충돌 형상 종류의 열거형 메타데이터를 조회합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */

/** @brief 구·상자·캡슐의 로컬 치수를 담는 충돌 형상 값입니다. */
struct FCollisionShape {
public:
    ReflectTypes(FCollisionShape);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FCollisionShape);
    /**
     * @brief 축별 반길이로 상자 충돌 형상을 만듭니다.
     * @param[in] HalfExtent 상자의 로컬 축별 반길이입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    static FCollisionShape MakeBox(const FVector& HalfExtent);
    /**
     * @brief 반지름으로 구 충돌 형상을 만듭니다.
     * @param[in] Radius 형상 반지름입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    static FCollisionShape MakeSphere(float Radius);
    /**
     * @brief 반지름과 전체 반높이로 캡슐 충돌 형상을 만듭니다.
     * @param[in] Radius 형상 반지름입니다.
     * @param[in] HalfHeight 반구를 포함한 캡슐 반높이입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    static FCollisionShape MakeCapsule(float Radius, float HalfHeight);
    /**
     * @brief 상자 충돌 형상인지 검사합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool IsBox() const;
    /**
     * @brief 구 충돌 형상인지 검사합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool IsSphere() const;
    /**
     * @brief 캡슐 충돌 형상인지 검사합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool IsCapsule() const;
    /**
     * @brief 반구를 제외한 캡슐 중심 선분 반길이를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetCapsuleAxisHalfLength() const;
    /**
     * @brief 상자의 로컬 축별 반길이를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector& GetBoxHalfExtent() const;
    /**
     * @brief 충돌 형상의 반지름을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetRadius() const;
    /**
     * @brief 반구를 포함한 캡슐 반높이를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetHalfHeight() const;

private:
    ReflectMembers();

private:
    /**
     * @brief 충돌 형상의 종류입니다.
     */
    ECollisionShape mShapeType{ECollisionShape::None};
    ReflectProperty(mShapeType, "ShapeType", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 상자의 로컬 축별 반길이입니다.
     */
    FVector mBoxHalfExtent{};
    ReflectProperty(mBoxHalfExtent, "BoxHalfExtent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 충돌 형상의 반지름입니다.
     */
    float mRadius{};
    ReflectProperty(mRadius, "Radius", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 반구를 포함한 캡슐 반높이입니다.
     */
    float mHalfHeight{};
    ReflectProperty(mHalfHeight, "HalfHeight", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/** @brief 겹침 검사에서 구한 월드 접촉 법선·위치·침투 깊이입니다. */
struct FCollisionContact {
public:
    ReflectTypes(FCollisionContact);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FCollisionContact);
    /**
     * @brief 충돌면의 단위 법선을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector& GetNormal() const;
    /**
     * @brief 충돌면의 단위 법선을 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetNormal(const FVector& Value);
    /**
     * @brief 초기 겹침을 해소할 침투 깊이를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetPenetrationDepth() const;
    /**
     * @brief 초기 겹침을 해소할 침투 깊이를 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetPenetrationDepth(float Value);
    /**
     * @brief 월드 접촉점을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector& GetPoint() const;
    /**
     * @brief 월드 접촉점을 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetPoint(const FVector& Value);

private:
    ReflectMembers();

private:
    /**
     * @brief 충돌면의 단위 법선입니다.
     */
    FVector mNormal{};
    ReflectProperty(mNormal, "Normal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 초기 겹침을 해소할 침투 깊이입니다.
     */
    float mPenetrationDepth{};
    ReflectProperty(mPenetrationDepth, "PenetrationDepth", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 월드 접촉점입니다.
     */
    FVector mPoint{};
    ReflectProperty(mPoint, "Point", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/** @brief 월드 캡슐의 중심 선분 두 끝점과 반지름입니다. */
struct FCapsuleSegment {
public:
    ReflectTypes(FCapsuleSegment);

public:
    /**
     * @brief 전달된 설정과 기본값으로 FCapsuleSegment 객체를 생성합니다.
     */
    FCapsuleSegment() = default;
    /**
     * @brief 전달된 설정과 기본값으로 FCapsuleSegment 객체를 생성합니다.
     * @param[in] InA 캡슐 중심 선분의 시작점입니다.
     * @param[in] InB 캡슐 중심 선분의 끝점입니다.
     * @param[in] InRadius 형상의 반지름입니다.
     */
    FCapsuleSegment(FVector InA, FVector InB, float InRadius);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FCapsuleSegment);
    /**
     * @brief 캡슐 중심 선분의 시작점을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector& GetA() const;
    /**
     * @brief 캡슐 중심 선분의 시작점을 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetA(const FVector& Value);
    /**
     * @brief 캡슐 중심 선분의 끝점을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector& GetB() const;
    /**
     * @brief 캡슐 중심 선분의 끝점을 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetB(const FVector& Value);
    /**
     * @brief 충돌 형상의 반지름을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetRadius() const;
    /**
     * @brief 충돌 형상의 반지름을 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetRadius(float Value);

private:
    ReflectMembers();

private:
    /**
     * @brief 캡슐 중심 선분의 시작점입니다.
     */
    FVector mA{};
    ReflectProperty(mA, "A", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 캡슐 중심 선분의 끝점입니다.
     */
    FVector mB{};
    ReflectProperty(mB, "B", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 충돌 형상의 반지름입니다.
     */
    float mRadius{};
    ReflectProperty(mRadius, "Radius", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/** @brief 월드 중심·직교 기저·반길이로 표현한 회전 상자입니다. */
struct FOrientedBox {
public:
    ReflectTypes(FOrientedBox);

public:
    /**
     * @brief 전달된 설정과 기본값으로 FOrientedBox 객체를 생성합니다.
     */
    FOrientedBox() = default;
    /**
     * @brief 전달된 설정과 기본값으로 FOrientedBox 객체를 생성합니다.
     * @param[in] InCenter 상자의 월드 중심입니다.
     * @param[in] InAxis 회전 상자의 세 기저 축입니다.
     * @param[in] InExtent 상자 각 축의 반길이입니다.
     */
    FOrientedBox(FVector InCenter, std::array<FVector, 3> InAxis, FVector InExtent);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FOrientedBox);
    /**
     * @brief 월드 중심 위치를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector& GetCenter() const;
    /**
     * @brief 월드 중심 위치를 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetCenter(const FVector& Value);
    /**
     * @brief 회전 상자의 세 기저 축을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const std::array<FVector, 3>& GetAxis() const;
    /**
     * @brief 회전 상자의 세 기저 축을 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetAxis(const std::array<FVector, 3>& Value);
    /**
     * @brief 각 축의 반길이를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector& GetExtent() const;
    /**
     * @brief 각 축의 반길이를 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetExtent(const FVector& Value);

private:
    ReflectMembers();

private:
    /**
     * @brief 월드 중심 위치입니다.
     */
    FVector mCenter{};
    ReflectProperty(mCenter, "Center", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 회전 상자의 세 기저 축입니다.
     */
    std::array<FVector, 3> mAxis{};
    ReflectProperty(mAxis, "Axis", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 각 축의 반길이입니다.
     */
    FVector mExtent{};
    ReflectProperty(mExtent, "Extent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/** @brief 스윕 대상의 충돌 형상과 월드 위치·회전입니다. */
struct FSweepTarget {
public:
    ReflectTypes(FSweepTarget);

public:
    /**
     * @brief 전달된 설정과 기본값으로 FSweepTarget 객체를 생성합니다.
     */
    FSweepTarget() = default;
    /**
     * @brief 전달된 설정과 기본값으로 FSweepTarget 객체를 생성합니다.
     * @param[in] InShape 대상의 충돌 형상입니다.
     * @param[in] InPos 대상의 월드 위치입니다.
     * @param[in] InRot 대상의 월드 회전입니다.
     */
    FSweepTarget(FCollisionShape InShape, FVector InPos, FQuat InRot);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FSweepTarget);
    /**
     * @brief 충돌 형상을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FCollisionShape& GetShape() const;
    /**
     * @brief 충돌 형상을 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetShape(const FCollisionShape& Value);
    /**
     * @brief 월드 위치를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector& GetPos() const;
    /**
     * @brief 월드 위치를 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetPos(const FVector& Value);
    /**
     * @brief 월드 회전을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FQuat& GetRot() const;
    /**
     * @brief 월드 회전을 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetRot(const FQuat& Value);

private:
    ReflectMembers();

private:
    /**
     * @brief 충돌 형상입니다.
     */
    FCollisionShape mShape{};
    ReflectProperty(mShape, "Shape", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 월드 위치입니다.
     */
    FVector mPos{};
    ReflectProperty(mPos, "Pos", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 월드 회전입니다.
     */
    FQuat mRot{};
    ReflectProperty(mRot, "Rot", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/** @brief 최초 스윕 충돌 여부·시간 비율·대상·접촉 정보입니다. */
struct FSweepResult {
public:
    ReflectTypes(FSweepResult);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FSweepResult);
    /**
     * @brief 이동을 차단하는 충돌 여부를 반환합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool GetBlockingHit() const;
    /**
     * @brief 이동을 차단하는 충돌 여부를 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetBlockingHit(bool Value);
    /**
     * @brief 추적 시작부터 겹친 상태를 반환합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool GetStartPenetrating() const;
    /**
     * @brief 추적 시작부터 겹친 상태를 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetStartPenetrating(bool Value);
    /**
     * @brief 전체 추적 거리에서 적중한 비율을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetTime() const;
    /**
     * @brief 전체 추적 거리에서 적중한 비율을 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetTime(float Value);
    /**
     * @brief 적중한 스윕 대상의 인덱스를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    Int32 GetTargetIndex() const;
    /**
     * @brief 적중한 스윕 대상의 인덱스를 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetTargetIndex(Int32 Value);
    /**
     * @brief 충돌 접촉 정보를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FCollisionContact& GetContact() const;
    /**
     * @brief 충돌 접촉 정보를 설정합니다.
     * @param[in] Value 설정하거나 읽을 값입니다.
     */
    void SetContact(const FCollisionContact& Value);

private:
    ReflectMembers();

private:
    /**
     * @brief 이동을 차단하는 충돌 여부입니다.
     */
    bool mBlockingHit{};
    ReflectProperty(mBlockingHit, "BlockingHit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 추적 시작부터 겹친 상태입니다.
     */
    bool mStartPenetrating{};
    ReflectProperty(mStartPenetrating, "StartPenetrating", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 전체 추적 거리에서 적중한 비율입니다.
     */
    float mTime{1.0f};
    ReflectProperty(mTime, "Time", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 적중한 스윕 대상의 인덱스입니다.
     */
    Int32 mTargetIndex{-1};
    ReflectProperty(mTargetIndex, "TargetIndex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 충돌 접촉 정보입니다.
     */
    FCollisionContact mContact{};
    ReflectProperty(mContact, "Contact", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/** @brief 구·상자·캡슐의 겹침과 스윕 질의를 제공합니다. */
namespace Collision {
    /**
     * @brief 충돌 캡슐을 월드 중심 선분과 반지름으로 변환합니다.
     * @param[in] Shape 검사할 로컬 충돌 형상입니다.
     * @param[in] Pos 형상의 월드 위치입니다.
     * @param[in] Rot 형상의 월드 회전입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FCapsuleSegment ToCapsuleSegment(const FCollisionShape& Shape, const FVector& Pos, const FQuat& Rot);
    /**
     * @brief 로컬 상자를 월드 중심·기저·반길이로 변환합니다.
     * @param[in] Shape 검사할 로컬 충돌 형상입니다.
     * @param[in] Pos 형상의 월드 위치입니다.
     * @param[in] Rot 형상의 월드 회전입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FOrientedBox ToOrientedBox(const FCollisionShape& Shape, const FVector& Pos, const FQuat& Rot);

    /**
     * @brief 형상 조합별 좁은 단계 겹침 검사를 수행합니다.
     * @param[in] A 첫 번째 검사 대상입니다.
     * @param[in] PosA 첫 형상의 월드 위치입니다.
     * @param[in] RotA 첫 형상의 월드 회전입니다.
     * @param[in] B 두 번째 검사 대상입니다.
     * @param[in] PosB 둘째 형상의 월드 위치입니다.
     * @param[in] RotB 둘째 형상의 월드 회전입니다.
     * @param[out] OutContact 접촉 정보를 기록할 선택적 출력 주소입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool Overlap(const FCollisionShape& A, const FVector& PosA, const FQuat& RotA, const FCollisionShape& B, const FVector& PosB, const FQuat& RotB, FCollisionContact* OutContact = nullptr);

    /**
     * @brief 두 캡슐 중심 선분의 거리로 겹침과 접촉을 계산합니다.
     * @param[in] A 첫 번째 검사 대상입니다.
     * @param[in] B 두 번째 검사 대상입니다.
     * @param[out] OutContact 접촉 정보를 기록할 선택적 출력 주소입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool OverlapCapsuleCapsule(const FCapsuleSegment& A, const FCapsuleSegment& B, FCollisionContact* OutContact);
    /**
     * @brief 캡슐 선분과 회전 상자의 거리로 겹침을 계산합니다.
     * @param[in] Capsule 검사할 월드 캡슐입니다.
     * @param[in] Box 검사할 회전 상자입니다.
     * @param[out] OutContact 접촉 정보를 기록할 선택적 출력 주소입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool OverlapCapsuleBox(const FCapsuleSegment& Capsule, const FOrientedBox& Box, FCollisionContact* OutContact);
    /**
     * @brief 분리 축 검사로 두 회전 상자의 겹침을 계산합니다.
     * @param[in] A 첫 번째 검사 대상입니다.
     * @param[in] B 두 번째 검사 대상입니다.
     * @param[out] OutContact 접촉 정보를 기록할 선택적 출력 주소입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool OverlapBoxBox(const FOrientedBox& A, const FOrientedBox& B, FCollisionContact* OutContact);

    /**
     * @brief 점에서 선분까지의 최근접 위치를 계산합니다.
     * @param[in] P 최근접 점을 구할 원본 위치입니다.
     * @param[in] A 첫 번째 검사 대상입니다.
     * @param[in] B 두 번째 검사 대상입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FVector ClosestPointOnSegment(const FVector& P, const FVector& A, const FVector& B);
    /**
     * @brief 두 선분의 최근접 점과 그 사이의 제곱 거리를 계산합니다.
     * @param[in] P1 첫 선분의 시작점입니다.
     * @param[in] Q1 첫 선분의 끝점입니다.
     * @param[in] P2 둘째 선분의 시작점입니다.
     * @param[in] Q2 둘째 선분의 끝점입니다.
     * @param[out] OutC1 첫 선분의 최근접 점을 기록합니다.
     * @param[out] OutC2 둘째 선분의 최근접 점을 기록합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float ClosestPointsSegmentSegment(const FVector& P1, const FVector& Q1, const FVector& P2, const FVector& Q2, FVector& OutC1, FVector& OutC2);
    /**
     * @brief 점을 회전 상자의 각 축 범위로 제한한 최근접 위치를 계산합니다.
     * @param[in] P 최근접 점을 구할 원본 위치입니다.
     * @param[in] Box 검사할 회전 상자입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FVector ClosestPointOnBox(const FVector& P, const FOrientedBox& Box);

    /**
     * @brief 형상을 이동시켜 대상 목록 중 최초 차단 접촉을 찾습니다.
     * @param[in] Shape 검사할 로컬 충돌 형상입니다.
     * @param[in] Start 추적 또는 선분의 시작점입니다.
     * @param[in] Delta 형상을 이동할 월드 변위입니다.
     * @param[in] Rot 형상의 월드 회전입니다.
     * @param[in] Targets 검사할 스윕 대상 목록입니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FSweepResult Sweep(const FCollisionShape& Shape, const FVector& Start, const FVector& Delta, const FQuat& Rot, const TArray<FSweepTarget>& Targets);
}
