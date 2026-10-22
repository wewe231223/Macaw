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
#include "CoreUObject/Reflection/ReflectionMacros.h"
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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

namespace {
    FString PathText(const std::filesystem::path& Path) {
        const auto Text{Path.u8string()};
        return FString{reinterpret_cast<const char*>(Text.data()), Text.size()};
    }
}

UEditorEngine::UEditorEngine()
	: mClient{*this},
	  mOutlinerPanel{*this},
	  mDetailsPanel{*this},
	  mConsolePanel{*this},
	  mStatisticsPanel{*this} {
}

UEditorEngine::~UEditorEngine() {
    Shutdown();
}

ImplementReflection(UEditorEngine, EClassFlags::NoFactory)

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

    std::error_code Error{};
    std::filesystem::create_directories("Saved/Week1", Error);
    mIniPath = "Saved/Week1/EditorLayout.ini";
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

void UEditorEngine::ClearSelection() {
    mSelectedActor = {};
    mSelectedObject = {};
}

AActor* UEditorEngine::GetSelectedActor() const {
    return GetWorldContext() != nullptr ? Cast<AActor>(GetWorldContext()->GetRegistry().Resolve(mSelectedActor)) : nullptr;
}

UObject* UEditorEngine::GetSelectedObject() const {
    return GetWorldContext() != nullptr ? GetWorldContext()->GetRegistry().Resolve(mSelectedObject) : nullptr;
}

void UEditorEngine::SelectObject(FObjectHandle Handle) {
    if (GetWorldContext() != nullptr && GetWorldContext()->GetRegistry().Resolve(Handle) != nullptr) {
        mSelectedObject = Handle;
    }
}

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

bool UEditorEngine::ApplyTransform(AActor& Actor, const FTransform& Transform, bool Interactive) {
    USceneComponent* Root{Actor.GetRootComponent()};
    const FProperty* Property{Root != nullptr ? Root->GetClass()->FindClassProperty("Transform") : nullptr};

    return Property != nullptr && EditProperty(*Root, *Property, FPropertyValue{Transform}, Interactive);
}

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

void UEditorEngine::AddLog(const FString& Message) {
    mConsolePanel.AddLog(Message);
    OutputDebugStringA((Message + "\n").c_str());
}

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

FEditorViewportClient& UEditorEngine::GetViewportClient() {
    return mClient;
}

FRenderTarget* UEditorEngine::GetViewportTarget() const {
    return mViewport != nullptr ? mViewport->GetRenderTarget() : nullptr;
}

bool UEditorEngine::IsSceneDirty() const {
    return mDirty;
}

bool UEditorEngine::WantsExit() const {
    return mExit;
}

std::filesystem::path UEditorEngine::ChooseFile(bool Save) {
    std::array<wchar_t, 4096> Buffer{};

    if (Save) {
        const std::wstring Filename{mScenePath.empty() ? L"Week1.json" : mScenePath.filename().wstring()};
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
