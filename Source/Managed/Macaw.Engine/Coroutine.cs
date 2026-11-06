using System.Collections;

namespace Macaw;

/// <summary>StartCoroutine이 돌려주는 실행 중인 코루틴입니다. StopCoroutine에 넘기거나, 다른 코루틴에서 yield return해 끝날 때까지 기다릴 수 있습니다.</summary>
public sealed class Coroutine {
    internal readonly Stack<IEnumerator> mStack = new();
    internal float mWaitSeconds;
    internal Func<bool>? mWaitUntil;
    internal Coroutine? mWaitCoroutine;

    /// <summary>끝났거나 멈췄는지 나타냅니다.</summary>
    public bool IsFinished => mStack.Count == 0;
}

/// <summary>코루틴에서 yield return하면 지정한 게임 시간 동안 기다립니다. 일시 정지 중에는 흐르지 않습니다.</summary>
public sealed class WaitForSeconds {
    internal readonly float mSeconds;

    public WaitForSeconds(float Seconds) {
        mSeconds = Seconds;
    }
}

/// <summary>코루틴에서 yield return하면 조건이 참이 될 때까지 기다립니다.</summary>
public sealed class WaitUntil {
    internal readonly Func<bool> mCondition;

    public WaitUntil(Func<bool> Condition) {
        mCondition = Condition;
    }
}

/// <summary>한 스크립트의 코루틴을 Tick마다 진행합니다. 예외는 호출자에게 전달되어 스크립트를 비활성화합니다.</summary>
internal sealed class CoroutineRunner {
    private readonly List<Coroutine> mCoroutines = new();

    public Coroutine Start(IEnumerator Routine) {
        var Coroutine = new Coroutine();
        Coroutine.mStack.Push(Routine);
        mCoroutines.Add(Coroutine);
        Resume(Coroutine);

        return Coroutine;
    }

    public void Stop(Coroutine Coroutine) {
        Coroutine.mStack.Clear();
        mCoroutines.Remove(Coroutine);
    }

    public void Clear() {
        foreach (Coroutine Coroutine in mCoroutines) {
            Coroutine.mStack.Clear();
        }

        mCoroutines.Clear();
    }

    public void Advance(float DeltaTime) {
        // 진행 중에 코루틴이 추가되거나 멈출 수 있으므로 복사본을 순회합니다. 새 코루틴은 Start에서 이미 한 번 진행됐습니다.
        foreach (Coroutine Coroutine in mCoroutines.ToArray()) {
            if (!Coroutine.IsFinished && IsReady(Coroutine, DeltaTime)) {
                Resume(Coroutine);
            }
        }

        mCoroutines.RemoveAll(Coroutine => Coroutine.IsFinished);
    }

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
