using System.Numerics;
using System.Runtime.InteropServices;

namespace Macaw;

/// <summary>엔진 객체를 가리키는 참조의 기반 클래스입니다. 포인터 대신 핸들만 보관하므로 객체가 파괴되면 접근할 때 예외가 발생합니다.</summary>
public abstract unsafe partial class EngineObject : IEquatable<EngineObject> {

    /// <summary>네이티브가 소유하는 엔진 API 함수 테이블의 비소유 포인터입니다.</summary>
    internal static NativeApi* sApi;

    /// <summary>래퍼가 참조하는 엔진 객체의 인덱스·세대 핸들입니다.</summary>
    internal readonly ObjectHandle Handle;

    /// <summary>엔진 객체의 수명 소유권 없이 전달받은 핸들을 보관합니다.</summary>
    internal EngineObject(ObjectHandle Handle) {
        this.Handle = Handle;
    }

    /// <summary>객체가 아직 살아 있는지 나타냅니다. 파괴가 예약된 액터는 false입니다.</summary>
    public bool IsValid => sApi->ObjectIsAlive(Handle) != 0;

    /// <summary>Details 패널에 보이는 이름으로 리플렉션 프로퍼티를 읽습니다.</summary>
    /// <typeparam name="T">bool, int, uint, ulong, float, string, Vector2, Vector3, Vector4, Quaternion, Transform, 열거형, 엔진 객체를 쓸 수 있습니다.</typeparam>
    public T GetProperty<T>(string Name) {
        ScriptValue Value = Interop.GetProperty(this, Name);
        object? Result = Value.Tag switch {
            ScriptValueTag.None => throw new ArgumentException($"{GetName()} has no readable property named {Name}."),
            ScriptValueTag.Bool => Value.Integer != 0,
            ScriptValueTag.Int => (int)(uint)Value.Integer,
            ScriptValueTag.Uint => (uint)Value.Integer,
            ScriptValueTag.Uint64 => Value.Integer,
            ScriptValueTag.Float => Value.Floats[0],
            ScriptValueTag.String => Marshal.PtrToStringUTF8((nint)Value.Text),
            ScriptValueTag.Vector2 => new Vector2(Value.Floats[0], Value.Floats[1]),
            ScriptValueTag.Vector3 => new Vector3(Value.Floats[0], Value.Floats[1], Value.Floats[2]),
            ScriptValueTag.Vector4 => new Vector4(Value.Floats[0], Value.Floats[1], Value.Floats[2], Value.Floats[3]),
            ScriptValueTag.Quaternion => new Quaternion(Value.Floats[0], Value.Floats[1], Value.Floats[2], Value.Floats[3]),
            ScriptValueTag.Transform => new Transform {
                Location = new Vector3(Value.Floats[0], Value.Floats[1], Value.Floats[2]),
                Rotation = new Quaternion(Value.Floats[3], Value.Floats[4], Value.Floats[5], Value.Floats[6]),
                Scale = new Vector3(Value.Floats[7], Value.Floats[8], Value.Floats[9]),
            },
            _ => Bindings.Wrap(Value.Object),
        };

        if (typeof(T).IsEnum && Result is int Integer) {
            return (T)Enum.ToObject(typeof(T), Integer);
        }

        return Result is T || (Result == null && !typeof(T).IsValueType) ? (T)Result! : throw new InvalidCastException($"Property {Name} holds {Value.Tag}, not {typeof(T).Name}.");
    }

    /// <summary>Details 패널에 보이는 이름으로 리플렉션 프로퍼티를 씁니다. 타입이 다르거나 읽기 전용이면 예외가 발생합니다.</summary>
    public void SetProperty<T>(string Name, T NewValue) {
        ScriptValue Value = default;
        byte[]? Text = null;
        object? Boxed = typeof(T).IsEnum ? Convert.ToInt32(NewValue) : NewValue;

        switch (Boxed) {
            case bool Boolean:
                Value.Tag = ScriptValueTag.Bool;
                Value.Integer = Boolean ? 1UL : 0UL;
                break;
            case int Integer:
                Value.Tag = ScriptValueTag.Int;
                Value.Integer = (uint)Integer;
                break;
            case uint Unsigned:
                Value.Tag = ScriptValueTag.Uint;
                Value.Integer = Unsigned;
                break;
            case ulong Unsigned64:
                Value.Tag = ScriptValueTag.Uint64;
                Value.Integer = Unsigned64;
                break;
            case float Single:
                Value.Tag = ScriptValueTag.Float;
                Value.Floats[0] = Single;
                break;
            case string String:
                Value.Tag = ScriptValueTag.String;
                Text = Bindings.Utf8(String);
                break;
            case Vector2 Vector:
                Value.Tag = ScriptValueTag.Vector2;
                Value.Floats[0] = Vector.X;
                Value.Floats[1] = Vector.Y;
                break;
            case Vector3 Vector:
                Value.Tag = ScriptValueTag.Vector3;
                Value.Floats[0] = Vector.X;
                Value.Floats[1] = Vector.Y;
                Value.Floats[2] = Vector.Z;
                break;
            case Vector4 Vector:
                Value.Tag = ScriptValueTag.Vector4;
                Value.Floats[0] = Vector.X;
                Value.Floats[1] = Vector.Y;
                Value.Floats[2] = Vector.Z;
                Value.Floats[3] = Vector.W;
                break;
            case Quaternion Rotation:
                Value.Tag = ScriptValueTag.Quaternion;
                Value.Floats[0] = Rotation.X;
                Value.Floats[1] = Rotation.Y;
                Value.Floats[2] = Rotation.Z;
                Value.Floats[3] = Rotation.W;
                break;
            case Transform Transform:
                Value.Tag = ScriptValueTag.Transform;
                Value.Floats[0] = Transform.Location.X;
                Value.Floats[1] = Transform.Location.Y;
                Value.Floats[2] = Transform.Location.Z;
                Value.Floats[3] = Transform.Rotation.X;
                Value.Floats[4] = Transform.Rotation.Y;
                Value.Floats[5] = Transform.Rotation.Z;
                Value.Floats[6] = Transform.Rotation.W;
                Value.Floats[7] = Transform.Scale.X;
                Value.Floats[8] = Transform.Scale.Y;
                Value.Floats[9] = Transform.Scale.Z;
                break;
            case EngineObject Object:
                Value.Tag = ScriptValueTag.Object;
                Value.Object = Object.Handle;
                break;
            case null when !typeof(T).IsValueType:
                Value.Tag = ScriptValueTag.Object;
                Value.Object = ObjectHandle.None;
                break;
            default:
                throw new ArgumentException($"{typeof(T).Name} cannot be written to a property.");
        }

        fixed (byte* Pointer = Text) {
            Value.Text = Pointer;

            if (!Interop.SetProperty(this, Name, Value)) {
                throw new ArgumentException($"{GetName()}.{Name} does not exist, is read-only, or is not {typeof(T).Name}.");
            }
        }
    }

    /// <summary>래퍼의 네이티브 객체 인덱스와 세대가 같은지 비교합니다.</summary>
    public bool Equals(EngineObject? Other) => Other is not null && Handle.Equals(Other.Handle);

    /// <summary>상대가 같은 네이티브 객체를 가리키는 래퍼인지 비교합니다.</summary>
    public override bool Equals(object? Other) => Other is EngineObject Object && Equals(Object);

    /// <summary>객체 핸들의 인덱스와 세대로 해시를 계산합니다.</summary>
    public override int GetHashCode() => Handle.GetHashCode();

    /// <summary>두 래퍼의 null 상태와 엔진 객체 핸들을 비교합니다.</summary>
    public static bool operator ==(EngineObject? Left, EngineObject? Right) => Left is null ? Right is null : Left.Equals(Right);

    /// <summary>두 래퍼가 서로 다른 엔진 객체를 가리키는지 비교합니다.</summary>
    public static bool operator !=(EngineObject? Left, EngineObject? Right) => !(Left == Right);

    /// <summary>살아 있는 객체는 클래스·이름을, 파괴된 객체는 파괴 상태를 표시합니다.</summary>
    public override string ToString() => IsValid ? $"{GetType().Name}({GetName()})" : $"{GetType().Name}(destroyed)";

    /// <summary>래퍼가 생성되지 않은 엔진 클래스를 가리킵니다.</summary>
    internal sealed class Untyped : EngineObject {

        /// <summary>구체적인 바인딩이 없는 엔진 객체를 핸들 기반 래퍼로 구성합니다.</summary>
        public Untyped(ObjectHandle Handle) : base(Handle) { }
    }
}
