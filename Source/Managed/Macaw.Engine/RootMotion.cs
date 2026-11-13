namespace Macaw;

/// <summary>애니메이션 루트 모션의 추출과 소비용 누적 방식을 지정합니다.</summary>
public enum RootMotionMode {

    /// <summary>루트 모션을 추출하지 않고 원본 포즈를 유지합니다.</summary>
    NoRootMotionExtraction,

    /// <summary>포즈에서 루트 모션을 추출하지만 액터 이동용으로 누적하지 않습니다.</summary>
    IgnoreRootMotion,

    /// <summary>평가한 애니메이션의 루트 모션을 블렌딩하여 소비용으로 누적합니다.</summary>
    RootMotionFromEverything
}

/// <summary>스켈레탈 메시의 관리 코드용 루트 모션 속성을 제공합니다.</summary>
public partial class SkeletalMeshComponent {

    /// <summary>현재 재생 인스턴스의 루트 모션 모드이며 유효하지 않은 열거값을 설정하면 예외가 발생합니다.</summary>
    public RootMotionMode RootMotionMode {
        get {
            return (RootMotionMode)GetRootMotionModeValue();
        }
        set {
            if (!SetRootMotionModeValue((int)value)) {
                throw new ArgumentOutOfRangeException(nameof(value));
            }
        }
    }
}
