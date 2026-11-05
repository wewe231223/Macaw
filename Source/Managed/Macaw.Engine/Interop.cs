using System.Numerics;
using System.Runtime.InteropServices;

namespace Macaw;

/// <summary>엔진 객체를 가리키는 핸들입니다. CoreUObject::FObjectHandle과 배치가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal readonly struct ObjectHandle {
    public readonly uint Index;
    public readonly uint Generation;
}

/// <summary>경계를 넘는 Transform입니다. FScriptRuntime.cpp의 FAbiTransform과 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal struct Transform {
    public Vector3 Location;
    public Quaternion Rotation;
    public Vector3 Scale;
}

/// <summary>엔진이 C#에 제공하는 함수 테이블입니다. FScriptRuntime.cpp의 FNativeApi와 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeApi {
    public nint Size;
    public delegate* unmanaged<byte*, void> Log;
    public delegate* unmanaged<ObjectHandle, Transform*, int> ActorGetTransform;
    public delegate* unmanaged<ObjectHandle, Transform*, int> ActorSetTransform;
}

/// <summary>C#이 엔진에 제공하는 함수 테이블입니다. FScriptRuntime.cpp의 FManagedApi와 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedApi {
    public nint Size;
    public delegate* unmanaged<byte*, int> LoadScripts;
    public delegate* unmanaged<int> UnloadScripts;
    public delegate* unmanaged<byte*, ObjectHandle, nint> CreateInstance;
    public delegate* unmanaged<nint, void> BeginPlay;
    public delegate* unmanaged<nint, float, void> Tick;
    public delegate* unmanaged<nint, void> EndPlay;
    public delegate* unmanaged<nint, void> DestroyInstance;
}
