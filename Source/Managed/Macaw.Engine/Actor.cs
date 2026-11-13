using System.Numerics;

namespace Macaw;

/// <summary>엔진 액터를 가리키는 참조입니다. 함수 대부분은 C++ 어노테이션에서 생성됩니다.</summary>
public partial class Actor {

    /// <summary>루트 컴포넌트의 위치입니다. 1.0은 1미터입니다.</summary>
    public Vector3 Location {
        get => GetActorTransform().Location;
        set {
            Transform Transform = GetActorTransform();
            Transform.Location = value;
            ApplyTransform(Transform);
        }
    }

    /// <summary>루트 컴포넌트의 회전입니다.</summary>
    public Quaternion Rotation {
        get => GetActorTransform().Rotation;
        set {
            Transform Transform = GetActorTransform();
            Transform.Rotation = value;
            ApplyTransform(Transform);
        }
    }

    /// <summary>루트 컴포넌트의 축별 배율입니다.</summary>
    public Vector3 Scale {
        get => GetActorTransform().Scale;
        set {
            Transform Transform = GetActorTransform();
            Transform.Scale = value;
            ApplyTransform(Transform);
        }
    }

    /// <summary>T 타입의 첫 컴포넌트를 찾고, 없으면 null을 반환합니다.</summary>
    public T? GetComponent<T>() where T : ActorComponent {
        return (T?)FindComponentByClassName(Bindings.NativeName(typeof(T)));
    }

    /// <summary>T 타입의 모든 컴포넌트를 찾습니다.</summary>
    public T[] GetComponents<T>() where T : ActorComponent {
        return GetComponentsByClassName(Bindings.NativeName(typeof(T))).Cast<T>().ToArray();
    }

    /// <summary>이 액터에 붙은 T 타입의 첫 스크립트를 찾고, 없으면 null을 반환합니다.</summary>
    public T? GetScript<T>() where T : ScriptComponent {
        return Host.GetScripts(Handle).OfType<T>().FirstOrDefault();
    }

    /// <summary>변환을 액터에 적용하며 루트가 없거나 값이 거부되면 예외를 발생시킵니다.</summary>
    private void ApplyTransform(Transform Transform) {
        if (!SetActorTransform(Transform)) {
            throw new InvalidOperationException($"{GetName()} has no root component or rejected the transform.");
        }
    }
}

/// <summary>엔진 월드를 가리키는 참조입니다. 함수 대부분은 C++ 어노테이션에서 생성됩니다.</summary>
public partial class World {

    /// <summary>광선과 처음 만나는 메시를 찾습니다. 결과의 IsHit으로 맞았는지 확인합니다.</summary>
    public RaycastHit Raycast(Vector3 Origin, Vector3 Direction) => Raycast(Origin, Direction, null);

    /// <summary>T 타입의 모든 액터를 찾습니다.</summary>
    public T[] GetActors<T>() where T : Actor {
        return GetActorsByClassName(Bindings.NativeName(typeof(T))).Cast<T>().ToArray();
    }
}
