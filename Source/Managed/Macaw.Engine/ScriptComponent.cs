using System.Collections;

namespace Macaw;

/// <summary>모든 C# 스크립트의 기반 클래스입니다. 생성자에서는 엔진 API를 쓸 수 없으므로 초기화는 BeginPlay에 둡니다.</summary>
public abstract class ScriptComponent {

    /// <summary>예외가 발생해 더 이상 호출하지 않는 인스턴스인지 나타냅니다.</summary>
    internal bool mFaulted;

    /// <summary>이 스크립트의 코루틴 실행 상태를 소유하며 EndPlay에서 모두 중지합니다.</summary>
    internal readonly CoroutineRunner mCoroutines = new();

    /// <summary>이 스크립트를 소유한 액터입니다.</summary>
    public Actor Owner { get; internal set; } = null!;

    /// <summary>소유 액터가 속한 월드입니다.</summary>
    public World World => Owner.GetWorld() ?? throw new InvalidOperationException("The owner is not in a world.");

    /// <summary>소유 액터가 게임을 시작할 때 호출됩니다. 엔진 API를 사용하는 초기화는 여기에서 수행합니다.</summary>
    public virtual void BeginPlay() { }

    /// <summary>활성 스크립트의 게임 갱신 콜백이며 DeltaTime은 경과 시간(초)입니다.</summary>
    public virtual void Tick(float DeltaTime) { }

    /// <summary>소유 액터의 게임 실행이 끝날 때 호출되며 코루틴은 호출 전에 중지됩니다.</summary>
    public virtual void EndPlay() { }

    /// <summary>이 액터의 충돌 도형이 다른 액터의 충돌 도형과 겹치기 시작할 때 호출됩니다.</summary>
    public virtual void OnBeginOverlap(Actor Other) { }

    /// <summary>겹침이 끝날 때 호출됩니다. 상대가 파괴되어 끝났으면 Other는 null입니다.</summary>
    public virtual void OnEndOverlap(Actor? Other) { }

    /// <summary>소유 액터의 T 타입 컴포넌트를 찾습니다.</summary>
    public T? GetComponent<T>() where T : ActorComponent => Owner.GetComponent<T>();

    /// <summary>코루틴을 시작합니다. 첫 yield까지는 바로 실행되고, EndPlay에서 자동으로 멈춥니다.</summary>
    public Coroutine StartCoroutine(IEnumerator Routine) => mCoroutines.Start(Routine);

    /// <summary>코루틴을 멈춥니다.</summary>
    public void StopCoroutine(Coroutine Coroutine) => mCoroutines.Stop(Coroutine);

    /// <summary>이 스크립트의 코루틴을 모두 멈춥니다.</summary>
    public void StopAllCoroutines() => mCoroutines.Clear();
}
