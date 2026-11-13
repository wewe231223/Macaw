using System.Numerics;
using System.Runtime.InteropServices;

namespace Macaw;

/// <summary>카메라 관리자가 그릴 시점입니다. 엔진의 FMinimalViewInfo와 같은 값이며 배치가 고정되어 있습니다.</summary>
[StructLayout(LayoutKind.Sequential)]
public struct MinimalViewInfo {

    /// <summary>카메라의 월드 위치이며 미터 단위입니다.</summary>
    public Vector3 Location;

    /// <summary>카메라의 월드 회전입니다. 앞은 X, 오른쪽은 Y, 위는 Z입니다.</summary>
    public Quaternion Rotation;

    /// <summary>수직 시야각이며 도 단위입니다.</summary>
    public float FieldOfView;
}

/// <summary>
/// C#으로 만드는 카메라 모디파이어의 기반 클래스입니다. PlayerCameraManager.AddCameraModifier&lt;T&gt;()로 추가합니다.
/// 카메라 관리자는 뷰 타깃 블렌드가 끝난 시점을 Priority 순서대로 각 모디파이어의 ModifyCamera에 넘깁니다.
/// 효과는 Alpha로 세기를 조절해야 켜고 끌 때 부드럽게 바뀝니다.
/// </summary>
public abstract class ScriptCameraModifier {

    /// <summary>예외가 발생해 더 이상 호출하지 않는 인스턴스인지 나타냅니다.</summary>
    internal bool mFaulted;

    /// <summary>이 스크립트와 연결된 엔진 모디파이어입니다. 켜고 끄기, Priority, AlphaIn/OutTime을 여기서 다룹니다.</summary>
    public CameraModifier Modifier { get; internal set; } = null!;

    /// <summary>이 모디파이어를 소유한 카메라 관리자입니다.</summary>
    public PlayerCameraManager CameraManager => Modifier.GetCameraOwner() ?? throw new InvalidOperationException("The modifier is not on a camera.");

    /// <summary>현재 효과의 세기이며 0부터 1까지입니다.</summary>
    public float Alpha => Modifier.GetAlpha();

    /// <summary>카메라 관리자에 추가된 직후 호출됩니다. AlphaInTime 같은 설정은 여기서 합니다.</summary>
    public virtual void AddedToCamera() { }

    /// <summary>시점을 고칩니다. true를 반환하면 뒤 순서의 모디파이어는 이번 프레임에 적용되지 않습니다.</summary>
    public virtual bool ModifyCamera(float DeltaTime, ref MinimalViewInfo View) => false;
}

/// <summary>카메라 관리자에 사용자 C# 모디파이어를 추가·조회하는 편의 API를 제공합니다.</summary>
public partial class PlayerCameraManager {

    /// <summary>T 타입 C# 모디파이어를 만들어 추가합니다. 클래스를 찾지 못하면 null입니다.</summary>
    public T? AddCameraModifier<T>(int Priority = 127) where T : ScriptCameraModifier {
        CameraModifier? Added = CameraModifiers.AddModifier(this, typeof(T).FullName!, Priority);

        return Added != null ? Host.FindModifier(Added.Handle) as T : null;
    }

    /// <summary>C# 모디파이어를 빼고 해제합니다.</summary>
    public bool RemoveCameraModifier(ScriptCameraModifier Modifier) => RemoveCameraModifier(Modifier.Modifier);

    /// <summary>추가된 T 타입 C# 모디파이어 중 첫 번째를 찾습니다.</summary>
    public T? FindCameraModifier<T>() where T : ScriptCameraModifier => Host.FindModifierOf<T>(this);
}
