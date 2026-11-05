namespace Macaw;

/// <summary>모든 C# 스크립트의 기반 클래스입니다. 생성자에서는 엔진 API를 쓸 수 없으므로 초기화는 BeginPlay에 둡니다.</summary>
public abstract class ScriptComponent {
    /// <summary>예외가 발생해 더 이상 호출하지 않는 인스턴스인지 나타냅니다.</summary>
    internal bool mFaulted;

    /// <summary>이 스크립트를 소유한 액터입니다.</summary>
    public Actor Owner { get; internal set; } = null!;

    public virtual void BeginPlay() { }

    public virtual void Tick(float DeltaTime) { }

    public virtual void EndPlay() { }
}
