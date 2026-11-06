namespace Macaw;

/// <summary>Details 패널에서 편집하고 씬에 저장할 필드를 표시합니다. bool, int, float, string, Vector3를 지원합니다.</summary>
[AttributeUsage(AttributeTargets.Field)]
public sealed class FieldAttribute : Attribute {
}
