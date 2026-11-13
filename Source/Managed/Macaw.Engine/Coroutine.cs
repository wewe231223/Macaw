using System.Collections;

namespace Macaw;

/// <summary>StartCoroutine이 돌려주는 실행 중인 코루틴입니다. StopCoroutine에 넘기거나, 다른 코루틴에서 yield return해 끝날 때까지 기다릴 수 있습니다.</summary>
public sealed class Coroutine {

    /// <summary>중첩 yield를 순서대로 실행하는 열거자 스택입니다.</summary>
    internal readonly Stack<IEnumerator> mStack = new();

    /// <summary>재개 전까지 남은 게임 시간(초)입니다.</summary>
    internal float mWaitSeconds;

    /// <summary>다시 실행하기 전에 참이 되어야 할 조건이며 대기가 없으면 null입니다.</summary>
    internal Func<bool>? mWaitUntil;

    /// <summary>완료를 기다리는 다른 코루틴이며 대기가 없으면 null입니다.</summary>
    internal Coroutine? mWaitCoroutine;

    /// <summary>끝났거나 멈췄는지 나타냅니다.</summary>
    public bool IsFinished => mStack.Count == 0;
}

/// <summary>코루틴에서 yield return하면 지정한 게임 시간 동안 기다립니다. 일시 정지 중에는 흐르지 않습니다.</summary>
public sealed class WaitForSeconds {

    /// <summary>대기할 게임 시간(초)입니다.</summary>
    internal readonly float mSeconds;

    /// <summary>지정한 게임 시간(초)만큼 기다리는 요청을 만듭니다.</summary>
    public WaitForSeconds(float Seconds) {
        mSeconds = Seconds;
    }
}

/// <summary>코루틴에서 yield return하면 조건이 참이 될 때까지 기다립니다.</summary>
public sealed class WaitUntil {

    /// <summary>코루틴을 재개할지 판단하는 조건 콜백입니다.</summary>
    internal readonly Func<bool> mCondition;

    /// <summary>조건이 참이 될 때까지 기다리는 요청을 만듭니다.</summary>
    public WaitUntil(Func<bool> Condition) {
        mCondition = Condition;
    }
}

/// <summary>한 스크립트의 코루틴을 Tick마다 진행합니다. 예외는 호출자에게 전달되어 스크립트를 비활성화합니다.</summary>
internal sealed class CoroutineRunner {

    /// <summary>이 스크립트가 실행 중인 코루틴 목록입니다.</summary>
    private readonly List<Coroutine> mCoroutines = new();

    /// <summary>코루틴을 등록하고 첫 yield 또는 완료까지 즉시 실행합니다.</summary>
    public Coroutine Start(IEnumerator Routine) {
        var Coroutine = new Coroutine();
        Coroutine.mStack.Push(Routine);
        mCoroutines.Add(Coroutine);
        Resume(Coroutine);

        return Coroutine;
    }

    /// <summary>지정한 코루틴의 실행 스택을 비우고 목록에서 제거합니다.</summary>
    public void Stop(Coroutine Coroutine) {
        Coroutine.mStack.Clear();
        mCoroutines.Remove(Coroutine);
    }

    /// <summary>이 실행기가 관리하는 모든 코루틴을 완료 상태로 만듭니다.</summary>
    public void Clear() {
        foreach (Coroutine Coroutine in mCoroutines) {
            Coroutine.mStack.Clear();
        }

        mCoroutines.Clear();
    }

    /// <summary>경과 게임 시간(초)으로 대기를 갱신하고 준비된 코루틴을 재개합니다.</summary>
    public void Advance(float DeltaTime) {
        // 진행 중에 코루틴이 추가되거나 멈출 수 있으므로 복사본을 순회합니다. 새 코루틴은 Start에서 이미 한 번 진행됐습니다.
        foreach (Coroutine Coroutine in mCoroutines.ToArray()) {
            if (!Coroutine.IsFinished && IsReady(Coroutine, DeltaTime)) {
                Resume(Coroutine);
            }
        }

        mCoroutines.RemoveAll(Coroutine => Coroutine.IsFinished);
    }

    /// <summary>시간·조건·다른 코루틴의 대기 조건이 충족되었는지 확인합니다.</summary>
    private static bool IsReady(Coroutine Coroutine, float DeltaTime) {
        if (Coroutine.mWaitSeconds > 0.0f) {
            Coroutine.mWaitSeconds -= DeltaTime;
            return Coroutine.mWaitSeconds <= 0.0f;
        }

        if (Coroutine.mWaitUntil != null) {
            if (!Coroutine.mWaitUntil()) {
                return false;
            }

            Coroutine.mWaitUntil = null;
        }

        if (Coroutine.mWaitCoroutine != null) {
            if (!Coroutine.mWaitCoroutine.IsFinished) {
                return false;
            }

            Coroutine.mWaitCoroutine = null;
        }

        return true;
    }

    /// <summary>다음 대기 요청 또는 완료까지 중첩 열거자를 진행하며 지원하지 않는 yield 값은 거부합니다.</summary>
    private static void Resume(Coroutine Coroutine) {
        while (Coroutine.mStack.TryPeek(out IEnumerator? Routine)) {
            if (!Routine.MoveNext()) {
                Coroutine.mStack.Pop();
                continue;
            }

            switch (Routine.Current) {
                case null:
                    return;
                case WaitForSeconds Wait:
                    Coroutine.mWaitSeconds = Wait.mSeconds;
                    return;
                case WaitUntil Wait:
                    Coroutine.mWaitUntil = Wait.mCondition;
                    return;
                case Coroutine Other:
                    Coroutine.mWaitCoroutine = Other;
                    return;
                case IEnumerator Nested:
                    Coroutine.mStack.Push(Nested);
                    continue;
                default:
                    throw new InvalidOperationException($"A coroutine yielded {Routine.Current.GetType().Name}. Yield null, WaitForSeconds, WaitUntil, Coroutine or IEnumerator.");
            }
        }
    }
}
