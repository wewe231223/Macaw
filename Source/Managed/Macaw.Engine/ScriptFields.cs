using System.Globalization;
using System.Numerics;
using System.Reflection;

namespace Macaw;

/// <summary>[Field] 필드를 찾고 씬에 저장되는 문자열 값으로 변환합니다. 형식은 FDetailsPanel.cpp의 변환과 같아야 합니다.</summary>
internal static class ScriptFields {

    /// <summary>ScriptComponent까지 올라가며 [Field] 필드를 파생 클래스 먼저 나열합니다.</summary>
    public static IEnumerable<FieldInfo> Enumerate(Type Type) {
        for (Type? Current = Type; Current != null && Current != typeof(ScriptComponent); Current = Current.BaseType) {
            foreach (FieldInfo Field in Current.GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly)) {
                if (Field.IsDefined(typeof(FieldAttribute), false)) {
                    yield return Field;
                }
            }
        }
    }

    /// <summary>지원하는 필드 타입의 태그를 반환하고, 지원하지 않으면 null을 반환합니다.</summary>
    public static string? GetTag(Type Type) {
        if (Type == typeof(bool)) {
            return "Bool";
        }

        if (Type == typeof(int)) {
            return "Int";
        }

        if (Type == typeof(float)) {
            return "Float";
        }

        if (Type == typeof(string)) {
            return "String";
        }

        if (Type == typeof(Actor)) {
            return "Actor";
        }

        return Type == typeof(Vector3) ? "Vector3" : null;
    }

    /// <summary>지원하는 필드 값을 문화권에 독립적인 장면 저장 문자열로 변환합니다.</summary>
    public static string Format(object? Value) {
        return Value switch {
            bool Boolean => Boolean ? "true" : "false",
            int Integer => Integer.ToString(CultureInfo.InvariantCulture),
            float Single => Single.ToString("R", CultureInfo.InvariantCulture),
            Vector3 Vector => string.Create(CultureInfo.InvariantCulture, $"{Vector.X:R} {Vector.Y:R} {Vector.Z:R}"),
            string Text => Text,
            _ => "",
        };
    }

    /// <summary>형식 태그에 따라 저장 문자열을 값으로 해석하며 잘못된 숫자 형식은 예외를 전달합니다.</summary>
    public static object? Parse(string Tag, string Value) {
        switch (Tag) {
            case "Actor":
                string[] Handle = Value.Split(':');
                return Bindings.Wrap(new ObjectHandle(uint.Parse(Handle[0], CultureInfo.InvariantCulture), uint.Parse(Handle[1], CultureInfo.InvariantCulture)));
            case "Bool":
                return Value == "true";
            case "Int":
                return int.Parse(Value, CultureInfo.InvariantCulture);
            case "Float":
                return float.Parse(Value, CultureInfo.InvariantCulture);
            case "Vector3":
                string[] Parts = Value.Split(' ');
                return new Vector3(float.Parse(Parts[0], CultureInfo.InvariantCulture), float.Parse(Parts[1], CultureInfo.InvariantCulture), float.Parse(Parts[2], CultureInfo.InvariantCulture));
            default:
                return Value;
        }
    }
}
