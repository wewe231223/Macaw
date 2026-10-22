/**
 * @file EditorEngine.cpp
 * @brief 에디터 UI, 입력과 장면 파일 편집을 관리합니다.
 * @ingroup Editor
 */

#include "Editor/EditorEngine.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/AActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/UWorld.h"
#include "Engine/FWorldContext.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/FPropertyChangedEvent.h"
#include "CoreUObject/UObject.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FTransform.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FSceneView.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FViewportInput.h"
#include "Engine/FSceneViewport.h"
#include "RenderCore/IRendererModule.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Core/Base/ErrorHandler.h"
#include "Editor/ViewportClient.h"
#include "Engine/SceneSerializer.h"

#include <d3d11.h>
#include <commdlg.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <algorithm>
#include <array>
#include <sstream>

/**
 * @brief ImGui Win32 백엔드의 외부 메시지 처리 진입점입니다.
 * @param[in] Window 메시지를 받은 Win32 창입니다.
 * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
 * @param[in] WParam Windows 메시지의 WPARAM 인수입니다.
 * @param[in] LParam Windows 메시지의 LPARAM 인수입니다.
 * @return 연산을 적용한 새 값입니다.
 */
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 파일시스템 경로를 UI에 표시할 UTF-8 문자열로 변환합니다.
     * @param[in] Path 읽거나 쓸 파일 경로입니다.
     * @return 연산을 적용한 새 값입니다.
     */
    FString PathText(const std::filesystem::path& Path) {
        const auto Text{Path.u8string()};
        return FString{reinterpret_cast<const char*>(Text.data()), Text.size()};
    }
}

/**
 * @brief UEditorEngine의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UEditorEngine::UEditorEngine()
	: mClient{*this},
	  mOutlinerPanel{*this},
	  mDetailsPanel{*this},
	  mConsolePanel{*this},
	  mStatisticsPanel{*this} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UEditorEngine::~UEditorEngine() {
    Shutdown();
}

/**
 * @brief 엔진, ImGui 백엔드, 뷰포트와 초기 장면을 준비합니다.
 */
bool UEditorEngine::InitializeEditor(IRendererModule& Renderer, FWindowsApplication& Platform) {
    if (!UEngine::Initialize(Renderer)) {
        return false;
    }

    StaticClass();
    mPlatform = &Platform;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    mContextCreated = true;

    ImGuiIO& Input{ImGui::GetIO()};
    Input.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

    mIniPath = "EditorLayout.ini";
    Input.IniFilename = mIniPath.c_str();
    Input.LogFilename = nullptr;

    if (std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf")) {
        Input.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f);
    } else {
        Input.Fonts->AddFontDefault();
    }

    ImGui::StyleColorsDark();
    ImGuiStyle& Style{ImGui::GetStyle()};
    Style.WindowRounding = 3.0f;
    Style.FrameRounding = 3.0f;
    Style.GrabRounding = 3.0f;

    mWin32Ready = ImGui_ImplWin32_Init(Platform.GetWindowHandle());
    mDxReady = mWin32Ready && ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(Renderer.GetNativeDevice()), static_cast<ID3D11DeviceContext*>(Renderer.GetNativeContext()));

    if (!mDxReady) {
        Shutdown();

        return false;
    }

    Platform.SetMessageHandler([](HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
        return ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam) != 0;
    });

    mViewport = std::make_unique<FSceneViewport>(Renderer, mClient);

    if (!mViewport->Resize(960, 640)) {
        Shutdown();

        return false;
    }

    AStaticMeshActor* Cube{GetWorld()->SpawnPrimitive(EPrimitiveType::Cube)};
    AStaticMeshActor* Sphere{GetWorld()->SpawnPrimitive(EPrimitiveType::Sphere)};
    AStaticMeshActor* Plane{GetWorld()->SpawnPrimitive(EPrimitiveType::Plane)};

    if (Cube == nullptr || Sphere == nullptr || Plane == nullptr) {
        Shutdown();

        return false;
    }

    Sphere->SetActorTransform(FTransform{{0.0f, 2.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}});
    Plane->SetActorTransform(FTransform{{0.0f, 0.0f, -0.55f}, {}, {12.0f, 12.0f, 1.0f}});
    SelectActor(Cube->GetHandle());

    SynchronizeWorld();

    mInitialized = true;
    mDirty = false;

    return true;
}

/**
 * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
 */
void UEditorEngine::Shutdown() {
    mClient.GetGizmo().Cancel();
    mSelectedActor = {};
    mSelectedObject = {};

    if (mPlatform != nullptr) {
        mPlatform->SetMessageHandler({});
    }

    mViewport.reset();

    if (mDxReady) {
        ImGui_ImplDX11_Shutdown();
        mDxReady = false;
    }

    if (mWin32Ready) {
        ImGui_ImplWin32_Shutdown();

        mWin32Ready = false;
    }

    if (mContextCreated) {
        ImGui::DestroyContext();
        mContextCreated = false;
    }

    mInitialized = false;
    mPlatform = nullptr;

    UEngine::Shutdown();
}

/**
 * @brief 입력을 수집하고 패널, 월드 동기화와 에디터 프레임 렌더링을 진행합니다.
 */
void UEditorEngine::TickFrame(float DeltaTime) {
    if (!mInitialized) {
        return;
    }

    mDeltaTime = DeltaTime;
    GetRenderer()->BeginFrame(mPlatform->GetWidth(), mPlatform->GetHeight());

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawMenu();
    BuildLayout();
    mOutlinerPanel.Draw(DeltaTime);
    mDetailsPanel.Draw(DeltaTime);
    DrawViewport();
    mConsolePanel.Draw(DeltaTime);

    SynchronizeWorld();
    mViewport->Draw();

    mStatisticsPanel.Draw(DeltaTime);

    ImGui::Render();
    GetRenderer()->BeginUiRender();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

/**
 * @brief 지정한 기본 메시 종류의 액터를 생성하여 영속 레벨에 추가합니다.
 */
AStaticMeshActor* UEditorEngine::SpawnPrimitive(EPrimitiveType Type) {
    if (GetWorld() == nullptr) {
        return nullptr;
    }

    AStaticMeshActor* Actor{GetWorld()->SpawnPrimitive(Type)};

    if (Actor != nullptr) {
        const FSceneView View{mClient.CalcSceneView({960.0f, 640.0f})};
        Actor->SetActorTransform(FTransform{View.mLocation + View.mForward * 6.0f, {}, {1.0f, 1.0f, 1.0f}});

        SelectActor(Actor->GetHandle());
        mDirty = true;

        AddLog("Spawned " + Actor->GetName());
    }

    return Actor;
}

/**
 * @brief 현재 선택한 액터를 제거하고 관련 선택 상태를 초기화합니다.
 */
bool UEditorEngine::DestroySelectedActor() {
    AActor* Actor{GetSelectedActor()};

    if (Actor == nullptr || GetWorld() == nullptr) {
        return false;
    }

    const FObjectHandle Handle{Actor->GetHandle()};
    const FString Name{Actor->GetName()};

    mClient.GetGizmo().Cancel();
    ClearSelection();

    if (!GetWorld()->DestroyActor(Handle)) {
        return false;
    }

    mDirty = true;
    AddLog("Deleted " + Name);

    return true;
}

/**
 * @brief 유효한 액터 핸들을 선택하고 프로퍼티 편집 대상으로 지정합니다.
 */
void UEditorEngine::SelectActor(FObjectHandle Handle) {
    if (GetWorldContext() == nullptr) {
        return;
    }

    AActor* Actor{Cast<AActor>(GetWorldContext()->GetRegistry().Resolve(Handle))};

    if (Actor == nullptr || Actor->GetWorld() != GetWorld()) {
        ClearSelection();
        return;
    }

    if (!(mSelectedActor == Handle)) {
        mClient.GetGizmo().Cancel();
    }

    mSelectedActor = Handle;
    mSelectedObject = Actor->GetRootComponent() != nullptr ? Actor->GetRootComponent()->GetHandle() : Handle;
}

/**
 * @brief 액터와 객체 선택 핸들을 모두 무효화합니다.
 */
void UEditorEngine::ClearSelection() {
    mSelectedActor = {};
    mSelectedObject = {};
}

/**
 * @brief 선택한 액터의 세대 핸들을 반환합니다.
 */
AActor* UEditorEngine::GetSelectedActor() const {
    return GetWorldContext() != nullptr ? Cast<AActor>(GetWorldContext()->GetRegistry().Resolve(mSelectedActor)) : nullptr;
}

/**
 * @brief 프로퍼티 편집 대상의 세대 핸들을 반환합니다.
 */
UObject* UEditorEngine::GetSelectedObject() const {
    return GetWorldContext() != nullptr ? GetWorldContext()->GetRegistry().Resolve(mSelectedObject) : nullptr;
}

/**
 * @brief 레지스트리에서 유효한 객체를 확인한 뒤 편집 대상으로 선택합니다.
 */
void UEditorEngine::SelectObject(FObjectHandle Handle) {
    if (GetWorldContext() != nullptr && GetWorldContext()->GetRegistry().Resolve(Handle) != nullptr) {
        mSelectedObject = Handle;
    }
}

/**
 * @brief 프로퍼티 소속과 편집 권한을 확인하고 변경 전후 알림과 함께 값을 씁니다.
 */
bool UEditorEngine::EditProperty(UObject& Object, const FProperty& Property, const FPropertyValue& Value, bool Interactive) {
    if (GetWorldContext() == nullptr || Object.GetRegistry() != &GetWorldContext()->GetRegistry() || Object.GetClass()->FindClassProperty(Property.GetName()) != &Property || !Property.HasFlag(EPropertyFlags::Edit) || Property.HasFlag(EPropertyFlags::ReadOnly)) {
        return false;
    }

    Object.PreEditChange(Property);

    if (!Property.WriteValue(&Object, Value)) {
        AddLog("Rejected invalid property value: " + Property.GetName());

        return false;
    }

    Object.PostEditChangeProperty(FPropertyChangedEvent{&Property, Interactive});
    mDirty = true;

    return true;
}

/**
 * @brief 액터 루트의 변환 프로퍼티를 에디터 편집 경로로 변경합니다.
 */
bool UEditorEngine::ApplyTransform(AActor& Actor, const FTransform& Transform, bool Interactive) {
    USceneComponent* Root{Actor.GetRootComponent()};
    const FProperty* Property{Root != nullptr ? Root->GetClass()->FindClassProperty("Transform") : nullptr};

    return Property != nullptr && EditProperty(*Root, *Property, FPropertyValue{Transform}, Interactive);
}

/**
 * @brief 현재 월드를 지정한 장면 파일로 저장하고 성공하면 변경 표시를 해제합니다.
 */
bool UEditorEngine::SaveScene(const std::filesystem::path& Path) {
    if (GetWorldContext() == nullptr || Path.empty()) {
        return false;
    }

    const std::filesystem::path ScenePath{std::filesystem::absolute(Path.is_absolute() ? Path : std::filesystem::path{"Scenes"} / Path)};
    FString Error{};

    if (!FSceneSerializer::Save(ScenePath, *GetWorldContext(), Error)) {
        ErrorHandler::Fatal("Scene save", Error);
    }

    mScenePath = ScenePath;
    mDirty = false;

    AddLog("Saved " + PathText(ScenePath));

    return true;
}

/**
 * @brief 장면 파일을 새 컨텍스트에 복원한 뒤 현재 월드와 선택 상태를 교체합니다.
 */
bool UEditorEngine::LoadScene(const std::filesystem::path& Path) {
    if (Path.empty()) {
        return false;
    }

    const std::filesystem::path ScenePath{std::filesystem::absolute(Path.is_absolute() ? Path : std::filesystem::path{"Scenes"} / Path)};
    FString Error{};
    auto Context{FSceneSerializer::Load(ScenePath, Error)};

    if (Context == nullptr) {
        ErrorHandler::Fatal("Scene load", Error);
    }

    mClient.GetGizmo().Cancel();

    if (!ReplaceWorld(std::move(Context))) {
        ErrorHandler::Fatal("Scene load", "Scene allocation failed");
    }

    ClearSelection();
    mScenePath = ScenePath;
    mDirty = false;

    AddLog("Loaded " + PathText(ScenePath));

    return true;
}

/**
 * @brief 빈 월드 컨텍스트로 교체하고 선택 및 장면 파일 경로를 초기화합니다.
 */
bool UEditorEngine::NewScene() {
    mClient.GetGizmo().Cancel();

    if (!ReplaceWorld(std::make_unique<FWorldContext>())) {
        return false;
    }

    ClearSelection();
    mScenePath.clear();
    mDirty = false;

    return true;
}

/**
 * @brief 콘솔 패널에 메시지를 추가하고 스크롤 갱신을 요청합니다.
 */
void UEditorEngine::AddLog(const FString& Message) {
    mConsolePanel.AddLog(Message);
    OutputDebugStringA((Message + "\n").c_str());
}

/**
 * @brief 입력한 콘솔 명령을 해석하여 에디터 동작을 실행합니다.
 */
void UEditorEngine::ExecuteConsoleCommand(const FString& Command) {
    AddLog("> " + Command);
    std::istringstream Input{Command};
    FString Verb{};

    Input >> Verb;

    if (Verb == "clear") {
        mConsolePanel.Clear();
    } else if (Verb == "help") {
        AddLog("help | clear | spawn plane/cube/sphere | save <path.json> | load <path.json>");
    } else if (Verb == "spawn") {
        FString Type{};
        Input >> Type;

        if (Type == "plane" || Type == "cube" || Type == "sphere") {
            SpawnPrimitive(Type == "plane" ? EPrimitiveType::Plane : Type == "sphere" ? EPrimitiveType::Sphere : EPrimitiveType::Cube);
        } else {
            AddLog("Expected plane, cube or sphere.");
        }
    } else if (Verb == "save" || Verb == "load") {
        FString Text{};
        std::getline(Input >> std::ws, Text);
        const std::filesystem::path Path{Text.empty() ? mScenePath : std::filesystem::path{std::u8string_view{reinterpret_cast<const char8_t*>(Text.data()), Text.size()}}};

        if (Path.empty()) {
            AddLog("Specify a scene path.");
        } else if (Verb == "save") {
            SaveScene(Path);
        } else {
            LoadScene(Path);
        }
    } else if (!Verb.empty()) {
        AddLog("Unknown command.");
    }
}

/**
 * @brief 에디터 뷰포트 클라이언트를 반환합니다.
 */
FEditorViewportClient& UEditorEngine::GetViewportClient() {
    return mClient;
}

/**
 * @brief 에디터 뷰포트 렌더 타깃의 비소유 주소를 반환합니다.
 */
FRenderTarget* UEditorEngine::GetViewportTarget() const {
    return mViewport != nullptr ? mViewport->GetRenderTarget() : nullptr;
}

/**
 * @brief 저장 후 변경되어 저장이 필요한 장면인지 검사합니다.
 */
bool UEditorEngine::IsSceneDirty() const {
    return mDirty;
}

/**
 * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
 */
bool UEditorEngine::WantsExit() const {
    return mExit;
}

/**
 * @brief 장면 로드 또는 저장용 Windows 파일 대화상자를 표시합니다.
 */
std::filesystem::path UEditorEngine::ChooseFile(bool Save) {
    std::array<wchar_t, 4096> Buffer{};

    if (Save) {
        const std::wstring Filename{mScenePath.empty() ? L"Scene.json" : mScenePath.filename().wstring()};
        std::copy_n(Filename.begin(), std::min(Filename.size(), Buffer.size() - 1), Buffer.begin());
    }

    const std::filesystem::path Directory{std::filesystem::absolute("Scenes")};

    std::error_code Error{};
    std::filesystem::create_directories(Directory, Error);

    OPENFILENAMEW Dialog{};
    Dialog.lStructSize = sizeof(Dialog);
    Dialog.hwndOwner = mPlatform->GetWindowHandle();
    Dialog.lpstrFilter = L"Scene JSON (*.json)\0*.json\0All files (*.*)\0*.*\0";
    Dialog.lpstrFile = Buffer.data();
    Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
    Dialog.lpstrInitialDir = Directory.c_str();
    Dialog.lpstrDefExt = L"json";
    Dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | (Save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);

    if (Save ? GetSaveFileNameW(&Dialog) : GetOpenFileNameW(&Dialog)) {
        return std::filesystem::path{Buffer.data()};
    }

    return {};
}

/**
 * @brief 에디터 패널의 초기 ImGui 도킹 레이아웃을 구성합니다.
 */
void UEditorEngine::BuildLayout() {
    const ImGuiID Dockspace{ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport())};

    if (ImGui::DockBuilderGetNode(Dockspace) != nullptr && ImGui::DockBuilderGetNode(Dockspace)->ChildNodes[0] == nullptr && ImGui::DockBuilderGetNode(Dockspace)->ChildNodes[1] == nullptr) {
        ImGui::DockBuilderRemoveNode(Dockspace);
        ImGui::DockBuilderAddNode(Dockspace, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(Dockspace, ImGui::GetMainViewport()->WorkSize);

        ImGuiID Center{Dockspace};
        const ImGuiID Left{ImGui::DockBuilderSplitNode(Center, ImGuiDir_Left, 0.19f, nullptr, &Center)};
        const ImGuiID Right{ImGui::DockBuilderSplitNode(Center, ImGuiDir_Right, 0.32f, nullptr, &Center)};
        const ImGuiID Bottom{ImGui::DockBuilderSplitNode(Center, ImGuiDir_Down, 0.24f, nullptr, &Center)};

        ImGuiID Outliner{Left};
        const ImGuiID Statistics{ImGui::DockBuilderSplitNode(Outliner, ImGuiDir_Down, 0.42f, nullptr, &Outliner)};

        ImGui::DockBuilderDockWindow(mOutlinerPanel.GetName().c_str(), Outliner);
        ImGui::DockBuilderDockWindow(mStatisticsPanel.GetName().c_str(), Statistics);
        ImGui::DockBuilderDockWindow(mDetailsPanel.GetName().c_str(), Right);
        ImGui::DockBuilderDockWindow(mConsolePanel.GetName().c_str(), Bottom);
        ImGui::DockBuilderDockWindow("Viewport", Center);

        ImGui::DockBuilderFinish(Dockspace);
    }
}

/**
 * @brief 장면 파일, 프리미티브 생성과 패널 표시 메뉴를 그립니다.
 */
void UEditorEngine::DrawMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                NewScene();
            }

            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                const auto Path{ChooseFile(false)};

                if (!Path.empty()) {
                    LoadScene(Path);
                }
            }

            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                const auto Path{mScenePath.empty() ? ChooseFile(true) : mScenePath};

                if (!Path.empty()) {
                    SaveScene(Path);
                }
            }

            if (ImGui::MenuItem("Save Scene As...")) {
                const auto Path{ChooseFile(true)};

                if (!Path.empty()) {
                    SaveScene(Path);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                mExit = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem(mConsolePanel.GetName().c_str(), nullptr, mConsolePanel.IsVisible())) {
                mConsolePanel.SetVisible(!mConsolePanel.IsVisible());
            }

            if (ImGui::MenuItem(mStatisticsPanel.GetName().c_str(), nullptr, mStatisticsPanel.IsVisible())) {
                mStatisticsPanel.SetVisible(!mStatisticsPanel.IsVisible());
            }

            ImGui::EndMenu();
        }

        ImGui::Separator();

        ImGui::TextDisabled("%s%s", mScenePath.empty() ? "Untitled" : PathText(mScenePath.filename()).c_str(), mDirty ? " *" : "");
        ImGui::EndMainMenuBar();
    }

    const ImGuiIO& Input{ImGui::GetIO()};

    if (Input.KeyCtrl && !Input.WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_S)) {
            const auto Path{mScenePath.empty() ? ChooseFile(true) : mScenePath};

            if (!Path.empty()) {
                SaveScene(Path);
            }
        } else if (ImGui::IsKeyPressed(ImGuiKey_O)) {
            const auto Path{ChooseFile(false)};

            if (!Path.empty()) {
                LoadScene(Path);
            }
        } else if (ImGui::IsKeyPressed(ImGuiKey_N)) {
            NewScene();
        }
    }
}

/**
 * @brief 장면 타깃을 표시하고 뷰포트 입력 및 크기 변경을 전달합니다.
 */
void UEditorEngine::DrawViewport() {
    if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs)) {
        FTransformGizmo& Gizmo{mClient.GetGizmo()};
        const std::array<const char*, 3> Modes{"Translate", "Rotation", "Scale"};

        for (Int32 Index{}; Index < 3; ++Index) {
            if (Index > 0) {
                ImGui::SameLine();
            }

            if (ImGui::Selectable(Modes[Index], Gizmo.GetMode() == static_cast<EGizmoMode>(Index), 0, {90.0f, 0.0f})) {
                Gizmo.SetMode(static_cast<EGizmoMode>(Index));
            }
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(Gizmo.GetMode() == EGizmoMode::Scale);

        if (ImGui::Button(Gizmo.GetSpace() == EGizmoSpace::World ? "World" : "Local")) {
            Gizmo.SetSpace(Gizmo.GetSpace() == EGizmoSpace::World ? EGizmoSpace::Local : EGizmoSpace::World);
        }

        ImGui::EndDisabled();
        ImGui::SameLine();

        if (ImGui::Button("Reset Camera")) {
            mClient.ResetCamera();
        }

        const ImVec2 Available{ImGui::GetContentRegionAvail()};

        if (Available.x >= 1.0f && Available.y >= 1.0f && mViewport->Resize(static_cast<Uint32>(Available.x), static_cast<Uint32>(Available.y))) {
            FRenderTarget* Target{mViewport->GetRenderTarget()};
            ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Target->GetNativeTexture()))}, Available);

            const ImVec2 Origin{ImGui::GetItemRectMin()};
            const ImGuiIO& Input{ImGui::GetIO()};
            FViewportInput State{};
            const float ScaleX{Target->GetWidth() / Available.x};
            const float ScaleY{Target->GetHeight() / Available.y};

            State.mMouse = {(Input.MousePos.x - Origin.x) * ScaleX, (Input.MousePos.y - Origin.y) * ScaleY};
            State.mDelta = {Input.MouseDelta.x * ScaleX, Input.MouseDelta.y * ScaleY};

            State.mHovered = ImGui::IsItemHovered();
            State.mFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

            State.mLeftPressed = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            State.mLeftDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            State.mLeftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
            State.mRightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
            State.mMiddleDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

            State.mAlt = Input.KeyAlt;
            State.mWheel = Input.MouseWheel;
            State.mEscape = ImGui::IsKeyPressed(ImGuiKey_Escape);

            const bool Keyboard{State.mFocused && !Input.WantTextInput && !Input.KeyCtrl && !Input.KeyAlt};
            State.mDelete = Keyboard && ImGui::IsKeyPressed(ImGuiKey_Delete, false);
            State.mCycleGizmo = Keyboard && ImGui::IsKeyPressed(ImGuiKey_Space, false);
            State.mFocusSelection = Keyboard && ImGui::IsKeyPressed(ImGuiKey_F, false);

            State.mForward = Keyboard && ImGui::IsKeyDown(ImGuiKey_W);
            State.mBackward = Keyboard && ImGui::IsKeyDown(ImGuiKey_S);
            State.mLeft = Keyboard && ImGui::IsKeyDown(ImGuiKey_A);
            State.mRight = Keyboard && ImGui::IsKeyDown(ImGuiKey_D);
            State.mUp = Keyboard && ImGui::IsKeyDown(ImGuiKey_E);
            State.mDown = Keyboard && ImGui::IsKeyDown(ImGuiKey_Q);
            State.mFast = Input.KeyShift;

            mViewport->Input(State, mDeltaTime);
        }
    }

    ImGui::End();
}
