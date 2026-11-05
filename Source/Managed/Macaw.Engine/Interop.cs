using System.Runtime.InteropServices;

namespace Macaw;

/// <summary>엔진이 C#에 제공하는 함수 테이블입니다. FScriptRuntime.cpp의 FNativeApi와 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeApi {
    public nint Size;
    public delegate* unmanaged<byte*, void> Log;
}

/// <summary>C#이 엔진에 제공하는 함수 테이블입니다. FScriptRuntime.cpp의 FManagedApi와 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedApi {
    public nint Size;
    public delegate* unmanaged<byte*, int> LoadScripts;
    public delegate* unmanaged<int> UnloadScripts;
    public delegate* unmanaged<byte*, nint> CreateInstance;
    public delegate* unmanaged<nint, float, void> Tick;
    public delegate* unmanaged<nint, void> DestroyInstance;
}
