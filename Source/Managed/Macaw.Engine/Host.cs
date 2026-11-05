using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Macaw;

/// <summary>사용자 스크립트를 언로드할 수 있도록 담는 수집형 로드 컨텍스트입니다.</summary>
internal sealed class ScriptLoadContext : AssemblyLoadContext {
    public ScriptLoadContext() : base("GameScripts", isCollectible: true) { }

    // null을 돌려주면 Macaw.Engine 참조가 기본 컨텍스트로 해석되어 타입을 공유합니다.
    protected override Assembly? Load(AssemblyName Name) => null;
}

/// <summary>엔진이 호출하는 진입점입니다. 예외가 네이티브로 넘어가면 프로세스가 종료되므로 모든 진입점에서 잡습니다.</summary>
internal static unsafe class Host {
    private const string ShadowFolder = ".Loaded";

    private static ScriptLoadContext? sContext;
    private static Assembly? sAssembly;
    private static string? sShadowDirectory;
    private static int sGeneration;

    [UnmanagedCallersOnly]
    public static int Bootstrap(NativeApi* Native, ManagedApi* Managed) {
        if (Native->Size != sizeof(NativeApi) || Managed->Size != sizeof(ManagedApi)) {
            return 0;
        }

        Log.sApi = Native;
        Managed->LoadScripts = &LoadScripts;
        Managed->UnloadScripts = &UnloadScripts;
        Managed->CreateInstance = &CreateInstance;
        Managed->Tick = &Tick;
        Managed->DestroyInstance = &DestroyInstance;

        return 1;
    }

    /// <summary>빌드 중 덮어쓸 수 있도록 어셈블리를 섀도 복사한 뒤 로드합니다.</summary>
    [UnmanagedCallersOnly]
    private static int LoadScripts(byte* PathUtf8) {
        try {
            string Source = Path.GetFullPath(Marshal.PtrToStringUTF8((nint)PathUtf8)!);
            string Root = Path.Combine(Path.GetDirectoryName(Source)!, ShadowFolder);

            if (sGeneration == 0) {
                DeleteDirectory(Root);
            }

            string Directory = Path.Combine(Root, $"{Environment.ProcessId}_{++sGeneration}");
            System.IO.Directory.CreateDirectory(Directory);

            string Copy = Path.Combine(Directory, Path.GetFileName(Source));
            File.Copy(Source, Copy, true);

            string Symbols = Path.ChangeExtension(Source, ".pdb");

            if (File.Exists(Symbols)) {
                File.Copy(Symbols, Path.ChangeExtension(Copy, ".pdb"), true);
            }

            var Context = new ScriptLoadContext();
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
    private static nint CreateInstance(byte* NameUtf8) {
        try {
            string Name = Marshal.PtrToStringUTF8((nint)NameUtf8)!;

            if (sAssembly?.GetType(Name) is not { } Type || !Type.IsAssignableTo(typeof(ScriptComponent))) {
                Log.Info($"C# script class not found: {Name}");
                return 0;
            }

            return GCHandle.ToIntPtr(GCHandle.Alloc(Activator.CreateInstance(Type)!));
        } catch (Exception Exception) {
            Log.Info($"C# script could not be created: {Exception}");
            return 0;
        }
    }

    /// <summary>예외가 발생한 인스턴스는 기록 후 비활성화합니다.</summary>
    [UnmanagedCallersOnly]
    private static void Tick(nint Handle, float DeltaTime) {
        var Instance = (ScriptComponent)GCHandle.FromIntPtr(Handle).Target!;

        if (Instance.mFaulted) {
            return;
        }

        try {
            Instance.Tick(DeltaTime);
        } catch (Exception Exception) {
            Instance.mFaulted = true;

            try {
                Log.Info($"C# script disabled by an exception: {Exception}");
            } catch {
            }
        }
    }

    [UnmanagedCallersOnly]
    private static void DestroyInstance(nint Handle) {
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
