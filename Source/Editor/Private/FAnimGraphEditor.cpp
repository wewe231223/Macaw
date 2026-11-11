#include "FAnimGraphEditor.h"
#include "FAnimGraphNodeBuilder.h"
#include "Asset/FAssetManager.h"
#include "Asset/Animation/UAnimGraph.h"
#include "Core/FilePath.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Engine/Animation/FAnimGraphNodeRegistry.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_node_editor.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <format>
#include <utility>

namespace ed = ax::NodeEditor;

namespace Editor {

    namespace {
        constexpr char WindowId[]{"###AnimGraphEditor"};
        constexpr char VariablePayload[]{"MACAW_ANIM_GRAPH_VARIABLE"};

        // 노드 에디터 식별자 규칙입니다. 그래프 노드는 (식별자 + 1) << 8이고, 그 아래 8비트는 핀입니다.
        // 0xFF는 출력 핀, 1~254는 입력 핀 순서 + 1입니다. Output 노드와 그 입력 핀은 그래프 노드와 겹치지 않는 작은 값입니다.
        // 링크는 받는 입력 핀 하나에 하나뿐이므로 입력 핀 식별자를 링크 식별자로도 씁니다.
        constexpr std::uintptr_t OutputNode{1};
        constexpr std::uintptr_t OutputInputPin{1};
        constexpr std::uintptr_t OutputPinBits{0xFF};
        constexpr std::uintptr_t GetterBit{std::uintptr_t{1} << 61};
        constexpr std::uintptr_t FloatInputBit{std::uintptr_t{1} << 60};

        std::uintptr_t GetterKey(Core::Int32 Id) {
            return GetterBit | ((static_cast<std::uintptr_t>(Id) + 1) << 8);
        }

        Core::Int32 DecodeGetter(std::uintptr_t Key) {
            return (Key & GetterBit) != 0 ? static_cast<Core::Int32>(((Key & ~GetterBit) >> 8) - 1) : -1;
        }

        std::uintptr_t NodeKey(Core::Int32 Id) {
            return Id == Asset::UAnimGraph::OutputNodeId ? OutputNode : (static_cast<std::uintptr_t>(Id) + 1) << 8;
        }

        std::uintptr_t InputPinKey(Core::Int32 Node, Core::Int32 Pin) {
            return Node == Asset::UAnimGraph::OutputNodeId ? OutputInputPin : NodeKey(Node) | static_cast<std::uintptr_t>(Pin + 1);
        }

        std::uintptr_t OutputPinKey(Core::Int32 Node) {
            return NodeKey(Node) | OutputPinBits;
        }

        std::uintptr_t FloatInputKey(Core::Int32 Node, std::size_t Parameter) {
            return FloatInputBit | InputPinKey(Node, static_cast<Core::Int32>(Parameter));
        }

        bool IsBindableFloat(const Engine::FAnimGraphParameterInfo& Info) {
            return Info.mType == Engine::EAnimGraphParameterType::Float && Info.mBindable;
        }

        const Engine::FAnimGraphParameterInfo* FindFloatInput(const Asset::FAnimGraphNode* Node, Core::Int32 Parameter) {
            const Engine::FAnimGraphNodeType* Type{Node != nullptr ? Engine::FAnimGraphNodeRegistry::Find(Node->mType) : nullptr};

            if (Type == nullptr || Parameter < 0 || static_cast<std::size_t>(Parameter) >= Type->mParameters.size()) {
                return nullptr;
            }

            const Engine::FAnimGraphParameterInfo& Info{Type->mParameters[static_cast<std::size_t>(Parameter)]};

            return IsBindableFloat(Info) ? &Info : nullptr;
        }

        struct FPinRef {
        public:
            Core::Int32 mNode{};
            Core::Int32 mPin{};
            bool mOutput{};
        };

        FPinRef DecodePin(std::uintptr_t Key) {
            if (Key == OutputInputPin) {
                return {Asset::UAnimGraph::OutputNodeId, 0, false};
            }

            const std::uintptr_t Low{Key & 0xFF};

            return {static_cast<Core::Int32>((Key >> 8) - 1), Low == OutputPinBits ? 0 : static_cast<Core::Int32>(Low) - 1, Low == OutputPinBits};
        }

        Core::Int32 DecodeNode(std::uintptr_t Key) {
            if ((Key & GetterBit) != 0) {
                return -2;
            }

            return Key == OutputNode ? Asset::UAnimGraph::OutputNodeId : static_cast<Core::Int32>((Key >> 8) - 1);
        }

        bool IsUnder(const std::filesystem::path& Path, const std::filesystem::path& Root) {
            const auto Relative{Path.lexically_normal().lexically_relative(Root.lexically_normal())};

            return !Relative.empty() && *Relative.begin() != L"..";
        }

        Core::FString FormatFloat(float Value) {
            return std::format("{}", Value);
        }

        // 노드 본문에 보일 파라미터 요약입니다. 경로는 파일 이름만 보여 줍니다.
        Core::FString DisplayParameterValue(const Engine::FAnimGraphParameterInfo& Info, const Asset::FAnimGraphNode& Node) {
            const Core::FString* Value{Node.FindParameter(Info.mName)};
            const Core::FString Text{Value != nullptr ? *Value : Info.mDefault};

            if (Info.mType == Engine::EAnimGraphParameterType::AnimSequence || Info.mType == Engine::EAnimGraphParameterType::BlendSpace) {
                return Text.empty() ? Core::FString{"(없음)"} : Core::FilePath::ToUtf8(Core::FilePath::FromUtf8(Text).stem());
            }

            return Text;
        }
    }

    FAnimGraphEditor::FAnimGraphEditor(Asset::FAssetManager* Assets, Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails)
        : mAssets{Assets},
          mCatalog{Catalog},
          mThumbnails{Thumbnails} {
        ed::Config Config{};

        Config.SettingsFile = nullptr;
        mEditor = ed::CreateEditor(&Config);
        mGraphContexts.emplace("Initial", mEditor);
    }

    FAnimGraphEditor::~FAnimGraphEditor() {
        ClearGraphContexts();
    }

    bool FAnimGraphEditor::Open(const std::filesystem::path& Path, Core::FString& Error) {
        Asset::UAnimGraph* Source{mAssets != nullptr ? mAssets->LoadAnimGraph(Path, Error) : nullptr};

        if (Source == nullptr) {
            return false;
        }

        mNavigation.clear();
        ClearGraphContexts();
        mRootEditing = CoreUObject::NewObject<Asset::UAnimGraph>();
        mEditing = mRootEditing.get();
        mEditing->CopyFrom(*Source);
        mEditing->SetName(Source->GetName());
        mContextNode = 0;

        mPath = Path;
        mDirty = false;
        mOpen = true;
        mApplyPositions = true;
        mSelectedNode = -2;
        mNewVariable.fill('\0');
        mError.clear();

        // 다른 그래프의 노드가 남지 않도록 노드 에디터 컨텍스트를 새로 만듭니다.
        mView = {};
        mView.mGraph = mEditing;
        ActivateView(mView);
        ImGui::SetWindowFocus(WindowId);

        return true;
    }

    void FAnimGraphEditor::CloseRetired(const std::filesystem::path& Path) {
        if (!mOpen || mAssets == nullptr) {
            return;
        }

        const std::filesystem::path Root{mAssets->GetContentRoot()};
        const std::filesystem::path Target{Path.is_absolute() ? Path : Root / Path};

        if (Root / mPath == Target || IsUnder(Root / mPath, Target)) {
            Close();
        }
    }

    bool FAnimGraphEditor::IsFocused() const {
        ImGuiWindow* Window{mOpen ? ImGui::FindWindowByName(WindowId) : nullptr};
        ImGuiWindow* Focused{ImGui::GetCurrentContext()->NavWindow};

        return Window != nullptr && Focused != nullptr && ImGui::IsWindowChildOf(Focused, Window, true, false);
    }

    void FAnimGraphEditor::Close() {
        mOpen = false;
        mEditing = nullptr;
        mNavigation.clear();
        mRootEditing.reset();
    }

    bool FAnimGraphEditor::Save() {
        if (mEditing == nullptr || mAssets == nullptr) {
            return false;
        }

        if (!mAssets->SaveAnimGraph(mPath, *mRootEditing, mError)) {
            return false;
        }

        mDirty = false;
        mError.clear();

        return true;
    }

    void FAnimGraphEditor::Draw() {
        if (!mOpen || mEditing == nullptr) {
            return;
        }

        const Core::FString Title{std::format("Anim Graph - {}{}{}", mRootEditing->GetName(), mDirty ? " *" : "", WindowId)};
        bool Visible{true};

        ImGui::SetNextWindowSize({1180.0f, 680.0f}, ImGuiCond_FirstUseEver);

        const bool Expanded{ImGui::Begin(Title.c_str(), &Visible)};

        if (!Visible) {
            ImGui::End();
            Close();

            return;
        }

        const bool SaveRequested{IsFocused() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)};

        if (Expanded && !mError.empty()) {
            ImGui::TextColored({1.0f, 0.45f, 0.4f, 1.0f}, "%s", mError.c_str());
        }

        if (Expanded && ImGui::BeginTable("AnimGraphEditorLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV, ImGui::GetContentRegionAvail())) {
            ImGui::TableSetupColumn("Variables", ImGuiTableColumnFlags_WidthFixed, 230.0f);
            ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            if (ImGui::BeginChild("Variables")) {
                DrawVariables(SaveRequested);
            }

            ImGui::EndChild();
            ImGui::TableSetColumnIndex(1);
            DrawNavigation();

            if (mView.mMode == EGraphView::Machine) {
                DrawStateMachine();
            } else if (mView.mMode == EGraphView::Rule) {
                DrawTransitionRule();
            } else {
                DrawGraph();
            }

            ImGui::EndTable();
        }

        if (SaveRequested && mDirty) {
            Save();
        }

        ImGui::End();
    }

    void FAnimGraphEditor::DrawVariables(bool SaveRequested) {
        const ImGuiStyle& Style{ImGui::GetStyle()};
        const float TextHeight{ImGui::GetTextLineHeight()};
        const float AddWidth{ImGui::CalcTextSize("+ 추가").x + 2.0f * Style.FramePadding.x};

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, TextHeight * 0.3f);
        ImGui::TextUnformatted("Variables");
        ImGui::SameLine();
        ImGui::TextDisabled("%zu", mRootEditing->GetVariables().size());

        ImGui::SetNextItemWidth(std::max(1.0f, ImGui::GetContentRegionAvail().x - AddWidth - Style.ItemSpacing.x));
        const bool AddFromName{ImGui::InputTextWithHint("##NewVariable", "새 변수 이름", mNewVariable.data(), mNewVariable.size(), ImGuiInputTextFlags_EnterReturnsTrue)};
        ImGui::SameLine();

        bool Added{};

        if (ImGui::Button("+ 추가", {AddWidth, 0.0f}) || AddFromName) {
            Added = mRootEditing->AddVariable({mNewVariable.data(), 0.0f, Asset::EAnimGraphVariableSource::Manual});

            if (Added) {
                mNewVariable.fill('\0');
                mDirty = true;
                mError.clear();
            } else {
                mError = "변수 이름은 비어 있지 않고 다른 변수와 겹치지 않아야 합니다.";
            }
        }

        ImGui::Spacing();

        Core::Int32 Remove{-1};

        if (ImGui::BeginChild("VariableList", {}, ImGuiChildFlags_NavFlattened)) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{TextHeight * 0.45f, TextHeight * 0.35f});
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, TextHeight * 0.5f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.11f, 0.14f, 0.18f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_Border, {0.29f, 0.35f, 0.43f, 0.55f});

            for (std::size_t Index{}; Index < mRootEditing->GetVariables().size(); ++Index) {
                if (DrawVariable(Index, SaveRequested)) {
                    Remove = static_cast<Core::Int32>(Index);
                }
            }

            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);

            if (Added) {
                ImGui::SetScrollHereY(1.0f);
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        if (Remove >= 0) {
            mRootEditing->RemoveVariable(Remove);
            mDirty = true;
        }
    }

    bool FAnimGraphEditor::DrawVariable(std::size_t Index, bool SaveRequested) {
        ImGui::PushID(static_cast<int>(Index));
        const bool Visible{ImGui::BeginChild("Variable", {}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)};
        bool Remove{};

        if (Visible) {
            const ImGuiStyle& Style{ImGui::GetStyle()};
            const float Width{std::max(1.0f, ImGui::GetContentRegionAvail().x)};
            const float FrameHeight{ImGui::GetFrameHeight()};
            const float GetWidth{ImGui::CalcTextSize("Get").x + 2.0f * Style.FramePadding.x};
            const float NameWidth{std::max(1.0f, Width - FrameHeight - GetWidth - 2.0f * Style.ItemSpacing.x)};
            Asset::FAnimGraphVariable Variable{mRootEditing->GetVariables()[Index]};
            std::array<char, 64> Name{};
            bool Changed{};

            std::copy_n(Variable.mName.begin(), std::min(Variable.mName.size(), Name.size() - 1), Name.begin());
            ImGui::SetNextItemWidth(NameWidth);

            // 이름은 입력을 마칠 때 바꿉니다. 글자마다 바꾸면 중간 이름이 다른 변수와 겹칠 수 있습니다.
            const bool Rename{ImGui::InputText("##Name", Name.data(), Name.size(), ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivatedAfterEdit()};

            if ((Rename || (SaveRequested && ImGui::IsItemActive())) && Variable.mName != Name.data()) {
                Variable.mName = Name.data();
                Changed = true;
            }

            if (ImGui::CalcTextSize(Variable.mName.c_str()).x + 2.0f * Style.FramePadding.x > NameWidth) {
                ImGui::SetItemTooltip("%s", Variable.mName.c_str());
            }

            ImGui::SameLine();
            ImGui::Button("Get", {GetWidth, FrameHeight});

            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload(VariablePayload, Variable.mName.c_str(), Variable.mName.size() + 1);
                ImGui::Text("Get %s", Variable.mName.c_str());
                ImGui::EndDragDropSource();
            } else {
                ImGui::SetItemTooltip("캔버스로 끌어 Get %s 추가", Variable.mName.c_str());
            }

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.55f, 0.22f, 0.25f, 0.85f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.68f, 0.25f, 0.28f, 1.0f});
            Remove = ImGui::Button("×##Remove", {FrameHeight, FrameHeight});
            ImGui::PopStyleColor(3);
            ImGui::SetItemTooltip("변수 삭제");

            const char* Sources[]{"Manual", "Owner Speed"};
            const float MinSourceWidth{ImGui::CalcTextSize(Sources[1]).x + FrameHeight + 2.0f * Style.FramePadding.x};
            const float MinValueWidth{ImGui::CalcTextSize("-00.000").x + 2.0f * Style.FramePadding.x};
            const float FieldWidth{Width - Style.ItemSpacing.x};
            const bool SideBySide{FieldWidth >= MinSourceWidth + MinValueWidth};
            const float SourceWidth{SideBySide ? std::clamp(FieldWidth * 0.6f, MinSourceWidth, FieldWidth - MinValueWidth) : Width};
            int Source{static_cast<int>(Variable.mSource)};

            ImGui::TextDisabled("Source");

            if (SideBySide) {
                ImGui::SameLine(0.0f, SourceWidth + Style.ItemSpacing.x - ImGui::GetItemRectSize().x);
                ImGui::TextDisabled("Default");
            }

            ImGui::SetNextItemWidth(SourceWidth);

            if (ImGui::Combo("##Source", &Source, Sources, 2)) {
                Variable.mSource = static_cast<Asset::EAnimGraphVariableSource>(Source);
                Changed = true;
            }

            if (SideBySide) {
                ImGui::SameLine();
            } else {
                ImGui::TextDisabled("Default");
            }

            ImGui::SetNextItemWidth(-1.0f);

            if (Variable.mType == Asset::EAnimGraphVariableType::Bool) {
                Changed |= ImGui::Checkbox("##Default", &Variable.mBoolDefault);
                Variable.mSource = Asset::EAnimGraphVariableSource::Manual;
            } else {
                Changed |= ImGui::DragFloat("##Default", &Variable.mDefault, 0.01f, 0.0f, 0.0f, "%.3f");
            }

            int Type{static_cast<int>(Variable.mType)};
            const char* Types[]{"Float", "Bool"};

            if (ImGui::Combo("Type", &Type, Types, 2)) {
                Variable.mType = static_cast<Asset::EAnimGraphVariableType>(Type);
                Changed = true;
            }

            if (Changed) {
                if (mRootEditing->SetVariable(static_cast<Core::Int32>(Index), Variable)) {
                    mDirty = true;
                    mError.clear();
                } else {
                    mError = "변수 이름은 비어 있지 않고 다른 변수와 겹치지 않아야 합니다.";
                }
            }
        }

        ImGui::EndChild();
        ImGui::PopID();

        return Remove;
    }

    Core::Int32 FAnimGraphEditor::FindVariableIndex(const Core::FString& Name) const {
        const auto& Variables{mEditing->GetVariables()};
        const auto Found{std::find_if(Variables.begin(), Variables.end(), [&Name](const Asset::FAnimGraphVariable& Variable) {
            return Variable.mName == Name;
        })};

        return Found == Variables.end() ? -1 : static_cast<Core::Int32>(Found - Variables.begin());
    }

    void FAnimGraphEditor::DrawValueGetters() {
        Core::TArray<Core::Int32> Bound{};

        for (const Asset::FAnimGraphNode& Node : mEditing->GetNodes()) {
            const Engine::FAnimGraphNodeType* Type{Engine::FAnimGraphNodeRegistry::Find(Node.mType)};

            if (Type == nullptr) {
                continue;
            }

            for (const Engine::FAnimGraphParameterInfo& Info : Type->mParameters) {
                if (!IsBindableFloat(Info)) {
                    continue;
                }

                const Core::FString* Binding{Node.FindParameter(Info.mName + ".Binding")};

                if (Binding == nullptr) {
                    continue;
                }

                if (const Asset::FAnimGraphVariableGetter* Getter{mEditing->FindBoundGetter(Node, Info.mName)}; Getter != nullptr) {
                    Bound.push_back(Getter->mId);
                } else if (const Core::Int32 Id{mEditing->AddVariableGetter(*Binding, Node.mX - 245.0f, Node.mY - 170.0f)}; Id >= 0) {
                    ed::SetNodePosition(ed::NodeId{GetterKey(Id)}, {Node.mX - 245.0f, Node.mY - 170.0f});
                    Bound.push_back(Id);
                }
            }
        }

        for (const Asset::FAnimGraphVariableGetter& Getter : mEditing->GetVariableGetters()) {
            const Core::Int32 Index{FindVariableIndex(Getter.mVariable)};

            if (Index < 0) {
                continue;
            }

            const std::uintptr_t Key{GetterKey(Getter.mId)};

            if (mApplyPositions) {
                ed::SetNodePosition(ed::NodeId{Key}, {Getter.mX, Getter.mY});
            }

            const Asset::FAnimGraphVariable& Variable{mEditing->GetVariables()[static_cast<std::size_t>(Index)]};
            const Core::FString Value{FormatFloat(Variable.mDefault)};
            const bool Connected{std::find(Bound.begin(), Bound.end(), Getter.mId) != Bound.end()};

            Editor::FAnimGraphNodeBuilder::DrawFloatGetter(ed::NodeId{Key}, ed::PinId{Key | OutputPinBits}, Variable.mName.c_str(), Value.c_str(), Connected);
        }
    }

    void FAnimGraphEditor::DrawValueLinks() {
        const ImVec4 Color{Editor::FAnimGraphNodeBuilder::FloatColor()};

        for (const Asset::FAnimGraphNode& Node : mEditing->GetNodes()) {
            const Engine::FAnimGraphNodeType* Type{Engine::FAnimGraphNodeRegistry::Find(Node.mType)};

            if (Type == nullptr) {
                continue;
            }

            for (std::size_t Index{}; Index < Type->mParameters.size(); ++Index) {
                const Engine::FAnimGraphParameterInfo& Info{Type->mParameters[Index]};
                const Asset::FAnimGraphVariableGetter* Getter{IsBindableFloat(Info) ? mEditing->FindBoundGetter(Node, Info.mName) : nullptr};

                if (Getter != nullptr) {
                    const std::uintptr_t Input{FloatInputKey(Node.mId, Index)};
                    const std::uintptr_t Output{GetterKey(Getter->mId) | OutputPinBits};

                    ed::Link(ed::LinkId{Input}, ed::PinId{Output}, ed::PinId{Input}, Color, 2.0f);
                }
            }
        }
    }

    void FAnimGraphEditor::DrawGraph() {
        const ImVec2 CanvasOrigin{ImGui::GetCursorScreenPos()};
        const ImVec2 CanvasSize{ImGui::GetContentRegionAvail()};
        ed::SetCurrentEditor(mEditor);
        Editor::FAnimGraphNodeBuilder::ApplyStyle();
        ed::Begin("AnimGraphCanvas", CanvasSize);

        if (mApplyPositions) {
            ed::SetNodePosition(ed::NodeId{OutputNode}, {mEditing->GetOutputX(), mEditing->GetOutputY()});

            for (const Asset::FAnimGraphNode& Node : mEditing->GetNodes()) {
                ed::SetNodePosition(ed::NodeId{NodeKey(Node.mId)}, {Node.mX, Node.mY});
            }
        }

        const ImVec4 PoseColor{Editor::FAnimGraphNodeBuilder::PoseColor()};
        const ImVec4 FloatColor{Editor::FAnimGraphNodeBuilder::FloatColor()};
        const auto HasOutput{[this](Core::Int32 Id) {
            return std::any_of(mEditing->GetLinks().begin(), mEditing->GetLinks().end(), [Id](const Asset::FAnimGraphLink& Link) {
                return Link.mFromNode == Id;
            });
        }};

        // Output Pose 노드입니다.
        Editor::FAnimGraphNodeBuilder OutputBuilder{};

        OutputBuilder.FitHeader("Output Pose", "Output Pose");
        OutputBuilder.FitPins("Result");
        OutputBuilder.Begin(ed::NodeId{OutputNode}, 160.0f);
        OutputBuilder.Header("Output Pose", "Output Pose");
        OutputBuilder.InputPin(ed::PinId{OutputInputPin}, "Result", mEditing->FindInput(Asset::UAnimGraph::OutputNodeId, 0) != Asset::UAnimGraph::OutputNodeId, PoseColor);
        OutputBuilder.End();

        DrawValueGetters();

        // 블루프린트처럼 색 헤더 아래 첫 줄에 첫 입력 핀과 출력 핀을 나란히 두고, 그 아래에 파라미터 요약을 보여 줍니다.
        for (const Asset::FAnimGraphNode& Node : mEditing->GetNodes()) {
            const Engine::FAnimGraphNodeType* Type{Engine::FAnimGraphNodeRegistry::Find(Node.mType)};
            const char* TypeTitle{Type != nullptr ? Type->mTitle.c_str() : Node.mType.c_str()};
            const char* Title{Node.mName.empty() ? TypeTitle : Node.mName.c_str()};
            const std::size_t PinCount{Type != nullptr ? Type->mPoseInputs.size() : 0};
            const Core::FString UnknownType{Type == nullptr ? "알 수 없는 노드 종류: " + Node.mType : Core::FString{}};
            Core::TArray<Core::FString> Parameters{};
            Editor::FAnimGraphNodeBuilder Builder{};

            Builder.FitHeader(Title, TypeTitle);
            Builder.FitPins(PinCount > 0 ? Type->mPoseInputs.front().c_str() : nullptr, "Pose");

            for (std::size_t Pin{1}; Pin < PinCount; ++Pin) {
                Builder.FitPins(Type->mPoseInputs[Pin].c_str());
            }

            if (Type == nullptr) {
                Builder.FitText(UnknownType.c_str());
            } else {
                Parameters.reserve(Type->mParameters.size());

                for (const Engine::FAnimGraphParameterInfo& Info : Type->mParameters) {
                    Core::FString Value{DisplayParameterValue(Info, Node)};

                    if (IsBindableFloat(Info)) {
                        const Core::FString* Binding{Node.FindParameter(Info.mName + ".Binding")};

                        Value = Binding != nullptr ? Info.mName : Info.mName + "  " + Value;
                        Builder.FitPins(Value.c_str());
                    } else {
                        Builder.FitProperty(Info.mName.c_str(), Value.c_str());
                    }

                    Parameters.push_back(std::move(Value));
                }
            }

            Builder.Begin(ed::NodeId{NodeKey(Node.mId)}, 210.0f);
            Builder.Header(Title, TypeTitle);

            for (std::size_t Pin{}; Pin < PinCount; ++Pin) {
                const bool Connected{mEditing->FindInput(Node.mId, static_cast<Core::Int32>(Pin)) != Asset::UAnimGraph::OutputNodeId};

                Builder.InputPin(ed::PinId{InputPinKey(Node.mId, static_cast<Core::Int32>(Pin))}, Type->mPoseInputs[Pin].c_str(), Connected, PoseColor);

                if (Pin == 0) {
                    Builder.OutputPin(ed::PinId{OutputPinKey(Node.mId)}, "Pose", HasOutput(Node.mId), PoseColor, true);
                }
            }

            if (PinCount == 0) {
                Builder.OutputPin(ed::PinId{OutputPinKey(Node.mId)}, "Pose", HasOutput(Node.mId), PoseColor, false);
            }

            if (Type == nullptr) {
                Builder.BeginBody();
                Builder.DrawText(UnknownType.c_str(), {1.0f, 0.45f, 0.4f, 1.0f});
            } else {
                for (std::size_t Index{}; Index < Parameters.size(); ++Index) {
                    const Engine::FAnimGraphParameterInfo& Info{Type->mParameters[Index]};

                    if (IsBindableFloat(Info)) {
                        const Core::FString* Binding{Node.FindParameter(Info.mName + ".Binding")};
                        const bool Connected{Binding != nullptr && FindVariableIndex(*Binding) >= 0};

                        Builder.InputPin(ed::PinId{FloatInputKey(Node.mId, Index)}, Parameters[Index].c_str(), Connected, FloatColor);
                    }
                }

                if (std::any_of(Type->mParameters.begin(), Type->mParameters.end(), [](const Engine::FAnimGraphParameterInfo& Info) {
                    return !IsBindableFloat(Info);
                })) {
                    Builder.BeginBody();
                }

                for (std::size_t Index{}; Index < Parameters.size(); ++Index) {
                    if (const Engine::FAnimGraphParameterInfo& Info{Type->mParameters[Index]}; !IsBindableFloat(Info)) {
                        Builder.DrawProperty(Info.mName.c_str(), Parameters[Index].c_str());
                    }
                }
            }

            Builder.End();
        }

        for (const Asset::FAnimGraphLink& Link : mEditing->GetLinks()) {
            const std::uintptr_t Input{InputPinKey(Link.mToNode, Link.mToPin)};

            ed::Link(ed::LinkId{Input}, ed::PinId{OutputPinKey(Link.mFromNode)}, ed::PinId{Input}, PoseColor, 2.0f);
        }

        DrawValueLinks();

        if (mFitRequested && !mApplyPositions) {
            ed::NavigateToContent(0.0f);
            mFitRequested = false;
        }

        // 핀을 끌어 새 링크를 만듭니다. 출력 핀과 입력 핀을 하나씩 잇고, 같은 노드나 순환 연결은 거부합니다.
        if (ed::BeginCreate({0.98f, 0.72f, 0.2f, 1.0f}, 2.0f)) {
            ed::PinId Start{};
            ed::PinId End{};

            if (ed::QueryNewLink(&Start, &End) && Start && End) {
                std::uintptr_t From{Start.Get()};
                std::uintptr_t To{End.Get()};

                if ((From & OutputPinBits) != OutputPinBits) {
                    std::swap(From, To);
                }

                if (((From | To) & (GetterBit | FloatInputBit)) != 0) {
                    const Asset::FAnimGraphVariableGetter* Getter{mEditing->FindVariableGetter(DecodeGetter(From))};
                    const FPinRef Input{(To & FloatInputBit) != 0 ? DecodePin(To & ~FloatInputBit) : FPinRef{-2, -1, false}};
                    Asset::FAnimGraphNode* Target{mEditing->FindNode(Input.mNode)};
                    const Engine::FAnimGraphParameterInfo* Info{FindFloatInput(Target, Input.mPin)};

                    if (Getter == nullptr || Info == nullptr) {
                        ed::RejectNewItem({1.0f, 0.3f, 0.3f, 1.0f}, 2.0f);
                    } else if (ed::AcceptNewItem(FloatColor, 3.0f)) {
                        if (mEditing->FindBoundGetter(*Target, Info->mName) != Getter) {
                            Target->SetParameter(Info->mName + ".Binding", Getter->mVariable, Getter->mId);
                            mDirty = true;
                        }
                    }
                } else {
                    const FPinRef A{DecodePin(From)};
                    const FPinRef B{DecodePin(To)};

                    if (!A.mOutput || B.mOutput || (B.mNode != Asset::UAnimGraph::OutputNodeId && mEditing->WouldCreateCycle(A.mNode, B.mNode))) {
                        ed::RejectNewItem({1.0f, 0.3f, 0.3f, 1.0f}, 2.0f);
                    } else if (ed::AcceptNewItem({0.4f, 0.9f, 0.5f, 1.0f}, 3.0f) && mEditing->Connect(A.mNode, B.mNode, B.mPin)) {
                        mDirty = true;
                    }
                }
            }
        }

        ed::EndCreate();

        if (ed::BeginDelete()) {
            ed::LinkId Link{};
            ed::NodeId Node{};

            while (ed::QueryDeletedLink(&Link)) {
                if (ed::AcceptDeletedItem()) {
                    const bool FloatLink{(Link.Get() & FloatInputBit) != 0};
                    const FPinRef Input{DecodePin(Link.Get() & ~FloatInputBit)};

                    if (FloatLink) {
                        Asset::FAnimGraphNode* Target{mEditing->FindNode(Input.mNode)};

                        if (const Engine::FAnimGraphParameterInfo* Info{FindFloatInput(Target, Input.mPin)}; Info != nullptr) {
                            const Core::FString BindingKey{Info->mName + ".Binding"};

                            if (Target->FindParameter(BindingKey) != nullptr) {
                                Target->SetParameter(BindingKey, {});
                                mDirty = true;
                            }
                        }
                    } else {
                        mDirty |= mEditing->Disconnect(Input.mNode, Input.mPin);
                    }
                }
            }

            while (ed::QueryDeletedNode(&Node)) {
                const Core::Int32 Id{DecodeNode(Node.Get())};

                // Output 노드는 지울 수 없습니다.
                if (Id == Asset::UAnimGraph::OutputNodeId) {
                    ed::RejectDeletedItem();
                } else if (ed::AcceptDeletedItem()) {
                    if ((Node.Get() & GetterBit) != 0) {
                        mDirty |= mEditing->RemoveVariableGetter(DecodeGetter(Node.Get()));
                    } else {
                        mDirty |= mEditing->RemoveNode(Id);
                    }

                    if (mSelectedNode == Id) {
                        mSelectedNode = -2;
                    }
                }
            }
        }

        ed::EndDelete();

        // 캔버스 좌표에서 마우스 위치를 기억해 두고, 팝업은 노드 에디터를 잠시 멈춘 화면 좌표에서 그립니다.
        const ImVec2 CanvasMouse{ImGui::GetMousePos()};
        ed::NodeId ContextNode{};
        Core::Int32 OpenMachineId{-1};

        ed::Suspend();

        const ImRect DropArea{CanvasOrigin, {CanvasOrigin.x + CanvasSize.x, CanvasOrigin.y + CanvasSize.y}};

        if (ImGui::BeginDragDropTargetCustom(DropArea, ImGui::GetID("AnimGraphVariableDrop"))) {
            if (const ImGuiPayload* Payload{ImGui::AcceptDragDropPayload(VariablePayload)}; Payload != nullptr && Payload->Data != nullptr && Payload->DataSize > 1 && static_cast<const char*>(Payload->Data)[Payload->DataSize - 1] == '\0') {
                const Core::FString Name{static_cast<const char*>(Payload->Data), static_cast<std::size_t>(Payload->DataSize - 1)};

                if (const Core::Int32 Id{mEditing->AddVariableGetter(Name, CanvasMouse.x, CanvasMouse.y)}; Id >= 0) {
                    ed::SetNodePosition(ed::NodeId{GetterKey(Id)}, CanvasMouse);
                    mDirty = true;
                } else {
                    mError = "Get 노드를 추가할 수 없습니다.";
                }
            }

            ImGui::EndDragDropTarget();
        }

        if (ed::ShowNodeContextMenu(&ContextNode)) {
            mContextNode = ContextNode.Get();
            mSelectedNode = DecodeNode(mContextNode);
            ed::SelectNode(ContextNode);
            ImGui::OpenPopup("AnimGraphNodeMenu");
        } else if (ed::ShowBackgroundContextMenu()) {
            mCreateX = CanvasMouse.x;
            mCreateY = CanvasMouse.y;
            ImGui::OpenPopup("AnimGraphCreateNode");
        }

        if (ImGui::BeginPopup("AnimGraphCreateNode")) {
            ImGui::TextDisabled("노드 추가");
            ImGui::Separator();

            for (const Engine::FAnimGraphNodeType& Type : Engine::FAnimGraphNodeRegistry::GetNodeTypes()) {
                if (ImGui::MenuItem(Type.mTitle.c_str())) {
                    const Core::Int32 Id{mEditing->AddNode(Type.mType, Type.mTitle, mCreateX, mCreateY)};

                    if (Id >= 0) {
                        ed::SetNodePosition(ed::NodeId{NodeKey(Id)}, {mCreateX, mCreateY});
                        mSelectedNode = Id;
                        mDirty = true;
                    } else {
                        mError = "노드 수가 한도에 도달했습니다.";
                    }
                }
            }

            ImGui::EndPopup();
        }

        ImGui::SetNextWindowSizeConstraints({360.0f, 0.0f}, {500.0f, ImGui::GetIO().DisplaySize.y});

        if (ImGui::BeginPopup("AnimGraphNodeMenu")) {
            const Core::Int32 Id{DecodeNode(mContextNode)};
            const Core::Int32 Getter{DecodeGetter(mContextNode)};
            const bool CanDelete{mEditing->FindVariableGetter(Getter) != nullptr || mEditing->FindNode(Id) != nullptr};
            mSelectedNode = Id;
            ImGui::PushItemWidth(220.0f);

            if (const auto* Variable{mEditing->FindVariableGetter(Getter)}) {
                ImGui::SeparatorText("Variable Getter");
                ImGui::Text("Variable: %s", Variable->mVariable.c_str());
            } else if (Id == Asset::UAnimGraph::OutputNodeId) {
                ImGui::SeparatorText("Output Pose");
            } else {
                DrawSelection();
            }

            ImGui::PopItemWidth();
            ImGui::Separator();

            if (const auto* Node{mEditing->FindNode(Id)}; Node != nullptr && Node->mType == "StateMachine" && ImGui::MenuItem("Open State Machine")) {
                OpenMachineId = Id;
            }

            if (ImGui::MenuItem("삭제", "Delete", false, CanDelete)) {
                mDirty |= Getter >= 0 ? mEditing->RemoveVariableGetter(Getter) : mEditing->RemoveNode(Id);
                mSelectedNode = -2;
            }

            if (ImGui::MenuItem("입력 연결 끊기", nullptr, false, Id != -2)) {
                for (Core::Int32 Pin{}; Pin < 255; ++Pin) {
                    mDirty |= mEditing->Disconnect(Id, Pin);
                }

                if (Asset::FAnimGraphNode* Target{mEditing->FindNode(Id)}; Target != nullptr) {
                    const auto Removed{std::erase_if(Target->mParameters, [](const Asset::FAnimGraphParameter& Parameter) {
                        return Parameter.mName.ends_with(".Binding");
                    })};

                    mDirty |= Removed > 0;
                }
            }

            ImGui::EndPopup();
        }

        ed::Resume();

        std::array<ed::NodeId, 2> Selected{};
        const int SelectedCount{ed::GetSelectedNodes(Selected.data(), static_cast<int>(Selected.size()))};

        if (SelectedCount == 1) {
            mSelectedNode = DecodeNode(Selected[0].Get());
        } else if (SelectedCount == 0 && !ImGui::IsPopupOpen("AnimGraphNodeMenu")) {
            mSelectedNode = -2;
        }

        // 사용자가 옮긴 노드 위치를 편집 사본에 반영합니다. 위치를 적용하는 프레임에는 아직 노드 에디터 값이 이전 상태이므로 건너뜁니다.
        if (!mApplyPositions) {
            const ImVec2 Output{ed::GetNodePosition(ed::NodeId{OutputNode})};

            if (std::abs(Output.x - mEditing->GetOutputX()) > 0.5f || std::abs(Output.y - mEditing->GetOutputY()) > 0.5f) {
                mEditing->SetNodePosition(Asset::UAnimGraph::OutputNodeId, Output.x, Output.y);
                mDirty = true;
            }

            for (const Asset::FAnimGraphNode& Node : mEditing->GetNodes()) {
                const ImVec2 Position{ed::GetNodePosition(ed::NodeId{NodeKey(Node.mId)})};

                if (std::isfinite(Position.x) && (std::abs(Position.x - Node.mX) > 0.5f || std::abs(Position.y - Node.mY) > 0.5f)) {
                    mEditing->SetNodePosition(Node.mId, Position.x, Position.y);
                    mDirty = true;
                }
            }

            for (const Asset::FAnimGraphVariableGetter& Getter : mEditing->GetVariableGetters()) {
                const ImVec2 Position{ed::GetNodePosition(ed::NodeId{GetterKey(Getter.mId)})};

                if (std::abs(Position.x - Getter.mX) > 0.5f || std::abs(Position.y - Getter.mY) > 0.5f) {
                    mDirty |= mEditing->SetVariableGetterPosition(Getter.mId, Position.x, Position.y);
                }
            }
        } else {
            mApplyPositions = false;
            mFitRequested = true;
        }

        const std::uintptr_t DoubleClickedKey{ed::GetDoubleClickedNode().Get()};
        const Core::Int32 DoubleClicked{DecodeNode(DoubleClickedKey)};
        const auto* OpenNode{mEditing->FindNode(DoubleClicked)};
        const bool OpenMachine{(DoubleClickedKey & GetterBit) == 0 && OpenNode != nullptr && OpenNode->mType == "StateMachine"};
        ed::End();
        ed::SetCurrentEditor(nullptr);

        if (OpenMachineId >= 0 || OpenMachine) {
            OpenStateMachine(OpenMachineId >= 0 ? OpenMachineId : DoubleClicked);
        }
    }

    void FAnimGraphEditor::DrawSelection() {
        ImGui::SeparatorText("Node Properties");

        Asset::FAnimGraphNode* Node{mEditing->FindNode(mSelectedNode)};

        if (Node == nullptr) {
            return;
        }

        const Engine::FAnimGraphNodeType* Type{Engine::FAnimGraphNodeRegistry::Find(Node->mType)};

        ImGui::PushID(Node->mId);
        ImGui::TextDisabled("%s", Type != nullptr ? Type->mTitle.c_str() : Node->mType.c_str());

        std::array<char, 64> Name{};

        std::copy_n(Node->mName.begin(), std::min(Node->mName.size(), Name.size() - 1), Name.begin());

        if (ImGui::InputText("Name", Name.data(), Name.size())) {
            Node->mName = Name.data();
            mDirty = true;
        }

        if (Type == nullptr) {
            ImGui::PopID();

            return;
        }

        if (!Type->mParameters.empty()) {
            ImGui::SeparatorText("Parameters");
        }

        for (const Engine::FAnimGraphParameterInfo& Info : Type->mParameters) {
            const Core::FString* Stored{Node->FindParameter(Info.mName)};
            Core::FString Value{Stored != nullptr ? *Stored : Info.mDefault};

            ImGui::PushID(Info.mName.c_str());

            switch (Info.mType) {
                case Engine::EAnimGraphParameterType::Float: {
                    const Core::FString BindingKey{Info.mName + ".Binding"};
                    const Core::FString* Binding{Node->FindParameter(BindingKey)};
                    float Number{};

                    std::from_chars(Value.data(), Value.data() + Value.size(), Number);

                    if (Info.mBindable) {
                        ImGui::SetNextItemWidth(110.0f);

                        if (ImGui::BeginCombo("##Binding", Binding != nullptr ? Binding->c_str() : "(값)")) {
                            const Core::FString SelectedBinding{Binding != nullptr ? *Binding : Core::FString{}};

                            if (ImGui::Selectable("(값)", Binding == nullptr)) {
                                Node->SetParameter(BindingKey, {});
                                mDirty = true;
                            }

                            for (const Asset::FAnimGraphVariable& Variable : mEditing->GetVariables()) {
                                if (Variable.mType == Asset::EAnimGraphVariableType::Float && ImGui::Selectable(Variable.mName.c_str(), SelectedBinding == Variable.mName) && SelectedBinding != Variable.mName) {
                                    Node->SetParameter(BindingKey, Variable.mName);
                                    mDirty = true;
                                }
                            }

                            ImGui::EndCombo();
                        }

                        Binding = Node->FindParameter(BindingKey);
                        ImGui::SameLine();
                    }

                    ImGui::BeginDisabled(Binding != nullptr);
                    ImGui::SetNextItemWidth(-ImGui::CalcTextSize(Info.mName.c_str()).x - ImGui::GetStyle().ItemInnerSpacing.x);

                    const bool Edited{Info.mMin < Info.mMax ? ImGui::SliderFloat(Info.mName.c_str(), &Number, Info.mMin, Info.mMax, "%.3f") : ImGui::DragFloat(Info.mName.c_str(), &Number, 0.01f, 0.0f, 0.0f, "%.3f")};

                    if (Edited && std::isfinite(Number)) {
                        Node->SetParameter(Info.mName, FormatFloat(Number));
                        mDirty = true;
                    }

                    ImGui::EndDisabled();

                    break;
                }

                case Engine::EAnimGraphParameterType::Bool: {
                    bool Flag{Value == "true"};

                    if (ImGui::Checkbox(Info.mName.c_str(), &Flag)) {
                        Node->SetParameter(Info.mName, Flag ? "true" : "false");
                        mDirty = true;
                    }

                    break;
                }

                case Engine::EAnimGraphParameterType::AnimSequence:
                case Engine::EAnimGraphParameterType::BlendSpace: {
                    const Asset::EAssetType AssetType{Info.mType == Engine::EAnimGraphParameterType::AnimSequence ? Asset::EAssetType::AnimSequence : Asset::EAssetType::BlendSpace};

                    if (mCatalog != nullptr && mThumbnails != nullptr && mPicker.Draw(*mCatalog, *mThumbnails, Info.mName.c_str(), Value, AssetType, {}, "None")) {
                        Node->SetParameter(Info.mName, Value);
                        mDirty = true;
                    }

                    break;
                }
            }

            ImGui::PopID();
        }

        ImGui::PopID();
    }

}
