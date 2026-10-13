#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/USceneComponent.h"
#include "Core/Archive/FArchive.h"

class UCameraComponent : public USceneComponent {
public:
    ReflectTypes(UCameraComponent, USceneComponent);

public:
    inline static const FMatrix CameraBasis{
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f};

public:
    UCameraComponent();
    ~UCameraComponent() override = default;

public:
    ReflectClass(UCameraComponent, EClassFlags::None);

    FMatrix GetViewMatrix() const;
    FMatrix GetProjectionMatrix() const;
    FMatrix GetViewProjectionMatrix() const;
    ReflectFunction(GetFOV);
    float GetFOV() const;
    ReflectFunction(GetAspectRatio);
    float GetAspectRatio() const;
    ReflectFunction(GetNearPlane);
    float GetNearPlane() const;
    ReflectFunction(GetFarPlane);
    float GetFarPlane() const;
    ReflectFunction(SetFOV, FName{"InFOV"});
    void SetFOV(float InFOV);
    ReflectFunction(SetAspectRatio, FName{"InAspectRatio"});
    void SetAspectRatio(float InAspectRatio);
    ReflectFunction(SetNearPlane, FName{"InNearPlane"});
    void SetNearPlane(float InNearPlane);
    ReflectFunction(SetFarPlane, FName{"InFarPlane"});
    void SetFarPlane(float InFarPlane);

    void OnRegister() override;
    void OnUnregister() override;

    void SetMoveSensitivity(float InMoveSensitivity);
    float GetMoveSensitivity() const;

    void SetRotationSensitivity(float InRotationSensitivity);
    float GetRotationSensitivity() const;

protected:
    void Serialize(FArchive& Archive) override;

private:
    ReflectAccessor(mFov, GetFOV, SetFOV, "FOV", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mFov), ReflectEditor("Camera", "FOV"), ReflectRange(0.01f, 3.13f));
    float mFov{1.0472f}; // 약 60도, 라디안
    ReflectAccessor(mAspectRatio, GetAspectRatio, SetAspectRatio, "AspectRatio", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mAspectRatio), ReflectEditor("Camera", "Aspect Ratio"), ReflectRange(0.01f, std::numeric_limits<float>::max()));
    float mAspectRatio{16.0f / 9.0f};
    ReflectAccessor(mNearPlane, GetNearPlane, SetNearPlane, "NearPlane", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mNearPlane), ReflectEditor("Camera", "Near Plane"), ReflectRange(0.001f, std::numeric_limits<float>::max()));
    float mNearPlane{0.1f};
    ReflectAccessor(mFarPlane, GetFarPlane, SetFarPlane, "FarPlane", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mFarPlane), ReflectEditor("Camera", "Far Plane"), ReflectRange(0.001f, std::numeric_limits<float>::max()));
    float mFarPlane{1000.0f};
    float mMoveSensitivity{5.0f};
    float mRotationSensitivity{0.1f};

    ReflectMembers();
};
