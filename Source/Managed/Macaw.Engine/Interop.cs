using System.Numerics;
using System.Runtime.InteropServices;

namespace Macaw;

/// <summary>엔진 객체를 가리키는 핸들입니다. CoreUObject::FObjectHandle과 배치가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal readonly struct ObjectHandle : IEquatable<ObjectHandle> {
    public static readonly ObjectHandle None = new(uint.MaxValue, 0);

    public readonly uint Index;
    public readonly uint Generation;

    public ObjectHandle(uint Index, uint Generation) {
        this.Index = Index;
        this.Generation = Generation;
    }

    public bool IsNone => Index == uint.MaxValue;

    public bool Equals(ObjectHandle Other) => Index == Other.Index && Generation == Other.Generation;

    public override bool Equals(object? Other) => Other is ObjectHandle Handle && Equals(Handle);

    public override int GetHashCode() => HashCode.Combine(Index, Generation);
}

/// <summary>위치, 회전과 스케일입니다. ScriptAbi.h의 FTransform과 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
public struct Transform {
    public Vector3 Location;
    public Quaternion Rotation;
    public Vector3 Scale;
}

/// <summary>레이캐스트 결과입니다. ScriptAbi.h의 FWorldHit과 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
public readonly struct RaycastHit {
    private readonly ObjectHandle mActor;
    private readonly ObjectHandle mComponent;
    private readonly Vector3 mPosition;
    private readonly float mDistance;

    /// <summary>무언가에 맞았는지 나타냅니다.</summary>
    public bool IsHit => !mActor.IsNone;

    /// <summary>맞은 액터이며, 맞은 것이 없으면 null입니다.</summary>
    public Actor? Actor => (Actor?)Bindings.Wrap(mActor);

    /// <summary>맞은 컴포넌트이며, 맞은 것이 없으면 null입니다.</summary>
    public ActorComponent? Component => (ActorComponent?)Bindings.Wrap(mComponent);

    /// <summary>맞은 위치입니다.</summary>
    public Vector3 Position => mPosition;

    /// <summary>광선 시작점부터의 거리이며 단위는 미터입니다.</summary>
    public float Distance => mDistance;
}

/// <summary>프로퍼티 값의 종류입니다. FScriptInterop.h의 EScriptValueTag와 순서가 같아야 합니다.</summary>
internal enum ScriptValueTag {
    None,
    Bool,
    Int,
    Uint,
    Uint64,
    Float,
    String,
    Vector2,
    Vector3,
    Vector4,
    Quaternion,
    Transform,
    Object,
}

/// <summary>경계를 넘는 프로퍼티 값입니다. FScriptInterop.h의 FScriptValue와 배치가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ScriptValue {
    public ScriptValueTag Tag;
    public fixed float Floats[10];
    public ulong Integer;
    public byte* Text;
    public ObjectHandle Object;
}

/// <summary>엔진이 C#에 제공하는 함수 테이블입니다. FScriptRuntime.cpp의 FNativeApi와 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeApi {
    public nint Size;
    public delegate* unmanaged<byte*, void> Log;
    public delegate* unmanaged<ObjectHandle, int> ObjectIsAlive;
    public delegate* unmanaged<byte*, void> CatalogAddClass;
    public delegate* unmanaged<byte*, byte*, byte*, void> CatalogAddField;
    public nint* Bindings;
    public nint BindingCount;
    public nint BindingHash;
}

/// <summary>C#이 엔진에 제공하는 함수 테이블입니다. FScriptRuntime.cpp의 FManagedApi와 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedApi {
    public nint Size;
    public delegate* unmanaged<byte*, int, int> LoadScripts;
    public delegate* unmanaged<int> UnloadScripts;
    public delegate* unmanaged<byte*, ObjectHandle, nint> CreateInstance;
    public delegate* unmanaged<nint, void> BeginPlay;
    public delegate* unmanaged<nint, float, void> Tick;
    public delegate* unmanaged<nint, void> EndPlay;
    public delegate* unmanaged<nint, void> DestroyInstance;
    public delegate* unmanaged<void> DescribeScripts;
    public delegate* unmanaged<nint, byte*, byte*, byte*, int> SetField;
    public delegate* unmanaged<nint, int, ObjectHandle, void> Overlap;
    public delegate* unmanaged<byte*, ObjectHandle, nint> CreateModifier;
    public delegate* unmanaged<nint, float, MinimalViewInfo*, int> ModifyCamera;
    public delegate* unmanaged<nint, void> DestroyModifier;
}
