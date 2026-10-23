/**
 * @file FRotator.h
 * @brief 도 단위의 Pitch, Yaw, Roll 회전을 표현합니다.
 */
#pragma once

struct FVector;

// Degree but Quat: Radians
/**
 * @brief Pitch(Y축), Yaw(Z축), Roll(X축) 회전 각도를 도 단위로 저장합니다.
 * @note 쿼터니언 변환은 FQuat::FromRotator를 사용합니다.
 */
struct FRotator {
    /**
     * @brief 회전 각도와 별도 V 배열을 포함하는 익명 저장 공간입니다.
     */
    union {
        /**
         * @brief V 배열 뒤에 Pitch, Yaw, Roll을 독립적으로 배치한 익명 구조체입니다.
         */
        struct
        {
            /**
             * @brief 이름 있는 각도 성분 앞에 별도로 배치된 배열입니다.
             * @note 현재 선언에서 Pitch, Yaw, Roll과 저장 공간을 공유하지 않으며 생성자가 이 배열을 초기화하지 않습니다.
             */
            float V[3];
            /**
             * @brief Y축 주위의 도 단위 회전 각도입니다.
             */
            float Pitch;
            /**
             * @brief Z축 주위의 도 단위 회전 각도입니다.
             */
            float Yaw;
            /**
             * @brief X축 주위의 도 단위 회전 각도입니다.
             */
            float Roll;
        };
    };

    /**
     * @brief Pitch, Yaw, Roll을 모두 0도로 초기화합니다.
     */
    FRotator();
    /**
     * @brief 세 회전 각도를 지정한 값으로 초기화합니다.
     * @param P 도 단위 Pitch입니다.
     * @param Y 도 단위 Yaw입니다.
     * @param R 도 단위 Roll입니다.
     */
    FRotator(float P, float Y, float R);
    /**
     * @brief 벡터의 X, Y, Z를 각각 Pitch, Yaw, Roll에 복사합니다.
     * @param Vector 각 성분을 도 단위 각도로 취급할 벡터입니다.
     */
    FRotator(FVector Vector);

    /**
     * @brief Other의 각도를 현재 객체에 더하고 결과의 복사본을 반환합니다.
     * @param Other 성분별로 더할 회전 각도입니다.
     * @return 변경된 현재 회전 값의 복사본입니다.
     * @note 현재 객체를 직접 변경하며 각도 정규화나 회전 합성은 수행하지 않습니다.
     */
    FRotator operator+(const FRotator& Other) {
        Pitch += Other.Pitch;
        Yaw += Other.Yaw;
        Roll += Other.Roll;

        return *this;
    }
};

/**
 * @brief 벡터의 X, Y, Z를 회전 값의 Roll, Pitch, Yaw에 각각 더합니다.
 * @param Rot 연산의 기준이 되는 회전 값입니다.
 * @param Vec X는 Roll, Y는 Pitch, Z는 Yaw에 더할 도 단위 값입니다.
 * @return 더한 결과입니다. 호출자가 전달한 Rot 원본은 변경하지 않습니다.
 */
FRotator operator+(FRotator Rot, const FVector& Vec);
