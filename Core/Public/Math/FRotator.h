#pragma once

struct FVector;

// Degree but Quat: Radians
struct FRotator {
    union {
        struct
        {
            float V[3];
            float Pitch;
            float Yaw;
            float Roll;
        };
    };

    FRotator();
    FRotator(float P, float Y, float R);
    FRotator(FVector Vector);

    FRotator operator+(const FRotator& Other) {
        Pitch += Other.Pitch;
        Yaw += Other.Yaw;
        Roll += Other.Roll;

        return *this;
    }
};

FRotator operator+(FRotator Rot, const FVector& Vec);
