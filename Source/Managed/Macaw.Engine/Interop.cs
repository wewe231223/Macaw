using System.Numerics;
using System.Runtime.InteropServices;

namespace Macaw;

/// <summary>엔진 객체를 가리키는 핸들입니다. CoreUObject::FObjectHandle과 배치가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal readonly struct ObjectHandle : IEquatable<ObjectHandle> {

    /// <summary>어떤 엔진 객체도 가리키지 않는 예약 핸들입니다.</summary>
    public static readonly ObjectHandle None = new(uint.MaxValue, 0);

    /// <summary>네이티브 객체 레지스트리의 슬롯 인덱스입니다.</summary>
    public readonly uint Index;

    /// <summary>슬롯 재사용 후 이전 객체와 구분하는 세대 번호입니다.</summary>
    public readonly uint Generation;

    /// <summary>네이티브 객체 슬롯과 세대 번호로 핸들을 구성합니다.</summary>
    public ObjectHandle(uint Index, uint Generation) {
        this.Index = Index;
        this.Generation = Generation;
    }

    /// <summary>예약된 빈 슬롯 인덱스인지 확인합니다.</summary>
    public bool IsNone => Index == uint.MaxValue;

    /// <summary>슬롯 인덱스와 세대 번호가 모두 같은지 비교합니다.</summary>
    public bool Equals(ObjectHandle Other) => Index == Other.Index && Generation == Other.Generation;

    /// <summary>상대가 같은 슬롯·세대의 객체 핸들인지 비교합니다.</summary>
    public override bool Equals(object? Other) => Other is ObjectHandle Handle && Equals(Handle);

    /// <summary>슬롯 인덱스와 세대로 핸들 해시를 계산합니다.</summary>
    public override int GetHashCode() => HashCode.Combine(Index, Generation);
}

/// <summary>위치, 회전과 스케일입니다. ScriptAbi.h의 FTransform과 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
public struct Transform {

    /// <summary>변환의 위치 벡터입니다.</summary>
    public Vector3 Location;

    /// <summary>변환의 회전 쿼터니언입니다.</summary>
    public Quaternion Rotation;

    /// <summary>변환의 축별 크기 배율입니다.</summary>
    public Vector3 Scale;
}

/// <summary>레이캐스트 결과입니다. ScriptAbi.h의 FWorldHit과 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
public readonly struct RaycastHit {

    /// <summary>충돌한 액터의 네이티브 핸들입니다.</summary>
    private readonly ObjectHandle mActor;

    /// <summary>충돌한 컴포넌트의 네이티브 핸들입니다.</summary>
    private readonly ObjectHandle mComponent;

    /// <summary>충돌 지점의 월드 좌표입니다.</summary>
    private readonly Vector3 mPosition;

    /// <summary>광선 시작점부터 충돌 지점까지의 거리입니다.</summary>
    private readonly float mDistance;

    /// <summary>무언가에 맞았는지 나타냅니다.</summary>
    public bool IsHit => !mActor.IsNone;

    /// <summary>맞은 액터이며, 맞은 것이 없으면 null입니다.</summary>
    public Actor? Actor => (Actor?)Bindings.Wrap(mActor);

    /// <summary>맞은 컴포넌트이며, 맞은 것이 없으면 null입니다.</summary>
    public ActorComponent? Component => (ActorComponent?)Bindings.Wrap(mComponent);

    /// <summary>맞은 위치입니다.</summary>
    public Vector3 Position => mPosition;

    /// <summary>광선 시작점부터의 거리이며 단위는 미터입니다.</summary>
    public float Distance => mDistance;
}

/// <summary>프로퍼티 값의 종류입니다. FScriptInterop.h의 EScriptValueTag와 순서가 같아야 합니다.</summary>
internal enum ScriptValueTag {
    None,
    Bool,
    Int,
    Uint,
    Uint64,
    Float,
    String,
    Vector2,
    Vector3,
    Vector4,
    Quaternion,
    Transform,
    Object,
}

/// <summary>경계를 넘는 프로퍼티 값입니다. FScriptInterop.h의 FScriptValue와 배치가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ScriptValue {

    /// <summary>현재 유효한 값 저장 영역을 구분하는 태그입니다.</summary>
    public ScriptValueTag Tag;

    /// <summary>벡터·쿼터니언·변환의 성분을 전달하는 고정 길이 저장 영역입니다.</summary>
    public fixed float Floats[10];

    /// <summary>정수와 Bool 값을 전달하는 저장 영역입니다.</summary>
    public ulong Integer;

    /// <summary>호출 규약의 유효 기간 안에서만 사용하는 UTF-8 문자열 포인터입니다.</summary>
    public byte* Text;

    /// <summary>객체 값을 전달하는 세대 검증 핸들입니다.</summary>
    public ObjectHandle Object;
}

/// <summary>엔진이 C#에 제공하는 함수 테이블입니다. FScriptRuntime.cpp의 FNativeApi와 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeApi {

    /// <summary>함수 테이블의 바이트 크기이며 시작 시 ABI 일치 여부를 검증합니다.</summary>
    public nint Size;

    /// <summary>UTF-8 메시지를 엔진 로그에 기록하는 네이티브 함수입니다.</summary>
    public delegate* unmanaged<byte*, void> Log;

    /// <summary>객체 핸들의 생존 여부를 확인하는 네이티브 함수입니다.</summary>
    public delegate* unmanaged<ObjectHandle, int> ObjectIsAlive;

    /// <summary>스크립트 클래스 이름을 네이티브 카탈로그에 등록하는 함수입니다.</summary>
    public delegate* unmanaged<byte*, void> CatalogAddClass;

    /// <summary>스크립트 필드 메타데이터를 네이티브 카탈로그에 등록하는 함수입니다.</summary>
    public delegate* unmanaged<byte*, byte*, byte*, void> CatalogAddField;

    /// <summary>생성기가 동일 순서로 배치한 네이티브 바인딩 함수 주소 배열입니다.</summary>
    public nint* Bindings;

    /// <summary>네이티브 바인딩 테이블의 함수 개수입니다.</summary>
    public nint BindingCount;

    /// <summary>바인딩 서명·순서의 일치 여부를 검증하는 해시입니다.</summary>
    public nint BindingHash;
}

/// <summary>C#이 엔진에 제공하는 함수 테이블입니다. FScriptRuntime.cpp의 FManagedApi와 순서가 같아야 합니다.</summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedApi {

    /// <summary>관리 함수 테이블의 바이트 크기이며 시작 시 ABI를 검증합니다.</summary>
    public nint Size;

    /// <summary>사용자 스크립트 어셈블리를 직접 또는 섀도 복사하여 로드하는 함수입니다.</summary>
    public delegate* unmanaged<byte*, int, int> LoadScripts;

    /// <summary>현재 스크립트 컨텍스트를 언로드하고 수집 완료 여부를 반환하는 함수입니다.</summary>
    public delegate* unmanaged<int> UnloadScripts;

    /// <summary>소유 액터에 스크립트를 만들고 GCHandle을 반환하는 함수입니다.</summary>
    public delegate* unmanaged<byte*, ObjectHandle, nint> CreateInstance;

    /// <summary>스크립트의 게임 시작 콜백을 호출하는 함수입니다.</summary>
    public delegate* unmanaged<nint, void> BeginPlay;

    /// <summary>경과 시간(초)으로 스크립트와 코루틴을 갱신하는 함수입니다.</summary>
    public delegate* unmanaged<nint, float, void> Tick;

    /// <summary>코루틴을 중지하고 게임 종료 콜백을 호출하는 함수입니다.</summary>
    public delegate* unmanaged<nint, void> EndPlay;

    /// <summary>스크립트 목록과 GCHandle을 해제하는 함수입니다.</summary>
    public delegate* unmanaged<nint, void> DestroyInstance;

    /// <summary>스크립트 클래스와 필드 메타데이터를 네이티브에 전달하는 함수입니다.</summary>
    public delegate* unmanaged<void> DescribeScripts;

    /// <summary>문자열 형식의 필드 값을 해석하여 인스턴스에 적용하는 함수입니다.</summary>
    public delegate* unmanaged<nint, byte*, byte*, byte*, int> SetField;

    /// <summary>액터 겹침 시작·종료 이벤트를 전달하는 함수입니다.</summary>
    public delegate* unmanaged<nint, int, ObjectHandle, void> Overlap;

    /// <summary>네이티브 카메라 모디파이어에 C# 인스턴스를 연결하는 함수입니다.</summary>
    public delegate* unmanaged<byte*, ObjectHandle, nint> CreateModifier;

    /// <summary>공유 ABI 카메라 시점을 평가하고 후속 모디파이어 중단 여부를 반환하는 함수입니다.</summary>
    public delegate* unmanaged<nint, float, MinimalViewInfo*, int> ModifyCamera;

    /// <summary>C# 카메라 모디파이어의 등록과 GCHandle을 해제하는 함수입니다.</summary>
    public delegate* unmanaged<nint, void> DestroyModifier;
}
