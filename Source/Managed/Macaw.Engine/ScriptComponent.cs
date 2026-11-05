namespace Macaw;

/// <summary>모든 C# 스크립트의 기반 클래스입니다.</summary>
public abstract class ScriptComponent {
    /// <summary>예외가 발생해 더 이상 호출하지 않는 인스턴스인지 나타냅니다.</summary>
    internal bool mFaulted;

    public virtual void Tick(float DeltaTime) { }
}
