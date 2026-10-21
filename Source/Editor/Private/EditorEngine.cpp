#include "Editor/EditorEngine.h"
#include "Engine/ULevel.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/AActor.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/UWorld.h"
#include "Engine/FWorldContext.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/FPropertyChangedEvent.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/Reflection/FEnumValue.h"
#include "CoreUObject/Reflection/EPropertyType.h"
#include "CoreUObject/Reflection/EPropertyFlags.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FTransform.h"
#include "Core/FMemoryStats.h"
#include "Engine/FMeshVertex.h"
#include "Engine/FMeshData.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FSceneView.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FViewportInput.h"
#include "Engine/FSceneViewport.h"
#include "RenderCore/FRenderStats.h"
#include "RenderCore/IRendererModule.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/PrimitiveMesh.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/Base/ErrorHandler.h"
#include "Editor/ViewportClient.h"
#include "Engine/SceneSerializer.h"
#include "Core/Memory.h"

#include <d3d11.h>
#include <commdlg.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <algorithm>
#include <array>
#include <format>
#include <sstream>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

namespace {
    FString PathText(const std::filesystem::path& Path) {
        const auto Text{Path.u8string()};
        return FString{reinterpret_cast<const char*>(Text.data()), Text.size()};
    }

    bool DrawPropertyValue(const FProperty& Property, FPropertyValue& Value) {
        const char* Label{Property.GetName().c_str()};

        if (Property.GetType() == EPropertyType::Enum) {
            Int32 Number{std::get<Int32>(Value)};
            const auto& Choices{Property.GetEnumValues()};
            const auto Current{std::find_if(Choices.begin(), Choices.end(), [Number](const FEnumValue& Choice) {
                return Choice.mValue == Number;
            })};
            bool Changed{};

            if (ImGui::BeginCombo(Label, Current != Choices.end() ? Current->mName.c_str() : "Invalid")) {
                for (const FEnumValue& Choice : Choices) {
                    if (ImGui::Selectable(Choice.mName.c_str(), Number == Choice.mValue)) {
                        Number = Choice.mValue;
                        Changed = true;
                    }
                }

                ImGui::EndCombo();
            }

            Value = Number;

            return Changed;
        }

        if (auto* Vector{std::get_if<FVector>(&Value)}) {
            float Values[]{Vector->mX, Vector->mY, Vector->mZ};

            if (ImGui::DragFloat3(Label, Values, 0.025f, 0.0f, 0.0f, "%.3f")) {
                *Vector = {Values[0], Values[1], Values[2]};

                return true;
            }
        } else if (auto* Rotation{std::get_if<FRotator>(&Value)}) {
            float Values[]{Rotation->mPitch, Rotation->mYaw, Rotation->mRoll};

            if (ImGui::DragFloat3(Label, Values, 0.25f, 0.0f, 0.0f, "%.2f")) {
                *Rotation = {Values[0], Values[1], Values[2]};

                return true;
            }
        } else if (auto* String{std::get_if<FString>(&Value)}) {
            std::array<char, 512> Buffer{};
            std::copy_n(String->begin(), std::min(String->size(), Buffer.size() - 1), Buffer.begin());

            if (ImGui::InputText(Label, Buffer.data(), Buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                *String = Buffer.data();

                return true;
            }
        } else if (auto* Boolean{std::get_if<bool>(&Value)}) {
            return ImGui::Checkbox(Label, Boolean);
        } else if (auto* FloatValue{std::get_if<float>(&Value)}) {
            return ImGui::DragFloat(Label, FloatValue, 0.025f);
        } else if (auto* IntegerValue{std::get_if<Int32>(&Value)}) {
            return ImGui::InputInt(Label, IntegerValue);
        } else if (auto* Guid{std::get_if<FGuid>(&Value)}) {
            ImGui::TextDisabled("%s", Label);
            ImGui::TextWrapped("%s", Guid->ToString().c_str());
        } else if (const UStruct* Struct{Property.GetStruct()}) {
            bool Changed{};

            if (ImGui::TreeNodeEx(Label, ImGuiTreeNodeFlags_DefaultOpen)) {
                for (const FProperty* Child : Struct->GetProperties()) {
                    FPropertyValue ChildValue{Child->ReadValue(GetPropertyData(Value))};

                    if (DrawPropertyValue(*Child, ChildValue)) {
                        Changed = Child->WriteValue(GetPropertyData(Value), ChildValue) || Changed;
                    }
                }

                ImGui::TreePop();
            }

            return Changed;
        }

        return false;
    }

    void DrawReflectedObject(UEditorEngine& Engine, UObject& Object) {
        ImGui::PushID(&Object);
        ImGui::TextDisabled("%s", Object.GetClass()->GetName().c_str());

        for (const FProperty* Property : Object.GetClass()->GetAllProperties()) {
            if (!Property->HasFlag(EPropertyFlags::Edit) && !Property->HasFlag(EPropertyFlags::ReadOnly)) {
                continue;
            }

            FPropertyValue Value{Property->ReadValue(&Object)};
            const bool ReadOnly{Property->HasFlag(EPropertyFlags::ReadOnly)};

            ImGui::BeginDisabled(ReadOnly);

            if (DrawPropertyValue(*Property, Value) && !ReadOnly) {
                Engine.EditProperty(Object, *Property, Value, ImGui::IsMouseDown(ImGuiMouseButton_Left));
            }

            ImGui::EndDisabled();
        }

        ImGui::PopID();
    }
}

struct UEditorEngine::FImplementation {
private:
    friend class UEditorEngine;

public:
    explicit FImplementation(UEditorEngine& Owner);

public:
    void DrawMenu();
    void DrawOutliner();
    void DrawDetails();
    void DrawViewport();
    void DrawConsole();
    void DrawStatistics();
    void BuildLayout();
    std::filesystem::path ChooseFile(bool Save);

private:
    UEditorEngine* mOwner{};
    FWindowsApplication* mPlatform{};

    FEditorViewportClient mClient;
    std::unique_ptr<FSceneViewport> mViewport{};

    FObjectHandle mSelectedActor{};
    FObjectHandle mSelectedObject{};

    std::filesystem::path mScenePath{};

    TArray<FString> mLogs{};
    std::array<char, 512> mConsoleInput{};

    FString mIniPath{};

    float mDeltaTime{0.016f};

    bool mInitialized{};
    bool mContextCreated{};
    bool mWin32Ready{};
    bool mDxReady{};

    bool mDirty{};
    bool mExit{};

    bool mShowConsole{true};
    bool mShowStatistics{true};
    bool mScrollToBottom{};
};

UEditorEngine::FImplementation::FImplementation(UEditorEngine& Owner)
	: mOwner{&Owner},
	  mClient{Owner} {
}

std::filesystem::path UEditorEngine::FImplementation::ChooseFile(bool Save) {
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

void UEditorEngine::FImplementation::BuildLayout() {
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

        ImGui::DockBuilderDockWindow("World Outliner", Outliner);
        ImGui::DockBuilderDockWindow("Statistics", Statistics);
        ImGui::DockBuilderDockWindow("Details", Right);
        ImGui::DockBuilderDockWindow("Console", Bottom);
        ImGui::DockBuilderDockWindow("Viewport", Center);

        ImGui::DockBuilderFinish(Dockspace);
    }
}

void UEditorEngine::FImplementation::DrawMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                mOwner->NewScene();
            }

            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                const auto Path{ChooseFile(false)};

                if (!Path.empty()) {
                    mOwner->LoadScene(Path);
                }
            }

            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                const auto Path{mScenePath.empty() ? ChooseFile(true) : mScenePath};

                if (!Path.empty()) {
                    mOwner->SaveScene(Path);
                }
            }

            if (ImGui::MenuItem("Save Scene As...")) {
                const auto Path{ChooseFile(true)};

                if (!Path.empty()) {
                    mOwner->SaveScene(Path);
                }
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                mExit = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Console", nullptr, &mShowConsole);
            ImGui::MenuItem("Statistics", nullptr, &mShowStatistics);
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
                mOwner->SaveScene(Path);
            }
        } else if (ImGui::IsKeyPressed(ImGuiKey_O)) {
            const auto Path{ChooseFile(false)};

            if (!Path.empty()) {
                mOwner->LoadScene(Path);
            }
        } else if (ImGui::IsKeyPressed(ImGuiKey_N)) {
            mOwner->NewScene();
        }
    }
}

void UEditorEngine::FImplementation::DrawOutliner() {
    if (ImGui::Begin("World Outliner")) {
        if (ImGui::Button("+ Plane")) {
            mOwner->SpawnPrimitive(EPrimitiveType::Plane);
        }

        ImGui::SameLine();

        if (ImGui::Button("+ Cube")) {
            mOwner->SpawnPrimitive(EPrimitiveType::Cube);
        }

        ImGui::SameLine();

        if (ImGui::Button("+ Sphere")) {
            mOwner->SpawnPrimitive(EPrimitiveType::Sphere);
        }

        ImGui::Separator();

        const UWorld* World{mOwner->GetWorld()};

        if (World != nullptr && World->GetPersistentLevel() != nullptr) {
            ImGui::TextUnformatted(std::format("PersistentLevel ({})", World->GetPersistentLevel()->GetActors().size()).c_str());

            for (const auto& Actor : World->GetPersistentLevel()->GetActors()) {
                ImGui::PushID(Actor.get());

                if (ImGui::Selectable(Actor->GetName().c_str(), mOwner->GetSelectedActor() == Actor.get())) {
                    mOwner->SelectActor(Actor->GetHandle());
                }

                ImGui::PopID();
            }
        }

        ImGui::Separator();

        ImGui::BeginDisabled(mOwner->GetSelectedActor() == nullptr);

        if (ImGui::Button("Delete Selected")) {
            mOwner->DestroySelectedActor();
        }

        ImGui::EndDisabled();
    }

    ImGui::End();
}

void UEditorEngine::FImplementation::DrawDetails() {
    if (ImGui::Begin("Details")) {
        AActor* Actor{mOwner->GetSelectedActor()};

        if (Actor != nullptr) {
            if (ImGui::CollapsingHeader("Actor", ImGuiTreeNodeFlags_DefaultOpen)) {
                DrawReflectedObject(*mOwner, *Actor);
            }

            for (const auto& Component : Actor->GetComponents()) {
                ImGui::PushID(Component.get());

                if (ImGui::CollapsingHeader(Component->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    mSelectedObject = Component->GetHandle();
                    DrawReflectedObject(*mOwner, *Component);
                }

                ImGui::PopID();
            }
        }
    }

    ImGui::End();
}

void UEditorEngine::FImplementation::DrawViewport() {
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

void UEditorEngine::FImplementation::DrawConsole() {
    if (!mShowConsole) {
        return;
    }

    if (ImGui::Begin("Console", &mShowConsole)) {
        if (ImGui::BeginChild("Messages", {0.0f, -ImGui::GetFrameHeightWithSpacing()}, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
            for (const FString& Message : mLogs) {
                ImGui::TextUnformatted(Message.c_str());
            }

            if (mScrollToBottom) {
                ImGui::SetScrollHereY(1.0f);
                mScrollToBottom = false;
            }
        }

        ImGui::EndChild();
        ImGui::SetNextItemWidth(-1.0f);

        if (ImGui::InputText("##Command", mConsoleInput.data(), mConsoleInput.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            mOwner->ExecuteConsoleCommand(mConsoleInput.data());
            mConsoleInput.fill(0);
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

    ImGui::End();
}

void UEditorEngine::FImplementation::DrawStatistics() {
    if (!mShowStatistics) {
        return;
    }

    if (ImGui::Begin("Statistics", &mShowStatistics)) {
        const FMemoryStats Memory{FMemory::GetStats()};
        const FRenderStats Render{mOwner->GetRenderer()->GetStats()};
        const UWorld* World{mOwner->GetWorld()};

        ImGui::TextUnformatted(std::format("Frame: {:.2f} ms", mDeltaTime * 1000.0f).c_str());
        ImGui::TextUnformatted(std::format("Live UObjects: {}", Memory.mLiveObjects).c_str());
        ImGui::TextUnformatted(std::format("Created UObjects: {}", Memory.mCreatedObjects).c_str());
        ImGui::TextUnformatted(std::format("Actors: {}", World != nullptr && World->GetPersistentLevel() != nullptr ? World->GetPersistentLevel()->GetActors().size() : 0).c_str());
        ImGui::Separator();

        ImGui::TextUnformatted(std::format("UObject heap: {:.2f} KiB", Memory.mHeapBytes / 1024.0).c_str());
        ImGui::TextUnformatted(std::format("Peak UObject heap: {:.2f} KiB", Memory.mPeakHeapBytes / 1024.0).c_str());

        std::size_t GeometryBytes{};

        for (Uint32 Index{}; Index < static_cast<Uint32>(EPrimitiveType::Count); ++Index) {
            const FMeshData& Mesh{GetPrimitiveMesh(static_cast<EPrimitiveType>(Index))};
            GeometryBytes += Mesh.mVertices.capacity() * sizeof(FMeshVertex) + Mesh.mIndices.capacity() * sizeof(Uint32);
        }

        ImGui::TextUnformatted(std::format("Shared CPU meshes: {:.2f} KiB", GeometryBytes / 1024.0).c_str());
        ImGui::TextUnformatted(std::format("GPU buffers/target: {:.2f} MiB", Render.mGpuBytes / (1024.0 * 1024.0)).c_str());
        ImGui::Separator();

        ImGui::TextUnformatted(std::format("Visible primitives: {}", Render.mVisiblePrimitives).c_str());
        ImGui::TextUnformatted(std::format("Scene draw calls: {}", Render.mDrawCalls).c_str());
        ImGui::TextUnformatted(std::format("Object uploads: {}", Render.mTransformUpdates).c_str());
    }

    ImGui::End();
}

UEditorEngine::UEditorEngine()
	: mImplementation{std::make_unique<FImplementation>(*this)} {
}

UEditorEngine::~UEditorEngine() {
    Shutdown();
}

ImplementReflection(UEditorEngine, EClassFlags::NoFactory) {
    static_cast<void>(Information);
}

bool UEditorEngine::InitializeEditor(IRendererModule& Renderer, FWindowsApplication& Platform) {
    if (!UEngine::Initialize(Renderer)) {
        return false;
    }

    StaticClass();
    auto& State{*mImplementation};
    State.mPlatform = &Platform;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    State.mContextCreated = true;

    ImGuiIO& Input{ImGui::GetIO()};
    Input.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

    std::error_code Error{};
    std::filesystem::create_directories("Saved/Week1", Error);
    State.mIniPath = "Saved/Week1/EditorLayout.ini";
    Input.IniFilename = State.mIniPath.c_str();
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

    State.mWin32Ready = ImGui_ImplWin32_Init(Platform.GetWindowHandle());
    State.mDxReady = State.mWin32Ready && ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(Renderer.GetNativeDevice()), static_cast<ID3D11DeviceContext*>(Renderer.GetNativeContext()));

    if (!State.mDxReady) {
        Shutdown();

        return false;
    }

    Platform.SetMessageHandler([](HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
        return ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam) != 0;
    });

    State.mViewport = std::make_unique<FSceneViewport>(Renderer, State.mClient);

    if (!State.mViewport->Resize(960, 640)) {
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

    State.mInitialized = true;
    State.mDirty = false;

    return true;
}

void UEditorEngine::Shutdown() {
    auto& State{*mImplementation};

    State.mClient.GetGizmo().Cancel();
    State.mSelectedActor = {};
    State.mSelectedObject = {};

    if (State.mPlatform != nullptr) {
        State.mPlatform->SetMessageHandler({});
    }

    State.mViewport.reset();

    if (State.mDxReady) {
        ImGui_ImplDX11_Shutdown();
        State.mDxReady = false;
    }

    if (State.mWin32Ready) {
        ImGui_ImplWin32_Shutdown();

        State.mWin32Ready = false;
    }

    if (State.mContextCreated) {
        ImGui::DestroyContext();
        State.mContextCreated = false;
    }

    State.mInitialized = false;
    State.mPlatform = nullptr;

    UEngine::Shutdown();
}

void UEditorEngine::TickFrame(float DeltaTime) {
    auto& State{*mImplementation};

    if (!State.mInitialized) {
        return;
    }

    State.mDeltaTime = DeltaTime;
    GetRenderer()->BeginFrame(State.mPlatform->GetWidth(), State.mPlatform->GetHeight());

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    State.DrawMenu();
    State.BuildLayout();
    State.DrawOutliner();
    State.DrawDetails();
    State.DrawViewport();
    State.DrawConsole();

    SynchronizeWorld();
    State.mViewport->Draw();

    State.DrawStatistics();

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
        const FSceneView View{mImplementation->mClient.CalcSceneView({960.0f, 640.0f})};
        Actor->SetActorTransform(FTransform{View.mLocation + View.mForward * 6.0f, {}, {1.0f, 1.0f, 1.0f}});

        SelectActor(Actor->GetHandle());
        mImplementation->mDirty = true;

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

    mImplementation->mClient.GetGizmo().Cancel();
    ClearSelection();

    if (!GetWorld()->DestroyActor(Handle)) {
        return false;
    }

    mImplementation->mDirty = true;
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

    if (!(mImplementation->mSelectedActor == Handle)) {
        mImplementation->mClient.GetGizmo().Cancel();
    }

    mImplementation->mSelectedActor = Handle;
    mImplementation->mSelectedObject = Actor->GetRootComponent() != nullptr ? Actor->GetRootComponent()->GetHandle() : Handle;
}

void UEditorEngine::ClearSelection() {
    mImplementation->mSelectedActor = {};
    mImplementation->mSelectedObject = {};
}

AActor* UEditorEngine::GetSelectedActor() const {
    return GetWorldContext() != nullptr ? Cast<AActor>(GetWorldContext()->GetRegistry().Resolve(mImplementation->mSelectedActor)) : nullptr;
}

UObject* UEditorEngine::GetSelectedObject() const {
    return GetWorldContext() != nullptr ? GetWorldContext()->GetRegistry().Resolve(mImplementation->mSelectedObject) : nullptr;
}

void UEditorEngine::SelectObject(FObjectHandle Handle) {
    if (GetWorldContext() != nullptr && GetWorldContext()->GetRegistry().Resolve(Handle) != nullptr) {
        mImplementation->mSelectedObject = Handle;
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
    mImplementation->mDirty = true;

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

    mImplementation->mScenePath = ScenePath;
    mImplementation->mDirty = false;

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

    mImplementation->mClient.GetGizmo().Cancel();

    if (!ReplaceWorld(std::move(Context))) {
        ErrorHandler::Fatal("Scene load", "Scene allocation failed");
    }

    ClearSelection();
    mImplementation->mScenePath = ScenePath;
    mImplementation->mDirty = false;

    AddLog("Loaded " + PathText(ScenePath));

    return true;
}

bool UEditorEngine::NewScene() {
    mImplementation->mClient.GetGizmo().Cancel();

    if (!ReplaceWorld(std::make_unique<FWorldContext>())) {
        return false;
    }

    ClearSelection();
    mImplementation->mScenePath.clear();
    mImplementation->mDirty = false;

    return true;
}

void UEditorEngine::AddLog(const FString& Message) {
    auto& Logs{mImplementation->mLogs};

    if (Logs.size() >= 500) {
        Logs.erase(Logs.begin(), Logs.begin() + 100);
    }

    Logs.push_back(Message);
    mImplementation->mScrollToBottom = true;
    OutputDebugStringA((Message + "\n").c_str());
}

void UEditorEngine::ExecuteConsoleCommand(const FString& Command) {
    AddLog("> " + Command);
    std::istringstream Input{Command};
    FString Verb{};

    Input >> Verb;

    if (Verb == "clear") {
        mImplementation->mLogs.clear();
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
        const std::filesystem::path Path{Text.empty() ? mImplementation->mScenePath : std::filesystem::path{std::u8string_view{reinterpret_cast<const char8_t*>(Text.data()), Text.size()}}};

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
    return mImplementation->mClient;
}

FRenderTarget* UEditorEngine::GetViewportTarget() const {
    return mImplementation->mViewport != nullptr ? mImplementation->mViewport->GetRenderTarget() : nullptr;
}

bool UEditorEngine::IsSceneDirty() const {
    return mImplementation->mDirty;
}

bool UEditorEngine::WantsExit() const {
    return mImplementation->mExit;
}
