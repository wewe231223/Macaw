namespace Macaw;

public enum RootMotionMode {
    NoRootMotionExtraction,
    IgnoreRootMotion,
    RootMotionFromEverything
}

public partial class SkeletalMeshComponent {
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
