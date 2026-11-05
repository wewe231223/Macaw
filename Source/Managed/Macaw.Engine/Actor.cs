using System.Numerics;

namespace Macaw;

/// <summary>엔진 액터를 가리키는 참조입니다. 포인터 대신 핸들만 보관하므로 액터가 파괴되면 접근할 때 예외가 발생합니다.</summary>
public sealed unsafe class Actor {
    internal static NativeApi* sApi;

    private readonly ObjectHandle mHandle;

    internal Actor(ObjectHandle Handle) {
        mHandle = Handle;
    }

    /// <summary>루트 컴포넌트의 위치입니다. 1.0은 1미터입니다.</summary>
    public Vector3 Location {
        get => GetTransform().Location;
        set {
            Transform Transform = GetTransform();
            Transform.Location = value;
            SetTransform(Transform);
        }
    }

    /// <summary>루트 컴포넌트의 회전입니다.</summary>
    public Quaternion Rotation {
        get => GetTransform().Rotation;
        set {
            Transform Transform = GetTransform();
            Transform.Rotation = value;
            SetTransform(Transform);
        }
    }

    /// <summary>루트 컴포넌트의 축별 배율입니다.</summary>
    public Vector3 Scale {
        get => GetTransform().Scale;
        set {
            Transform Transform = GetTransform();
            Transform.Scale = value;
            SetTransform(Transform);
        }
    }

    private Transform GetTransform() {
        Transform Transform;

        if (sApi->ActorGetTransform(mHandle, &Transform) == 0) {
            throw new InvalidOperationException("The actor no longer exists or has no root component.");
        }

        return Transform;
    }

    private void SetTransform(Transform Transform) {
        if (sApi->ActorSetTransform(mHandle, &Transform) == 0) {
            throw new InvalidOperationException("The actor no longer exists or rejected the transform.");
        }
    }
}
