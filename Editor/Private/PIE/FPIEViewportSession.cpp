#include "pch.h"
#include "World/ACameraActor.h"
#include "Core/Base/ErrorHandler.h"
#include "Editor/Settings/FEditorConfigManager.h"
#include "Editor/PIE/FPIEViewportSession.h"
#include "Editor/UEditorEngine.h"
#include "Editor/Panel/FViewportHostWindow.h"
#include "Render/Renderer.h"
#include "World/Input/FWorldInput.h"
#include "World/UWorld.h"
#include <cmath>
#include <cstdio>

FPIEViewportSession::FPIEViewportSession(FRenderer& Renderer, FViewportHostWindow* Host)
	: mRenderer{Renderer},
	  mHost{Host} {
    UEditorEngine* Engine{GetEditorEngine()};

    ErrorHandler::Report(Engine == nullptr, "FPIEViewportSession", "No editor engine is active", ErrorHandler::EErrorLevel::Critical);
    mWindowViewport = std::make_unique<FEditorViewport>(0, Renderer.GetDevice(), Engine->GetEditorContext());
}

FPIEViewportSession::~FPIEViewportSession() {
    ReleaseRenderResources();
}

bool FPIEViewportSession::RequestStart() {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        ResetViewport();
        return false;
    }

    if (mTargetViewportId.has_value() && (mHost == nullptr || mHost->GetViewport(*mTargetViewportId) == nullptr)) {
        return false;
    }

    if (!Engine->RequestStartPIE()) {
        return false;
    }

    SelectViewport();

    return true;
}

bool FPIEViewportSession::SetTargetViewport(std::optional<FViewportId> ViewportId) {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        ResetViewport();
        return false;
    }

    if (Engine->HasPIESession() || (ViewportId.has_value() && (mHost == nullptr || mHost->GetViewport(*ViewportId) == nullptr))) {
        return false;
    }

    mTargetViewportId = ViewportId;

    return true;
}

std::optional<FViewportId> FPIEViewportSession::GetTargetViewportId() const {
    return mTargetViewportId;
}

void FPIEViewportSession::SelectViewport() {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        ResetViewport();
        return;
    }

    if (mTargetViewportId.has_value()) {
        mViewport = mHost != nullptr ? mHost->GetViewport(*mTargetViewportId) : nullptr;

        if (mViewport == nullptr) {
            Engine->RequestStopPIE();
            return;
        }

        mHost->SetVisible(true);
        ImGui::SetWindowCollapsed(mHost->GetWindowName(), false);
    } else {
        mViewport = mWindowViewport.get();
        mState.Set(EStateFlag::FocusWindow);
    }

    mViewport->SetPIEActive(true);
    mState.Clear(EStateFlag::InputCaptured);
    mState.Set(EStateFlag::InputResetPending);
}

void FPIEViewportSession::ResetViewport() {
    if (mViewport == nullptr) {
        return;
    }

    mViewport->SetPIEActive(false);
    mViewport = nullptr;
    mState.Reset();
    mState.Set(EStateFlag::InputResetPending);
}

void FPIEViewportSession::Synchronize() {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        ResetViewport();
        return;
    }

    if (!Engine->HasPIESession()) {
        ResetViewport();
        return;
    }

    if (mViewport == nullptr) {
        SelectViewport();
    }

    if (mViewport == nullptr) {
        return;
    }

    FWorldContext* Context{Engine->GetPIEWorldContext()};

    if (Context != nullptr) {
        UWorld& World{Context->GetWorld()};

        if (!World.GetSceneHandle().IsValid()) {
            World.BindScene(mRenderer.CreateScene());
        }

        EnsureCamera(World);
    }
}

void FPIEViewportSession::DrawControls() {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        ResetViewport();
        return;
    }

    const EPIEState State{Engine->GetPIEState()};

    ImGui::BeginDisabled(State != EPIEState::Stopped);

    if (State == EPIEState::Stopped && mTargetViewportId.has_value() && (mHost == nullptr || mHost->GetViewport(*mTargetViewportId) == nullptr)) {
        mTargetViewportId.reset();
    }

    char TargetLabel[96]{"New Window"};

    if (mTargetViewportId.has_value() && mHost != nullptr) {
        const FEditorViewport* Target{mHost->GetViewport(*mTargetViewportId)};

        if (Target != nullptr) {
            std::snprintf(TargetLabel, sizeof(TargetLabel), "Viewport %u (%s)", *mTargetViewportId + 1, Target->GetViewModeName());
        }
    }

    ImGui::SetNextItemWidth(220.0f);

    if (ImGui::BeginCombo("##PIEViewportTarget", TargetLabel)) {
        if (ImGui::Selectable("New Window", !mTargetViewportId.has_value())) {
            SetTargetViewport(std::nullopt);
        }

        for (FViewportId Id{}; mHost != nullptr && Id < mHost->GetViewportCount(); ++Id) {
            const FEditorViewport* Target{mHost->GetViewport(Id)};
            char Label[96]{};

            std::snprintf(Label, sizeof(Label), "Viewport %u (%s)", Id + 1, Target->GetViewModeName());

            if (ImGui::Selectable(Label, mTargetViewportId == Id)) {
                SetTargetViewport(Id);
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();

    if (ImGui::Button("Play")) {
        RequestStart();
    }

    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(State != EPIEState::Playing && State != EPIEState::Paused);

    if (ImGui::Button(State == EPIEState::Paused ? "Resume" : "Pause")) {
        if (State == EPIEState::Paused) {
            Engine->RequestResumePIE();
        } else {
            Engine->RequestPausePIE();
            mState.Clear(EStateFlag::InputCaptured);
        }
    }

    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(State == EPIEState::Stopped || State == EPIEState::Stopping);

    if (ImGui::Button("Stop")) {
        Engine->RequestStopPIE();
    }

    ImGui::EndDisabled();

    if (State != EPIEState::Stopped) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", State == EPIEState::Paused ? "Paused" : State == EPIEState::Starting ? "Starting" : State == EPIEState::Stopping ? "Stopping" : "Playing");
    }
}

void FPIEViewportSession::BeginFrame() {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        ResetViewport();
        return;
    }

    mWindowViewport->BeginFrame();

    if (Engine->HasPIESession() && mViewport == nullptr) {
        SelectViewport();
    }

    if (!Engine->HasPIESession()) {
        mWindowViewport->ReleaseRenderResources();
    }
}

void FPIEViewportSession::DrawWindow() {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        ResetViewport();
        return;
    }

    if (!Engine->HasPIESession()) {
        return;
    }

    if (mViewport != mWindowViewport.get()) {
        if (mViewport != nullptr && (mHost == nullptr || !mHost->IsVisible() || !mTargetViewportId.has_value() || mHost->GetViewport(*mTargetViewportId) != mViewport)) {
            Engine->RequestStopPIE();
            mState.Clear(EStateFlag::InputCaptured);
        }

        return;
    }

    bool Open{true};

    ImGui::SetNextWindowSize(ImVec2{960.0f, 600.0f}, ImGuiCond_FirstUseEver);

    if (mState.IsSet(EStateFlag::FocusWindow)) {
        ImGui::SetNextWindowFocus();
        mState.Clear(EStateFlag::FocusWindow);
    }

    const bool Visible{ImGui::Begin("Play In Editor###PIEViewport", &Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)};

    if (Visible && Open) {
        const ImVec2 Position{ImGui::GetCursorScreenPos()};
        const ImVec2 Size{ImGui::GetContentRegionAvail()};
        const FRect Rect{{static_cast<Int32>(Position.x), static_cast<Int32>(Position.y)}, {static_cast<Int32>(Position.x + Size.x), static_cast<Int32>(Position.y + Size.y)}};

        mWindowViewport->Draw(Rect, ImGui::GetMainViewport()->Pos, false);
        mWindowViewport->SetFocused(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
    }

    ImGui::End();

    if (!Open) {
        Engine->RequestStopPIE();
    }
}

ImGuiKey FPIEViewportSession::GetImGuiKey(Uint8 Key) {
    if (Key >= 'A' && Key <= 'Z') {
        return static_cast<ImGuiKey>(ImGuiKey_A + Key - 'A');
    }

    if (Key >= '0' && Key <= '9') {
        return static_cast<ImGuiKey>(ImGuiKey_0 + Key - '0');
    }

    if (Key >= VK_F1 && Key <= VK_F24) {
        return static_cast<ImGuiKey>(ImGuiKey_F1 + Key - VK_F1);
    }

    if (Key >= VK_NUMPAD0 && Key <= VK_NUMPAD9) {
        return static_cast<ImGuiKey>(ImGuiKey_Keypad0 + Key - VK_NUMPAD0);
    }

    switch (Key) {
        case VK_LBUTTON: return ImGuiKey_MouseLeft;
        case VK_RBUTTON: return ImGuiKey_MouseRight;
        case VK_MBUTTON: return ImGuiKey_MouseMiddle;
        case VK_XBUTTON1: return ImGuiKey_MouseX1;
        case VK_XBUTTON2: return ImGuiKey_MouseX2;
        case VK_TAB: return ImGuiKey_Tab;
        case VK_LEFT: return ImGuiKey_LeftArrow;
        case VK_RIGHT: return ImGuiKey_RightArrow;
        case VK_UP: return ImGuiKey_UpArrow;
        case VK_DOWN: return ImGuiKey_DownArrow;
        case VK_PRIOR: return ImGuiKey_PageUp;
        case VK_NEXT: return ImGuiKey_PageDown;
        case VK_HOME: return ImGuiKey_Home;
        case VK_END: return ImGuiKey_End;
        case VK_INSERT: return ImGuiKey_Insert;
        case VK_DELETE: return ImGuiKey_Delete;
        case VK_BACK: return ImGuiKey_Backspace;
        case VK_SPACE: return ImGuiKey_Space;
        case VK_RETURN: return ImGuiKey_Enter;
        case VK_ESCAPE: return ImGuiKey_Escape;
        case VK_LSHIFT: return ImGuiKey_LeftShift;
        case VK_RSHIFT: return ImGuiKey_RightShift;
        case VK_LCONTROL: return ImGuiKey_LeftCtrl;
        case VK_RCONTROL: return ImGuiKey_RightCtrl;
        case VK_LMENU: return ImGuiKey_LeftAlt;
        case VK_RMENU: return ImGuiKey_RightAlt;
        case VK_LWIN: return ImGuiKey_LeftSuper;
        case VK_RWIN: return ImGuiKey_RightSuper;
        case VK_OEM_1: return ImGuiKey_Semicolon;
        case VK_OEM_PLUS: return ImGuiKey_Equal;
        case VK_OEM_COMMA: return ImGuiKey_Comma;
        case VK_OEM_MINUS: return ImGuiKey_Minus;
        case VK_OEM_PERIOD: return ImGuiKey_Period;
        case VK_OEM_2: return ImGuiKey_Slash;
        case VK_OEM_3: return ImGuiKey_GraveAccent;
        case VK_OEM_4: return ImGuiKey_LeftBracket;
        case VK_OEM_5: return ImGuiKey_Backslash;
        case VK_OEM_6: return ImGuiKey_RightBracket;
        case VK_OEM_7: return ImGuiKey_Apostrophe;
        case VK_CAPITAL: return ImGuiKey_CapsLock;
        case VK_SCROLL: return ImGuiKey_ScrollLock;
        case VK_NUMLOCK: return ImGuiKey_NumLock;
        case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
        case VK_PAUSE: return ImGuiKey_Pause;
        case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
        case VK_DIVIDE: return ImGuiKey_KeypadDivide;
        case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case VK_ADD: return ImGuiKey_KeypadAdd;
        default: return ImGuiKey_None;
    }
}

void FPIEViewportSession::UpdateInput(UWorld& World) {
    FWorldInput& Input{World.GetInput()};
    const ImGuiIO& IO{ImGui::GetIO()};

    for (Uint32 Index{}; Index < 256; ++Index) {
        const Uint8 Key{static_cast<Uint8>(Index)};
        const ImGuiKey GuiKey{GetImGuiKey(Key)};
        const bool Down{Key == VK_SHIFT ? IO.KeyShift : Key == VK_CONTROL ? IO.KeyCtrl : Key == VK_MENU ? IO.KeyAlt : GuiKey != ImGuiKey_None && ImGui::IsKeyDown(GuiKey)};

        Input.UpdateKey(Key, Down);
    }

    for (Uint8 Button{}; Button < 5; ++Button) {
        Input.UpdateMouseButton(Button, ImGui::IsMouseDown(Button));
    }

    const FRect& Rect{mViewport->GetDisplayRect()};

    Input.SetMouseMotion(FVector2{IO.MousePos.x - static_cast<float>(Rect.mMin.mX), IO.MousePos.y - static_cast<float>(Rect.mMin.mY)}, FVector2{IO.MouseDelta.x, IO.MouseDelta.y}, IO.MouseWheel);
}

bool FPIEViewportSession::ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, float DeltaTime) {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        ResetViewport();
        return false;
    }

    if (mState.IsSet(EStateFlag::InputResetPending)) {
        KeyboardInput.Reset();
        MouseInput.Reset();
        mState.Clear(EStateFlag::InputResetPending);
    }

    const ImGuiIO& IO{ImGui::GetIO()};

    if (!IO.WantTextInput && !IO.AppFocusLost) {
        if (ImGui::IsKeyPressed(ImGuiKey_F5, false) && !Engine->HasPIESession()) {
            RequestStart();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F6, false)) {
            if (Engine->GetPIEState() == EPIEState::Paused) {
                Engine->RequestResumePIE();
            } else if (Engine->GetPIEState() == EPIEState::Playing) {
                Engine->RequestPausePIE();
                mState.Clear(EStateFlag::InputCaptured);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && Engine->HasPIESession()) {
            Engine->RequestStopPIE();
        }
    }

    if (!Engine->HasPIESession()) {
        return false;
    }

    const bool Focused{mViewport != nullptr && mViewport->IsVisible() && mViewport->IsFocused() && !IO.AppFocusLost};
    const bool ReleaseInput{IO.KeyShift && ImGui::IsKeyPressed(ImGuiKey_F1, false)};

    if (!Focused || IO.WantTextInput || ReleaseInput || Engine->GetPIEState() != EPIEState::Playing) {
        mState.Clear(EStateFlag::InputCaptured);
    } else if (mViewport->IsHovered() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
        mState.Set(EStateFlag::InputCaptured);
    }

    FWorldContext* Context{Engine->GetPIEWorldContext()};

    if (Context != nullptr) {
        UWorld& World{Context->GetWorld()};

        if (IsInputCaptured()) {
            UpdateInput(World);
            MoveCamera(World, DeltaTime);
        } else {
            World.GetInput().Reset();
        }
    }

    if (Focused) {
        KeyboardInput.ConsumeViewportNavigation(DeltaTime, true);
        MouseInput.Reset();
        return true;
    }

    return false;
}

UCameraComponent* FPIEViewportSession::EnsureCamera(UWorld& World) {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        return nullptr;
    }

    UCameraComponent* Camera{World.GetCameraSubsystem().GetMainCamera()};

    if (Camera != nullptr) {
        return Camera;
    }

    ACameraActor* Actor{World.SpawnActorDeferred<ACameraActor>()};

    if (Actor == nullptr) {
        Engine->RequestStopPIE();
        return nullptr;
    }

    Camera = Actor->GetCameraComponent();

    if (Camera == nullptr) {
        World.DestroyActor(Actor);
        Engine->RequestStopPIE();
        return nullptr;
    }

    mViewport->ConfigurePlayCamera(*Camera);
    World.FinishSpawningActor(Actor, Actor->GetActorTransform());

    return World.GetCameraSubsystem().GetMainCamera();
}

void FPIEViewportSession::MoveCamera(UWorld& World, float DeltaTime) {
    UCameraComponent* Camera{EnsureCamera(World)};

    if (Camera == nullptr || !std::isfinite(DeltaTime) || DeltaTime <= 0.0f) {
        return;
    }

    const FWorldInput& Input{World.GetInput()};
    FTransform Transform{Camera->GetComponentTransform()};
    const FEditorSettings& Settings{FEditorConfigManager::Get().GetSettings()};
    const FVector2& Delta{Input.GetMouseDelta()};

    if (Input.IsMouseButtonDown(ImGuiMouseButton_Right) && (Delta.mX != 0.0f || Delta.mY != 0.0f)) {
        FVector3 Forward{Transform.ToMatrixNoScale().Forward()};

        Forward.Normalize();

        const float Sensitivity{Settings.mRotationSensitivity * 0.001f};
        const float MaximumElevation{std::asin(0.99f)};
        const float Yaw{std::atan2(Forward.mY, Forward.mX) + Delta.mX * Sensitivity};
        const float Elevation{std::clamp(std::asin(std::clamp(Forward.mZ, -1.0f, 1.0f)) - Delta.mY * Sensitivity, -MaximumElevation, MaximumElevation)};
        const FQuat YawRotation{FQuat::CreateFromAxisAngle(FVector3::UnitZ, Yaw)};
        const FQuat PitchRotation{FQuat::CreateFromAxisAngle(FVector3::UnitY, -Elevation)};
        FQuat Rotation{FQuat::Concatenate(YawRotation, PitchRotation)};

        Rotation.Normalize();
        Transform.SetRotation(Rotation);
    }

    const float ForwardAxis{(Input.IsKeyDown('W') ? 1.0f : 0.0f) - (Input.IsKeyDown('S') ? 1.0f : 0.0f)};
    const float RightAxis{(Input.IsKeyDown('D') ? 1.0f : 0.0f) - (Input.IsKeyDown('A') ? 1.0f : 0.0f)};
    const float UpAxis{(Input.IsKeyDown('E') ? 1.0f : 0.0f) - (Input.IsKeyDown('Q') ? 1.0f : 0.0f)};
    const FMatrix Basis{Transform.ToMatrixNoScale()};
    FVector3 Direction{Basis.Forward() * ForwardAxis + Basis.Right() * RightAxis + Basis.Up() * UpAxis};

    if (Direction.LengthSquared() > 0.0f) {
        Direction.Normalize();
        Transform.SetPosition(Transform.GetPosition() + Direction * Settings.mMoveSensitivity * DeltaTime);
    }

    Camera->SetWorldTransform(Transform);
}

bool FPIEViewportSession::BuildCamera(UWorld& World, FViewMatrices& Camera) {
    UCameraComponent* Component{EnsureCamera(World)};

    if (Component == nullptr || mViewport == nullptr || !mViewport->PrepareForRender()) {
        return false;
    }

    const D3D11_VIEWPORT& Viewport{mViewport->GetRenderViewport()};

    Component->SetAspectRatio(Viewport.Width / Viewport.Height);
    Camera.mView = Component->GetViewMatrix();
    Camera.mProjection = Component->GetProjectionMatrix();
    Camera.mViewProjection = Camera.mView * Camera.mProjection;

    FFrustum LocalFrustum{};

    FFrustum::CreateFromMatrix(LocalFrustum, Camera.mProjection.ToSimpleMath());
    LocalFrustum.Transform(Camera.mViewFrustum, Camera.mView.Inverse().ToSimpleMath());

    return true;
}

void FPIEViewportSession::Render() {
    UEditorEngine* Engine{GetEditorEngine()};

    if (Engine == nullptr) {
        ResetViewport();
        return;
    }

    FWorldContext* Context{Engine->GetPIEWorldContext()};

    if (Context == nullptr || mViewport == nullptr) {
        return;
    }

    UWorld& World{Context->GetWorld()};
    FViewMatrices Camera{};

    if (!BuildCamera(World, Camera) || !World.SendSceneUpdates()) {
        return;
    }

    const FRenderScene* Scene{mRenderer.FindScene(World.GetSceneHandle())};

    if (Scene == nullptr) {
        return;
    }

    FRenderView View{};

    View.mTarget = &mViewport->GetRenderSurface();
    View.mCamera = Camera;
    View.mSettings = mViewport->GetRenderSettings();
    View.mSettings.mBRenderSky = true;
    View.mRenderMode = ERenderMode::Lit;
    mRenderer.RenderView(View, *Scene);
}

void FPIEViewportSession::ReleaseRenderResources() {
    ResetViewport();
    mWindowViewport->ReleaseRenderResources();
}

FEditorViewport* FPIEViewportSession::GetViewport() const {
    return mViewport;
}

bool FPIEViewportSession::IsInputCaptured() const {
    return mState.IsSet(EStateFlag::InputCaptured);
}
