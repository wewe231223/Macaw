using System.Numerics;
using Macaw;

namespace Game;

/// <summary>액터를 일정한 속도로 회전시키는 예제입니다. [Field]를 붙인 필드는 Details 패널에서 편집할 수 있습니다.</summary>
public sealed class Rotator : ScriptComponent {
    [Field] public float Speed = 1.0f;
    [Field] public Vector3 Axis = Vector3.UnitZ;

    public override void Tick(float DeltaTime) {
        Quaternion Step = Quaternion.CreateFromAxisAngle(Vector3.Normalize(Axis), Speed * DeltaTime);
        Owner.Rotation = Quaternion.Normalize(Owner.Rotation * Step);
    }
}
