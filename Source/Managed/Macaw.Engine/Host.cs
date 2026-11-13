using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Macaw;

/// <summary>사용자 스크립트를 언로드할 수 있도록 담는 수집형 로드 컨텍스트입니다.</summary>
internal sealed class ScriptLoadContext : AssemblyLoadContext {

    /// <summary>스크립트 어셈블리 기준으로 관리·네이티브 의존성 경로를 해석합니다.</summary>
    private readonly AssemblyDependencyResolver mResolver;

    /// <summary>메인 스크립트 경로를 기준으로 언로드 가능한 의존성 해석기를 구성합니다.</summary>
    public ScriptLoadContext(string MainAssembly) : base("GameScripts", isCollectible: true) {
        mResolver = new AssemblyDependencyResolver(MainAssembly);
    }

    // null을 돌려주면 기본 컨텍스트로 해석되므로 Macaw.Engine과 프레임워크 타입을 공유합니다.

    /// <summary>Macaw.Engine은 기본 컨텍스트와 공유하고 스크립트 의존성은 전용 컨텍스트에 로드합니다.</summary>
    protected override Assembly? Load(AssemblyName Name) {
        if (Name.Name == typeof(Host).Assembly.GetName().Name) {
            return null;
        }

        string? Path = mResolver.ResolveAssemblyToPath(Name);

        return Path != null ? LoadFromAssemblyPath(Path) : null;
    }

    /// <summary>스크립트의 네이티브 의존성을 해석하며 경로가 없으면 0을 반환합니다.</summary>
    protected override nint LoadUnmanagedDll(string Name) {
        string? Path = mResolver.ResolveUnmanagedDllToPath(Name);

        return Path != null ? LoadUnmanagedDllFromPath(Path) : 0;
    }
}

/// <summary>엔진이 호출하는 진입점입니다. 예외가 네이티브로 넘어가면 프로세스가 종료되므로 모든 진입점에서 잡습니다.</summary>
internal static unsafe class Host {

    /// <summary>빌드 출력을 잠그지 않도록 스크립트를 복사할 하위 폴더 이름입니다.</summary>
    private const string ShadowFolder = ".Loaded";

    /// <summary>현재 스크립트 세대를 소유하는 수집형 로드 컨텍스트입니다.</summary>
    private static ScriptLoadContext? sContext;

    /// <summary>현재 세대의 사용자 스크립트 어셈블리입니다.</summary>
    private static Assembly? sAssembly;

    /// <summary>현재 세대의 섀도 복사 경로이며 원본 직접 로드 시 null입니다.</summary>
    private static string? sShadowDirectory;

    /// <summary>프로세스 안에서 섀도 복사 디렉터리를 구분하는 세대 번호입니다.</summary>
    private static int sGeneration;

    /// <summary>소유 액터 핸들별로 생성된 스크립트 인스턴스를 보관합니다.</summary>
    private static readonly Dictionary<ObjectHandle, List<ScriptComponent>> sScripts = new();

    /// <summary>액터에 붙은 살아 있는 스크립트 목록을 반환합니다.</summary>
    internal static IReadOnlyList<ScriptComponent> GetScripts(ObjectHandle Owner) {
        return sScripts.TryGetValue(Owner, out List<ScriptComponent>? Scripts) ? Scripts : Array.Empty<ScriptComponent>();
    }

    /// <summary>네이티브·관리 함수 테이블의 크기와 바인딩 해시를 검증하여 연결하며 성공하면 1을 반환합니다.</summary>
    [UnmanagedCallersOnly]
    public static int Bootstrap(NativeApi* Native, ManagedApi* Managed) {
        if (Native->Size != sizeof(NativeApi) || Managed->Size != sizeof(ManagedApi) || Native->BindingCount != Bindings.Count || (uint)Native->BindingHash != Bindings.Hash) {
            return 0;
        }

        Log.sApi = Native;
        EngineObject.sApi = Native;
        Bindings.sTable = Native->Bindings;
        Managed->LoadScripts = &LoadScripts;
        Managed->UnloadScripts = &UnloadScripts;
        Managed->CreateInstance = &CreateInstance;
        Managed->BeginPlay = &BeginPlay;
        Managed->Tick = &Tick;
        Managed->EndPlay = &EndPlay;
        Managed->DestroyInstance = &DestroyInstance;
        Managed->DescribeScripts = &DescribeScripts;
        Managed->SetField = &SetField;
        Managed->Overlap = &Overlap;
        Managed->CreateModifier = &CreateModifier;
        Managed->ModifyCamera = &ModifyCamera;
        Managed->DestroyModifier = &DestroyModifier;

        return 1;
    }

    /// <summary>의존성 파일과 하위 폴더를 복사하되 지정한 섀도 폴더는 제외합니다.</summary>
    private static void CopyDirectory(string Source, string Destination, string Excluded) {
        System.IO.Directory.CreateDirectory(Destination);

        foreach (string File in System.IO.Directory.GetFiles(Source)) {
            System.IO.File.Copy(File, Path.Combine(Destination, Path.GetFileName(File)), true);
        }

        foreach (string Directory in System.IO.Directory.GetDirectories(Source)) {
            if (Path.GetFileName(Directory) != Excluded) {
                CopyDirectory(Directory, Path.Combine(Destination, Path.GetFileName(Directory)), Excluded);
            }
        }
    }

    /// <summary>스크립트 어셈블리를 로드합니다. ShadowCopy가 0이 아니면 빌드 중 덮어쓸 수 있도록 출력 폴더 전체를 섀도 복사한 뒤 로드하며, NuGet 의존성도 함께 복사됩니다.</summary>
    /// <remarks>내보낸 게임은 다시 빌드하지 않고 설치 폴더가 읽기 전용일 수 있으므로 원본을 바로 로드합니다.</remarks>
    [UnmanagedCallersOnly]
    private static int LoadScripts(byte* PathUtf8, int ShadowCopy) {
        try {
            string Source = Path.GetFullPath(Marshal.PtrToStringUTF8((nint)PathUtf8)!);

            if (ShadowCopy == 0) {
                var Direct = new ScriptLoadContext(Source);
                sAssembly = Direct.LoadFromAssemblyPath(Source);
                sContext = Direct;
                sShadowDirectory = null;
                Log.Info($"C# scripts loaded: {Source}");

                return 1;
            }

            string Root = Path.Combine(Path.GetDirectoryName(Source)!, ShadowFolder);

            if (sGeneration == 0) {
                DeleteDirectory(Root);
            }

            string Directory = Path.Combine(Root, $"{Environment.ProcessId}_{++sGeneration}");
            CopyDirectory(Path.GetDirectoryName(Source)!, Directory, ShadowFolder);

            string Copy = Path.Combine(Directory, Path.GetFileName(Source));
            var Context = new ScriptLoadContext(Copy);
            sAssembly = Context.LoadFromAssemblyPath(Copy);
            sContext = Context;
            sShadowDirectory = Directory;
            Log.Info($"C# scripts loaded: {Source} (generation {sGeneration})");

            return 1;
        } catch (Exception Exception) {
            Log.Info($"C# scripts failed to load: {Exception}");
            return 0;
        }
    }

    // 지역 변수가 컨텍스트를 붙잡지 않도록 분리합니다.

    /// <summary>강한 참조를 해제하여 스크립트 언로드를 시작하고 수집 확인용 약한 참조를 반환합니다.</summary>
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static WeakReference BeginUnload() {
        var Weak = new WeakReference(sContext);
        sContext!.Unload();
        sContext = null;
        sAssembly = null;

        return Weak;
    }

    /// <summary>스크립트를 언로드하고 실제로 수집되면 1을, 무언가 붙잡고 있으면 0을 반환합니다.</summary>
    [UnmanagedCallersOnly]
    private static int UnloadScripts() {
        try {
            if (sContext == null) {
                return 1;
            }

            WeakReference Weak = BeginUnload();

            for (int Attempt = 0; Attempt < 10 && Weak.IsAlive; ++Attempt) {
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }

            if (Weak.IsAlive) {
                Log.Info("C# scripts are still referenced after unload; static events or threads may be holding them.");
                return 0;
            }

            DeleteDirectory(sShadowDirectory);

            return 1;
        } catch (Exception Exception) {
            Log.Info($"C# scripts failed to unload: {Exception}");
            return 0;
        }
    }

    /// <summary>이름으로 스크립트 인스턴스를 생성하여 액터에 연결하고 GCHandle을 반환하며 실패하면 0입니다.</summary>
    [UnmanagedCallersOnly]
    private static nint CreateInstance(byte* NameUtf8, ObjectHandle Owner) {
        try {
            string Name = Marshal.PtrToStringUTF8((nint)NameUtf8)!;

            if (sAssembly?.GetType(Name) is not { } Type || !Type.IsAssignableTo(typeof(ScriptComponent))) {
                Log.Info($"C# script class not found: {Name}");
                return 0;
            }

            var Instance = (ScriptComponent)Activator.CreateInstance(Type)!;
            Instance.Owner = (Actor)Bindings.Wrap(Owner)!;

            if (!sScripts.TryGetValue(Owner, out List<ScriptComponent>? Scripts)) {
                sScripts[Owner] = Scripts = new List<ScriptComponent>();
            }

            Scripts.Add(Instance);

            return GCHandle.ToIntPtr(GCHandle.Alloc(Instance));
        } catch (Exception Exception) {
            Log.Info($"C# script could not be created: {Exception}");
            return 0;
        }
    }

    /// <summary>문자열을 NUL로 끝나는 UTF-8 버퍼로 고정하여 콜백 동안만 포인터를 제공합니다.</summary>
    private static void CallWithUtf8(string Text, Action<nint> Call) {
        byte[] Bytes = System.Text.Encoding.UTF8.GetBytes(Text + "\0");

        fixed (byte* Pointer = Bytes) {
            Call((nint)Pointer);
        }
    }

    /// <summary>스크립트 클래스 이름을 네이티브 카탈로그에 추가합니다.</summary>
    private static void AddClass(string Name) {
        CallWithUtf8(Name, Text => EngineObject.sApi->CatalogAddClass((byte*)Text));
    }

    /// <summary>필드 이름·형식 태그·기본값을 UTF-8로 네이티브 카탈로그에 전달합니다.</summary>
    private static void AddField(string Name, string Tag, string Value) {
        CallWithUtf8(Name, NameText => CallWithUtf8(Tag, TagText => CallWithUtf8(Value, ValueText => EngineObject.sApi->CatalogAddField((byte*)NameText, (byte*)TagText, (byte*)ValueText))));
    }

    /// <summary>어셈블리 타입을 나열하며 일부 의존성 로드 실패 시 로드된 타입만 반환합니다.</summary>
    private static Type[] GetLoadableTypes(Assembly Assembly) {
        try {
            return Assembly.GetTypes();
        } catch (ReflectionTypeLoadException Exception) {
            Log.Info($"Some C# script types could not be loaded: {Exception.LoaderExceptions.FirstOrDefault()?.Message}");
            return Exception.Types.OfType<Type>().ToArray();
        }
    }

    /// <summary>생성할 수 있는 스크립트 클래스와 [Field] 필드를 엔진에 알립니다. 기본값은 임시 인스턴스에서 읽습니다.</summary>
    [UnmanagedCallersOnly]
    private static void DescribeScripts() {
        try {
            foreach (Type Type in GetLoadableTypes(sAssembly!).OrderBy(Candidate => Candidate.FullName, StringComparer.Ordinal)) {
                if (Type.IsAbstract || !Type.IsAssignableTo(typeof(ScriptComponent)) || Type.GetConstructor(Type.EmptyTypes) == null) {
                    continue;
                }

                object? Defaults = null;

                try {
                    Defaults = Activator.CreateInstance(Type);
                } catch (Exception Exception) {
                    Log.Info($"C# script {Type.FullName} threw in its constructor, so its field defaults are unknown: {Exception.InnerException?.Message ?? Exception.Message}");
                }

                AddClass(Type.FullName!);

                foreach (FieldInfo Field in ScriptFields.Enumerate(Type)) {
                    string? Tag = ScriptFields.GetTag(Field.FieldType);

                    if (Tag == null || Field.IsInitOnly) {
                        Log.Info($"[Field] {Type.FullName}.{Field.Name} is ignored: {(Tag == null ? $"{Field.FieldType.Name} is not supported" : "the field is readonly")}.");
                        continue;
                    }

                    object? Value = Defaults != null ? Field.GetValue(Defaults) : Field.FieldType.IsValueType ? Activator.CreateInstance(Field.FieldType) : null;
                    AddField(Field.Name, Tag, ScriptFields.Format(Value));
                }
            }
        } catch (Exception Exception) {
            Log.Info($"C# scripts could not be described: {Exception}");
        }
    }

    /// <summary>씬에 저장된 값을 필드에 넣습니다. 이름이나 타입이 바뀐 필드는 경고만 남기고 건너뜁니다.</summary>
    [UnmanagedCallersOnly]
    private static int SetField(nint Handle, byte* NameUtf8, byte* TagUtf8, byte* ValueUtf8) {
        try {
            ScriptComponent Instance = GetInstance(Handle);
            string Name = Marshal.PtrToStringUTF8((nint)NameUtf8)!;
            string Tag = Marshal.PtrToStringUTF8((nint)TagUtf8)!;
            FieldInfo? Field = ScriptFields.Enumerate(Instance.GetType()).FirstOrDefault(Candidate => Candidate.Name == Name);

            if (Field == null || Field.IsInitOnly || ScriptFields.GetTag(Field.FieldType) != Tag) {
                Log.Info($"Saved field {Instance.GetType().FullName}.{Name} ({Tag}) no longer matches the script and was skipped.");
                return 0;
            }

            Field.SetValue(Instance, ScriptFields.Parse(Tag, Marshal.PtrToStringUTF8((nint)ValueUtf8)!));

            return 1;
        } catch (Exception Exception) {
            Log.Info($"A saved C# field could not be applied: {Exception.Message}");
            return 0;
        }
    }

    /// <summary>유효한 GCHandle에서 스크립트 인스턴스를 조회합니다.</summary>
    private static ScriptComponent GetInstance(nint Handle) => (ScriptComponent)GCHandle.FromIntPtr(Handle).Target!;

    /// <summary>예외가 발생한 인스턴스를 기록 후 비활성화합니다.</summary>
    private static void Disable(ScriptComponent Instance, Exception Exception) {
        Instance.mFaulted = true;

        try {
            Log.Info($"C# script disabled by an exception: {Exception}");
        } catch {
        }
    }

    /// <summary>스크립트의 BeginPlay를 호출하며 예외 발생 시 인스턴스를 비활성화합니다.</summary>
    [UnmanagedCallersOnly]
    private static void BeginPlay(nint Handle) {
        ScriptComponent Instance = GetInstance(Handle);

        try {
            Instance.BeginPlay();
        } catch (Exception Exception) {
            Disable(Instance, Exception);
        }
    }

    /// <summary>활성 스크립트의 Tick과 코루틴을 갱신하며 예외는 비활성화로 처리합니다.</summary>
    [UnmanagedCallersOnly]
    private static void Tick(nint Handle, float DeltaTime) {
        ScriptComponent Instance = GetInstance(Handle);

        if (Instance.mFaulted) {
            return;
        }

        try {
            Instance.Tick(DeltaTime);
            Instance.mCoroutines.Advance(DeltaTime);
        } catch (Exception Exception) {
            Disable(Instance, Exception);
        }
    }

    /// <summary>모든 코루틴을 중지하고 오류 없는 스크립트의 EndPlay를 호출합니다.</summary>
    [UnmanagedCallersOnly]
    private static void EndPlay(nint Handle) {
        ScriptComponent Instance = GetInstance(Handle);
        Instance.mCoroutines.Clear();

        if (Instance.mFaulted) {
            return;
        }

        try {
            Instance.EndPlay();
        } catch (Exception Exception) {
            Disable(Instance, Exception);
        }
    }

    /// <summary>액터 겹침의 시작·종료 콜백을 호출하며 상대 파괴 시 종료 인수는 null일 수 있습니다.</summary>
    [UnmanagedCallersOnly]
    private static void Overlap(nint Handle, int Begin, ObjectHandle Other) {
        ScriptComponent Instance = GetInstance(Handle);

        if (Instance.mFaulted) {
            return;
        }

        try {
            var OtherActor = (Actor?)Bindings.Wrap(Other);

            if (Begin == 0) {
                Instance.OnEndOverlap(OtherActor);
            } else if (OtherActor != null) {
                Instance.OnBeginOverlap(OtherActor);
            }
        } catch (Exception Exception) {
            Disable(Instance, Exception);
        }
    }

    /// <summary>네이티브 모디파이어 핸들별로 연결한 C# 인스턴스를 보관합니다.</summary>
    private static readonly Dictionary<ObjectHandle, ScriptCameraModifier> sModifiers = new();

    /// <summary>엔진 모디파이어에 연결된 C# 모디파이어를 찾습니다.</summary>
    internal static ScriptCameraModifier? FindModifier(ObjectHandle Modifier) {
        return sModifiers.TryGetValue(Modifier, out ScriptCameraModifier? Instance) ? Instance : null;
    }

    /// <summary>카메라 관리자에 추가된 T 타입 C# 모디파이어 중 첫 번째를 찾습니다.</summary>
    internal static T? FindModifierOf<T>(PlayerCameraManager CameraManager) where T : ScriptCameraModifier {
        foreach (ScriptCameraModifier Instance in sModifiers.Values) {
            if (Instance is T Found && Instance.Modifier.GetCameraOwner() is { } Owner && Owner.Handle.Equals(CameraManager.Handle)) {
                return Found;
            }
        }

        return null;
    }

    /// <summary>유효한 GCHandle에서 카메라 모디파이어 인스턴스를 조회합니다.</summary>
    private static ScriptCameraModifier GetModifier(nint Handle) => (ScriptCameraModifier)GCHandle.FromIntPtr(Handle).Target!;

    /// <summary>이름으로 C# 모디파이어를 만들어 네이티브 래퍼에 연결하며 실패하면 0을 반환합니다.</summary>
    [UnmanagedCallersOnly]
    private static nint CreateModifier(byte* NameUtf8, ObjectHandle Modifier) {
        try {
            string Name = Marshal.PtrToStringUTF8((nint)NameUtf8)!;

            if (sAssembly?.GetType(Name) is not { } Type || !Type.IsAssignableTo(typeof(ScriptCameraModifier)) || Type.IsAbstract) {
                Log.Info($"C# camera modifier class not found: {Name}");
                return 0;
            }

            var Instance = (ScriptCameraModifier)Activator.CreateInstance(Type)!;
            Instance.Modifier = (CameraModifier)Bindings.Wrap(Modifier)!;
            sModifiers[Modifier] = Instance;

            try {
                Instance.AddedToCamera();
            } catch (Exception Exception) {
                Instance.mFaulted = true;
                Log.Info($"C# camera modifier disabled by an exception: {Exception}");
            }

            return GCHandle.ToIntPtr(GCHandle.Alloc(Instance));
        } catch (Exception Exception) {
            Log.Info($"C# camera modifier could not be created: {Exception}");
            return 0;
        }
    }

    /// <summary>공유 ABI 시점을 C# 모디파이어에 전달하며 후속 적용 중단 요청이면 1을 반환합니다.</summary>
    [UnmanagedCallersOnly]
    private static int ModifyCamera(nint Handle, float DeltaTime, MinimalViewInfo* View) {
        ScriptCameraModifier Instance = GetModifier(Handle);

        if (Instance.mFaulted) {
            return 0;
        }

        try {
            return Instance.ModifyCamera(DeltaTime, ref *View) ? 1 : 0;
        } catch (Exception Exception) {
            Instance.mFaulted = true;
            Log.Info($"C# camera modifier disabled by an exception: {Exception}");
            return 0;
        }
    }

    /// <summary>모디파이어 인스턴스를 등록 목록에서 빼고 GCHandle의 소유 참조를 해제합니다.</summary>
    [UnmanagedCallersOnly]
    private static void DestroyModifier(nint Handle) {
        ScriptCameraModifier Instance = GetModifier(Handle);

        sModifiers.Remove(Instance.Modifier.Handle);
        GCHandle.FromIntPtr(Handle).Free();
    }

    /// <summary>액터별 목록에서 스크립트를 제거하고 GCHandle의 소유 참조를 해제합니다.</summary>
    [UnmanagedCallersOnly]
    private static void DestroyInstance(nint Handle) {
        ScriptComponent Instance = GetInstance(Handle);

        if (sScripts.TryGetValue(Instance.Owner.Handle, out List<ScriptComponent>? Scripts)) {
            Scripts.Remove(Instance);

            if (Scripts.Count == 0) {
                sScripts.Remove(Instance.Owner.Handle);
            }
        }

        GCHandle.FromIntPtr(Handle).Free();
    }

    // 다른 프로세스가 사용 중인 폴더는 지울 수 없으므로 실패를 무시합니다.

    /// <summary>언로드한 섀도 복사 디렉터리를 정리하며 파일 사용·권한 오류는 무시합니다.</summary>
    private static void DeleteDirectory(string? Directory) {
        try {
            if (Directory != null && System.IO.Directory.Exists(Directory)) {
                System.IO.Directory.Delete(Directory, true);
            }
        } catch (IOException) {
        } catch (UnauthorizedAccessException) {
        }
    }
}
