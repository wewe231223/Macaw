using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Macaw;

/// <summary>사용자 스크립트를 언로드할 수 있도록 담는 수집형 로드 컨텍스트입니다.</summary>
internal sealed class ScriptLoadContext : AssemblyLoadContext {
    private readonly AssemblyDependencyResolver mResolver;

    public ScriptLoadContext(string MainAssembly) : base("GameScripts", isCollectible: true) {
        mResolver = new AssemblyDependencyResolver(MainAssembly);
    }

    // null을 돌려주면 기본 컨텍스트로 해석되므로 Macaw.Engine과 프레임워크 타입을 공유합니다.
    protected override Assembly? Load(AssemblyName Name) {
        if (Name.Name == typeof(Host).Assembly.GetName().Name) {
            return null;
        }

        string? Path = mResolver.ResolveAssemblyToPath(Name);

        return Path != null ? LoadFromAssemblyPath(Path) : null;
    }

    protected override nint LoadUnmanagedDll(string Name) {
        string? Path = mResolver.ResolveUnmanagedDllToPath(Name);

        return Path != null ? LoadUnmanagedDllFromPath(Path) : 0;
    }
}

/// <summary>엔진이 호출하는 진입점입니다. 예외가 네이티브로 넘어가면 프로세스가 종료되므로 모든 진입점에서 잡습니다.</summary>
internal static unsafe class Host {
    private const string ShadowFolder = ".Loaded";

    private static ScriptLoadContext? sContext;
    private static Assembly? sAssembly;
    private static string? sShadowDirectory;
    private static int sGeneration;
    private static readonly Dictionary<ObjectHandle, List<ScriptComponent>> sScripts = new();

    /// <summary>액터에 붙은 살아 있는 스크립트 목록을 반환합니다.</summary>
    internal static IReadOnlyList<ScriptComponent> GetScripts(ObjectHandle Owner) {
        return sScripts.TryGetValue(Owner, out List<ScriptComponent>? Scripts) ? Scripts : Array.Empty<ScriptComponent>();
    }

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

    private static void CallWithUtf8(string Text, Action<nint> Call) {
        byte[] Bytes = System.Text.Encoding.UTF8.GetBytes(Text + "\0");

        fixed (byte* Pointer = Bytes) {
            Call((nint)Pointer);
        }
    }

    private static void AddClass(string Name) {
        CallWithUtf8(Name, Text => EngineObject.sApi->CatalogAddClass((byte*)Text));
    }

    private static void AddField(string Name, string Tag, string Value) {
        CallWithUtf8(Name, NameText => CallWithUtf8(Tag, TagText => CallWithUtf8(Value, ValueText => EngineObject.sApi->CatalogAddField((byte*)NameText, (byte*)TagText, (byte*)ValueText))));
    }

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

    private static ScriptComponent GetInstance(nint Handle) => (ScriptComponent)GCHandle.FromIntPtr(Handle).Target!;

    /// <summary>예외가 발생한 인스턴스를 기록 후 비활성화합니다.</summary>
    private static void Disable(ScriptComponent Instance, Exception Exception) {
        Instance.mFaulted = true;

        try {
            Log.Info($"C# script disabled by an exception: {Exception}");
        } catch {
        }
    }

    [UnmanagedCallersOnly]
    private static void BeginPlay(nint Handle) {
        ScriptComponent Instance = GetInstance(Handle);

        try {
            Instance.BeginPlay();
        } catch (Exception Exception) {
            Disable(Instance, Exception);
        }
    }

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

    private static ScriptCameraModifier GetModifier(nint Handle) => (ScriptCameraModifier)GCHandle.FromIntPtr(Handle).Target!;

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

    [UnmanagedCallersOnly]
    private static void DestroyModifier(nint Handle) {
        ScriptCameraModifier Instance = GetModifier(Handle);

        sModifiers.Remove(Instance.Modifier.Handle);
        GCHandle.FromIntPtr(Handle).Free();
    }

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
