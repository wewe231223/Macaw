/**
 * @file FTransform.h
 * @brief 위치, 쿼터니언 회전과 축별 스케일을 묶은 공간 변환입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FQuat.h"

namespace Core {

    /**
     * @brief 위치, 쿼터니언 회전과 축별 스케일을 묶은 공간 변환입니다.
     * @ingroup Core
     */
    struct alignas(16) FTransform {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FTransform);

    public:
        /**
         * @brief FTransform의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        FTransform() = default;

        /**
         * @brief FTransform의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Location 오류가 발생한 원래 호출 위치입니다.
         * @param[in] Rotation 적용할 오일러 또는 쿼터니언 회전입니다.
         * @param[in] Scale Scale에 적용할 입력 값입니다.
         */
        FTransform(const Core::FVector& Location, const Core::FQuat& Rotation, const Core::FVector& Scale);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FTransform() = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
         */
        FTransform(const Core::FTransform& Other) = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        Core::FTransform& operator=(const Core::FTransform& Other) = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
         */
        FTransform(Core::FTransform&& Other) noexcept = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        Core::FTransform& operator=(Core::FTransform&& Other) noexcept = default;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FTransform);

        /**
         * @brief 현재 변환 뒤에 상대 변환을 적용하도록 위치, 회전과 축별 스케일을 합성합니다.
         * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FTransform operator*(const Core::FTransform& Other) const;

        /**
         * @brief 현재 변환 뒤에 상대 변환을 적용하도록 위치, 회전과 축별 스케일을 합성합니다.
         * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        Core::FTransform& operator*=(const Core::FTransform& Other);

        /**
         * @brief 위치에 스케일, 회전과 이동을 적용합니다.
         * @param[in] Position 변환하거나 투영할 위치입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FVector TransformPosition(const Core::FVector& Position) const;

        /**
         * @brief 방향 벡터에 스케일과 회전을 적용합니다.
         * @param[in] Vector 연산에 사용할 벡터입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FVector TransformVector(const Core::FVector& Vector) const;

        /**
         * @brief 위치 또는 벡터에 역변환을 적용합니다.
         * @param[in] Position 변환하거나 투영할 위치입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FVector InverseTransformPosition(const Core::FVector& Position) const;

        /**
         * @brief 위치 또는 벡터에 역변환을 적용합니다.
         * @param[in] Vector 연산에 사용할 벡터입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FVector InverseTransformVector(const Core::FVector& Vector) const;

        /**
         * @brief 위치에 스케일을 제외한 회전과 이동을 적용합니다.
         * @param[in] Position 변환하거나 투영할 위치입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FVector TransformPositionNoScale(const Core::FVector& Position) const;

        /**
         * @brief 벡터에 스케일을 제외한 회전만 적용합니다.
         * @param[in] Vector 연산에 사용할 벡터입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FVector TransformVectorNoScale(const Core::FVector& Vector) const;

        /**
         * @brief 위치에 이동과 회전의 역변환을 적용합니다.
         * @param[in] Position 변환하거나 투영할 위치입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FVector InverseTransformPositionNoScale(const Core::FVector& Position) const;

        /**
         * @brief 벡터에 역회전을 적용합니다.
         * @param[in] Vector 연산에 사용할 벡터입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FVector InverseTransformVectorNoScale(const Core::FVector& Vector) const;

        /**
         * @brief 위치·스케일 선형 보간과 회전 보간을 결합한 변환을 구합니다.
         * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
         * @param[in] Alpha 보간 비율입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FTransform Blend(const Core::FTransform& Other, float Alpha) const;

        /**
         * @brief 변환의 쿼터니언 회전을 제자리에서 정규화합니다.
         * @return 정규화할 수 있으면 true이며, 퇴화하거나 유효하지 않은 값이면 false입니다.
         */
        bool NormalizeRotation();

        /**
         * @brief 현재 변환이 허용 오차 이내에서 항등 변환인지 검사합니다.
         * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IsIdentity(float Tolerance = 0.0001f) const;

        /**
         * @brief 각 성분의 차이가 지정한 허용 오차 이내인지 검사합니다.
         * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
         * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool Equals(const Core::FTransform& Other, float Tolerance = 0.0001f) const;

        /**
         * @brief 현재 쿼터니언 회전을 도 단위 오일러 각으로 변환한 값을 반환합니다.
         * @return 현재 쿼터니언 회전을 도 단위 오일러 각으로 변환한 값입니다.
         */
        Core::FRotator GetRotator() const;

        /**
         * @brief 유효한 도 단위 오일러 회전을 쿼터니언으로 변환하여 적용합니다.
         * @param[in] Rotation 적용할 오일러 또는 쿼터니언 회전입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool SetRotator(const Core::FRotator& Rotation);

        /**
         * @brief 위치, 회전과 스케일을 결합한 변환 행렬을 반환합니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FMatrix ToMatrix() const;

        /**
         * @brief 성분이 유한하고 회전과 스케일이 사용 가능한 공간 변환인지 검사합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IsValid() const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 공간상의 위치입니다.
         */
        Core::FVector mLocation{};
        ReflectProperty(mLocation, "Location", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);


        /**
         * @brief 회전 값입니다.
         */
        Core::FQuat mRotation{};
        ReflectAccessor(mRotation, GetRotator, SetRotator, "Rotation", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);


        /**
         * @brief 축별 스케일입니다.
         */
        Core::FVector mScale{1.0f, 1.0f, 1.0f};
        ReflectProperty(mScale, "Scale", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);
    };

}
