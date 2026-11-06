using System.Text;

namespace Macaw;

/// <summary>생성된 바인딩이 함께 쓰는 보조 함수입니다. 함수 목록은 ScriptBindings.g.cs에 생성됩니다.</summary>
internal static unsafe partial class Bindings {
    /// <summary>엔진의 썽크 주소 테이블입니다. 순서는 생성기가 양쪽에 같게 만듭니다.</summary>
    public static nint* sTable;

    public static byte[] Utf8(string Text) => Encoding.UTF8.GetBytes(Text + "\0");

    public static ObjectHandle HandleOf(EngineObject? Object) => Object?.Handle ?? ObjectHandle.None;

    public static InvalidOperationException Gone(string Function) => new($"{Function}: the object no longer exists.");

    /// <summary>핸들을 실제 클래스에 가장 가까운 래퍼로 감쌉니다. 비어 있는 핸들은 null이 됩니다.</summary>
    public static EngineObject? Wrap(ObjectHandle Handle) {
        if (Handle.IsNone) {
            return null;
        }

        string Chain = Interop.GetClassChain(new EngineObject.Untyped(Handle));

        foreach (string Name in Chain.Split(';', StringSplitOptions.RemoveEmptyEntries)) {
            if (sFactories.TryGetValue(Name, out Func<ObjectHandle, EngineObject>? Factory)) {
                return Factory(Handle);
            }
        }

        return new EngineObject.Untyped(Handle);
    }

    public static T[] WrapArray<T>(ObjectHandle* Handles, int Count) where T : EngineObject {
        // Wrap이 엔진을 다시 호출해 보관 버퍼를 덮어쓰므로 핸들을 먼저 복사합니다.
        var Copy = new ObjectHandle[Count];
        new ReadOnlySpan<ObjectHandle>(Handles, Count).CopyTo(Copy);

        return Copy.Select(Handle => (T)Wrap(Handle)!).ToArray();
    }

    /// <summary>C# 래퍼 타입에 대응하는 엔진 클래스 이름을 반환합니다.</summary>
    public static string NativeName(Type Type) {
        return sNativeNames.TryGetValue(Type, out string? Name) ? Name : throw new ArgumentException($"{Type.Name} is not an engine class.");
    }
}
