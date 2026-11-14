/**
 * @file FAnimGraphStateMachineEditor.cpp
 * @brief 상태 머신과 전이 조건의 노드 편집 UI를 구현합니다.
 * @ingroup Editor
 */

#include "Core/Types.h"
#include "FAnimGraphEditor.h"
#include "FAnimGraphNodeBuilder.h"
#include "Asset/Animation/UAnimGraph.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_node_editor.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <utility>

namespace Editor {

    namespace ed = ax::NodeEditor;

    /**
     * @brief 상태 머신과 전이 조건의 노드 편집 UI를 구현하는 파일 내부 보조 구현입니다.
     */
    namespace {

        /**
         * @brief Bool 조건 핀의 표시 색상입니다.
         */
        constexpr ImVec4 BoolColor{0.82f, 0.24f, 0.28f, 1.0f};

        /**
         * @brief 상태 노드의 표시 색상입니다.
         */
        constexpr ImVec4 StateColor{0.45f, 0.72f, 1.0f, 1.0f};

        /**
         * @brief 상태 ID를 상태 편집기 노드 키로 인코딩합니다.
         * @param Id 조회 또는 변경할 항목 ID입니다.
         * @return 상태를 식별하는 노드 키입니다.
         */
        std::uintptr_t StateNodeKey(Core::Int32 Id) {
            return (static_cast<std::uintptr_t>(Id) + 2) * 16;
        }

        /**
         * @brief 상태 편집기 키에서 상태 ID를 복원합니다.
         * @param Key 해석할 노드 편집기 식별 키입니다.
         * @return 원본 상태 ID입니다.
         */
        Core::Int32 StateNodeId(std::uintptr_t Key) {
            return Key >= 32 ? static_cast<Core::Int32>(Key / 16 - 2) : -1;
        }

        /**
         * @brief 문자열을 ImGui 입력 버퍼로 편집하고 변경 결과를 반영합니다.
         * @param Label UI 항목의 표시 이름입니다.
         * @param Value 표시·비교하거나 적용할 값입니다.
         * @return 입력 문자열이 변경되었으면 true입니다.
         */
        bool EditString(const char* Label, Core::FString& Value) {
            std::array<char, 256> Buffer{};
            std::copy_n(Value.begin(), std::min(Value.size(), Buffer.size() - 1), Buffer.begin());

            if (ImGui::InputText(Label, Buffer.data(), Buffer.size())) {
                Value = Buffer.data();

                return true;
            }

            return false;
        }
    }

    void FAnimGraphEditor::ClearGraphContexts() {
        for (const auto& [Key, Context] : mGraphContexts) {
            ed::DestroyEditor(Context);
        }

        mGraphContexts.clear();
        mEditor = nullptr;
    }

    void FAnimGraphEditor::ActivateView(const FGraphView& View) {
        mView = View;
        mEditing = View.mGraph;
        mSelectedNode = View.mSelection;
        mContextNode = 0;
        const Core::FString Key{View.mKey};
        auto Found{mGraphContexts.find(Key)};
        const bool Created{Found == mGraphContexts.end()};

        if (Created) {
            ed::Config Config{};
            Config.SettingsFile = nullptr;
            Found = mGraphContexts.emplace(Key, ed::CreateEditor(&Config)).first;
        }

        mEditor = Found->second;
        mApplyPositions = Created;
        mFitRequested = Created;
    }

    void FAnimGraphEditor::Navigate(Asset::UAnimGraph* Graph, EGraphView Mode, Core::Int32 Machine, Core::Int32 Transition, const Core::FString& Label) {
        mView.mSelection = mSelectedNode;
        mNavigation.push_back(mView);
        FGraphView View{};
        View.mGraph = Graph;
        View.mMode = Mode;
        View.mMachine = Machine;
        View.mTransition = Transition;
        View.mLabel = Label;
        View.mKey = std::format("{}/{}:{}:{}", mView.mKey, static_cast<int>(Mode), Machine, Transition);
        ActivateView(View);
    }

    void FAnimGraphEditor::DrawNavigation() {
        Core::Int32 Destination{-1};
        const float Height{ImGui::GetFrameHeight() + ImGui::GetStyle().ScrollbarSize + 4.0f};

        if (ImGui::BeginChild("GraphNavigation", {0.0f, Height}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
            for (Core::Index_t Index{}; Index <= mNavigation.size(); ++Index) {
                const bool Current{Index == mNavigation.size()};
                const FGraphView& View{Current ? mView : mNavigation[Index]};
                const Core::FString Label{GetViewLabel(View, Index > 0 ? &mNavigation[Index - 1] : nullptr)};

                if (Index > 0) {
                    ImGui::SameLine();
                    ImGui::TextDisabled(">");
                    ImGui::SameLine();
                }

                ImGui::PushID(Index);
                ImGui::BeginDisabled(Current);

                if (ImGui::Button(Label.c_str())) {
                    Destination = static_cast<Core::Int32>(Index);
                }

                ImGui::EndDisabled();
                ImGui::PopID();
            }
        }

        ImGui::EndChild();

        if (Destination >= 0) {
            NavigateTo(static_cast<std::size_t>(Destination));
        }
    }

    Core::FString FAnimGraphEditor::GetViewLabel(const FGraphView& View, const FGraphView* Parent) const {
        if (View.mGraph != nullptr && View.mMode == EGraphView::Machine) {
            if (const auto* Node{View.mGraph->FindNode(View.mMachine)}) {
                return Node->mName.empty() ? Core::FString{"State Machine"} : Node->mName;
            }
        } else if (View.mGraph != nullptr && View.mMode == EGraphView::Rule) {
            const auto* Machine{View.mGraph->FindStateMachine(View.mMachine)};
            const auto* Transition{Machine != nullptr ? Machine->FindTransition(View.mTransition) : nullptr};
            const auto* From{Transition != nullptr ? Machine->FindState(Transition->mFromState) : nullptr};
            const auto* To{Transition != nullptr ? Machine->FindState(Transition->mToState) : nullptr};

            if (From != nullptr && To != nullptr) {
                return std::format("{} -> {}", From->mName, To->mName);
            }
        } else if (Parent != nullptr && Parent->mGraph != nullptr && Parent->mMode == EGraphView::Machine) {
            const auto* Machine{Parent->mGraph->FindStateMachine(Parent->mMachine)};

            if (const auto* State{Machine != nullptr ? Machine->FindState(View.mMachine) : nullptr}) {
                return State->mName;
            }
        }

        return View.mLabel;
    }

    bool FAnimGraphEditor::OpenStateMachine(Core::Int32 NodeId) {
        if (mEditing == nullptr || mView.mMode != EGraphView::Pose || mEditing->FindStateMachine(NodeId) == nullptr) {
            return false;
        }

        Navigate(mEditing, EGraphView::Machine, NodeId, -1, mEditing->FindNode(NodeId)->mName);

        return true;
    }

    bool FAnimGraphEditor::OpenState(Core::Int32 StateId) {
        auto* Machine{mEditing != nullptr && mView.mMode == EGraphView::Machine ? mEditing->FindStateMachine(mView.mMachine) : nullptr};
        auto* State{Machine != nullptr ? Machine->FindState(StateId) : nullptr};

        if (State == nullptr || State->mGraph == nullptr) {
            return false;
        }

        Navigate(State->mGraph.get(), EGraphView::Pose, StateId, -1, State->mName);

        return true;
    }

    bool FAnimGraphEditor::OpenTransitionRule(Core::Int32 TransitionId) {
        auto* Machine{mEditing != nullptr && mView.mMode == EGraphView::Machine ? mEditing->FindStateMachine(mView.mMachine) : nullptr};

        if (Machine == nullptr || Machine->FindTransition(TransitionId) == nullptr) {
            return false;
        }

        Navigate(mEditing, EGraphView::Rule, mView.mMachine, TransitionId, "Transition Rule");

        return true;
    }

    bool FAnimGraphEditor::NavigateBack() {
        return !mNavigation.empty() && NavigateTo(mNavigation.size() - 1);
    }

    bool FAnimGraphEditor::NavigateTo(Core::Index_t Depth) {
        if (Depth >= mNavigation.size()) {
            return false;
        }

        const FGraphView View{mNavigation[Depth]};
        mNavigation.resize(Depth);
        ActivateView(View);

        return true;
    }

    void FAnimGraphEditor::DrawStateMachine() {
        auto* Machine{mEditing->FindStateMachine(mView.mMachine)};

        if (Machine == nullptr) {
            return;
        }

        const auto AddState{[this, Machine]() {
            Core::Uint32 Number{static_cast<Core::Uint32>(Machine->GetStates().size())};
            Core::FString Name{};

            do {
                Name = std::format("State {}", Number++);
            } while (std::any_of(Machine->GetStates().begin(), Machine->GetStates().end(), [&Name](const auto& State) {
                return State.mName == Name;
            }));

            const Core::Int32 Id{Machine->AddState(Name, mCreateX, mCreateY, *mRootEditing)};

            if (Id >= 0) {
                ed::SetNodePosition(ed::NodeId{StateNodeKey(Id)}, {mCreateX, mCreateY});
                mSelectedNode = Id;
                mDirty = true;
            }
        }};

        ed::SetCurrentEditor(mEditor);

        if (ImGui::Button("+ State")) {
            mCreateX = static_cast<float>(Machine->GetStates().size()) * 220.0f;
            mCreateY = 0.0f;
            AddState();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Double-click a state or transition to open it");
        Editor::FAnimGraphNodeBuilder::ApplyStyle();
        ed::Begin("StateMachineCanvas");

        if (mApplyPositions) {
            ed::SetNodePosition(ed::NodeId{1}, {-180.0f, 0.0f});

            for (const auto& State : Machine->GetStates()) {
                ed::SetNodePosition(ed::NodeId{StateNodeKey(State.mId)}, {State.mX, State.mY});
            }
        }

        Editor::FAnimGraphNodeBuilder Entry{};
        Entry.FitHeader("Entry", "Entry");
        Entry.FitPins("", "State");
        Entry.Begin(ed::NodeId{1}, 120.0f);
        Entry.Header("Entry", "Entry");
        Entry.OutputPin(ed::PinId{2}, "State", Machine->GetEntryState() >= 0, StateColor, false);
        Entry.End();

        for (const auto& State : Machine->GetStates()) {
            const std::uintptr_t Key{StateNodeKey(State.mId)};
            Editor::FAnimGraphNodeBuilder Builder{};
            Builder.FitHeader(State.mName.c_str(), "State");
            Builder.FitPins("In", "Out");
            Builder.Begin(ed::NodeId{Key}, 170.0f);
            Builder.Header(State.mName.c_str(), "State");
            Builder.InputPin(ed::PinId{Key + 1}, "In", true, StateColor);
            Builder.OutputPin(ed::PinId{Key + 2}, "Out", true, StateColor, true);
            Builder.End();
        }

        if (Machine->GetEntryState() >= 0) {
            ed::Link(ed::LinkId{1}, ed::PinId{2}, ed::PinId{StateNodeKey(Machine->GetEntryState()) + 1}, StateColor, 2.5f);
        }

        for (const auto& Transition : Machine->GetTransitions()) {
            ed::Link(ed::LinkId{static_cast<std::uintptr_t>(Transition.mId) + 2}, ed::PinId{StateNodeKey(Transition.mFromState) + 2}, ed::PinId{StateNodeKey(Transition.mToState) + 1}, StateColor, 2.0f);
        }

        if (ed::BeginCreate()) {
            ed::PinId A{};
            ed::PinId B{};

            if (ed::QueryNewLink(&A, &B) && A && B) {
                if (A.Get() % 16 != 2) {
                    std::swap(A, B);
                }

                const bool Valid{A.Get() % 16 == 2 && B.Get() % 16 == 1 && Machine->FindState(StateNodeId(B.Get())) != nullptr && (A.Get() == 2 || Machine->FindState(StateNodeId(A.Get())) != nullptr)};

                if (!Valid) {
                    ed::RejectNewItem(BoolColor, 2.0f);
                } else if (ed::AcceptNewItem(StateColor, 2.0f)) {
                    mDirty |= A.Get() == 2 ? Machine->SetEntryState(StateNodeId(B.Get())) : Machine->AddTransition(StateNodeId(A.Get()), StateNodeId(B.Get())) >= 0;
                }
            }
        }

        ed::EndCreate();

        if (ed::BeginDelete()) {
            ed::NodeId Node{};
            ed::LinkId Link{};

            while (ed::QueryDeletedNode(&Node)) {
                if (Node.Get() == 1) {
                    ed::RejectDeletedItem();
                } else if (ed::AcceptDeletedItem()) {
                    mDirty |= Machine->RemoveState(StateNodeId(Node.Get()));
                    mSelectedNode = -2;
                }
            }

            while (ed::QueryDeletedLink(&Link)) {
                if (Link.Get() == 1) {
                    ed::RejectDeletedItem();
                } else if (ed::AcceptDeletedItem()) {
                    mDirty |= Machine->RemoveTransition(static_cast<Core::Int32>(Link.Get() - 2));
                    mView.mTransition = -1;
                }
            }
        }

        ed::EndDelete();
        std::array<ed::NodeId, 1> SelectedNodes{};
        std::array<ed::LinkId, 1> SelectedLinks{};

        if (ed::GetSelectedNodes(SelectedNodes.data(), 1) == 1) {
            mSelectedNode = StateNodeId(SelectedNodes[0].Get());
            mView.mTransition = -1;
        } else if (ed::GetSelectedLinks(SelectedLinks.data(), 1) == 1 && SelectedLinks[0].Get() >= 2) {
            mSelectedNode = -2;
            mView.mTransition = static_cast<Core::Int32>(SelectedLinks[0].Get() - 2);
        }

        const Core::Int32 DoubleClickedState{StateNodeId(ed::GetDoubleClickedNode().Get())};
        const std::uintptr_t OpenLink{ed::GetDoubleClickedLink().Get()};
        const ImVec2 Mouse{ImGui::GetMousePos()};
        ed::NodeId ContextNode{};
        ed::LinkId ContextLink{};
        Core::Int32 OpenStateId{-1};
        Core::Int32 OpenTransitionId{-1};
        ed::Suspend();

        if (ed::ShowNodeContextMenu(&ContextNode)) {
            mContextNode = ContextNode.Get();
            ed::SelectNode(ContextNode);
            ImGui::OpenPopup("AnimStateMenu");
        } else if (ed::ShowLinkContextMenu(&ContextLink) && ContextLink.Get() >= 2) {
            mContextNode = ContextLink.Get();
            ed::SelectLink(ContextLink);
            ImGui::OpenPopup("AnimTransitionMenu");
        } else if (ed::ShowBackgroundContextMenu()) {
            mCreateX = Mouse.x;
            mCreateY = Mouse.y;
            ImGui::OpenPopup("CreateState");
        }

        if (ImGui::BeginPopup("CreateState")) {
            if (ImGui::MenuItem("Add State")) {
                AddState();
            }

            ImGui::EndPopup();
        }

        ImGui::SetNextWindowSizeConstraints({360.0f, 0.0f}, {500.0f, ImGui::GetIO().DisplaySize.y});

        if (ImGui::BeginPopup("AnimStateMenu")) {
            mSelectedNode = StateNodeId(mContextNode);
            mView.mTransition = -1;
            ImGui::PushItemWidth(220.0f);
            DrawMachineSelection();
            ImGui::PopItemWidth();

            if (Machine->FindState(mSelectedNode) != nullptr) {
                ImGui::Separator();

                if (ImGui::MenuItem("Open State Graph")) {
                    OpenStateId = mSelectedNode;
                }

                if (ImGui::MenuItem("Delete State", "Delete")) {
                    mDirty |= Machine->RemoveState(mSelectedNode);
                    mSelectedNode = -2;
                }
            } else {
                ImGui::TextDisabled("Connect Entry to the initial state.");
            }

            ImGui::EndPopup();
        }

        ImGui::SetNextWindowSizeConstraints({360.0f, 0.0f}, {500.0f, ImGui::GetIO().DisplaySize.y});

        if (ImGui::BeginPopup("AnimTransitionMenu")) {
            mSelectedNode = -2;
            mView.mTransition = static_cast<Core::Int32>(mContextNode - 2);
            ImGui::PushItemWidth(220.0f);
            DrawMachineSelection();
            ImGui::PopItemWidth();
            ImGui::Separator();

            if (ImGui::MenuItem("Open Transition Rule")) {
                OpenTransitionId = mView.mTransition;
            }

            if (ImGui::MenuItem("Delete Transition", "Delete")) {
                mDirty |= Machine->RemoveTransition(mView.mTransition);
                mView.mTransition = -1;
            }

            ImGui::EndPopup();
        }

        ed::Resume();

        if (!mApplyPositions) {
            for (const auto& State : Machine->GetStates()) {
                const ImVec2 Position{ed::GetNodePosition(ed::NodeId{StateNodeKey(State.mId)})};

                if (std::isfinite(Position.x) && std::isfinite(Position.y) && (std::abs(Position.x - State.mX) > 0.5f || std::abs(Position.y - State.mY) > 0.5f)) {
                    auto* Data{Machine->FindState(State.mId)};
                    Data->mX = Position.x;
                    Data->mY = Position.y;
                    mDirty = true;
                }
            }
        }

        if (mFitRequested) {
            ed::NavigateToContent();
            mFitRequested = false;
        }

        mApplyPositions = false;
        ed::End();
        ed::SetCurrentEditor(nullptr);

        const Core::Int32 StateId{OpenStateId >= 0 ? OpenStateId : DoubleClickedState};
        const Core::Int32 TransitionId{OpenTransitionId >= 0 ? OpenTransitionId : (OpenLink >= 2 ? static_cast<Core::Int32>(OpenLink - 2) : -1)};

        if (auto* State{Machine->FindState(StateId)}; State != nullptr && State->mGraph != nullptr) {
            OpenState(StateId);
        } else if (Machine->FindTransition(TransitionId) != nullptr) {
            OpenTransitionRule(TransitionId);
        }
    }

    void FAnimGraphEditor::DrawMachineSelection() {
        auto* Machine{mEditing->FindStateMachine(mView.mMachine)};

        if (Machine == nullptr) {
            return;
        }

        if (auto* State{Machine->FindState(mSelectedNode)}) {
            ImGui::SeparatorText("State");
            Core::FString Name{State->mName};

            if (EditString("Name", Name) && !Name.empty() && std::none_of(Machine->GetStates().begin(), Machine->GetStates().end(), [State, &Name](const auto& Other) {
                return Other.mId != State->mId && Other.mName == Name;
            })) {
                State->mName = Name;
                mDirty = true;
            }

            mDirty |= ImGui::Checkbox("Reset on Entry", &State->mResetOnEntry);

            if (ImGui::Button("Set as Entry")) {
                mDirty |= Machine->SetEntryState(State->mId);
            }
        } else if (auto* Transition{Machine->FindTransition(mView.mTransition)}) {
            ImGui::SeparatorText("Transition");
            const auto* From{Machine->FindState(Transition->mFromState)};
            const auto* To{Machine->FindState(Transition->mToState)};
            ImGui::Text("%s -> %s", From != nullptr ? From->mName.c_str() : "?", To != nullptr ? To->mName.c_str() : "?");
            mDirty |= ImGui::DragFloat("Blend Duration", &Transition->mDuration, 0.01f, 0.0f, 10.0f, "%.3f s", ImGuiSliderFlags_AlwaysClamp);
            mDirty |= ImGui::InputInt("Priority", &Transition->mPriority);
            ImGui::TextDisabled("Lower priority values are checked first");
        }
    }

    void FAnimGraphEditor::DrawTransitionRule() {
        auto* Machine{mEditing->FindStateMachine(mView.mMachine)};
        auto* Transition{Machine != nullptr ? Machine->FindTransition(mView.mTransition) : nullptr};

        if (Transition == nullptr) {
            return;
        }

        auto& Graph{Transition->mRule};
        ed::SetCurrentEditor(mEditor);
        Editor::FAnimGraphNodeBuilder::ApplyStyle();
        const ImVec2 Origin{ImGui::GetCursorScreenPos()};
        const ImVec2 Size{ImGui::GetContentRegionAvail()};
        ed::Begin("TransitionRuleCanvas");

        if (mApplyPositions) {
            ed::SetNodePosition(ed::NodeId{1}, {450.0f, 0.0f});

            for (const auto& Node : Graph.GetNodes()) {
                ed::SetNodePosition(ed::NodeId{StateNodeKey(Node.mId)}, {Node.mX, Node.mY});
            }
        }

        Editor::FAnimGraphNodeBuilder Result{};
        Result.FitHeader("Result", "Can Enter Transition");
        Result.FitPins("Can Enter");
        Result.Begin(ed::NodeId{1}, 180.0f);
        Result.Header("Result", "Can Enter Transition");
        Result.InputPin(ed::PinId{1}, "Can Enter", Graph.GetResult() >= 0, BoolColor);
        Result.End();

        for (const auto& Node : Graph.GetNodes()) {
            const std::uintptr_t Key{StateNodeKey(Node.mId)};
            const char* Title{Asset::FAnimTransitionGraph::GetTitle(Node.mType)};
            const bool BoolOutput{Asset::FAnimTransitionGraph::IsBool(Node.mType)};
            const Core::Int32 InputCount{Asset::FAnimTransitionGraph::GetInputCount(Node.mType)};
            const ImVec4 InputColor{Asset::FAnimTransitionGraph::IsBoolInput(Node.mType) ? BoolColor : Editor::FAnimGraphNodeBuilder::FloatColor()};
            Editor::FAnimGraphNodeBuilder Builder{};
            Builder.FitHeader(Title, Title);
            Builder.FitPins(InputCount > 0 ? "A" : "", BoolOutput ? "Bool" : "Float");
            Builder.FitText(Node.mName.c_str());
            Builder.Begin(ed::NodeId{Key}, 160.0f);
            Builder.Header(Title, Title);

            if (InputCount > 0) {
                Builder.InputPin(ed::PinId{Key + 1}, "A", Node.mInputA >= 0, InputColor);
            }

            Builder.OutputPin(ed::PinId{Key + 3}, BoolOutput ? "Bool" : "Float", true, BoolOutput ? BoolColor : Editor::FAnimGraphNodeBuilder::FloatColor(), InputCount > 0);

            if (InputCount > 1) {
                Builder.InputPin(ed::PinId{Key + 2}, "B", Node.mInputB >= 0, InputColor);
            }

            Builder.BeginBody();

            if (Node.mType == Asset::EAnimRuleNodeType::Float || Node.mType == Asset::EAnimRuleNodeType::Bool) {
                const Core::FString Value{Node.mType == Asset::EAnimRuleNodeType::Bool ? Core::FString{Node.mBoolValue ? "true" : "false"} : std::format("{:.3f}", Node.mValue)};
                Builder.DrawText(Value.c_str(), BoolOutput ? BoolColor : Editor::FAnimGraphNodeBuilder::FloatColor());
            } else if (!Node.mName.empty()) {
                Builder.DrawText(Node.mName.c_str(), StateColor);
            }

            Builder.End();
        }

        for (const auto& Node : Graph.GetNodes()) {
            const std::uintptr_t Key{StateNodeKey(Node.mId)};
            const ImVec4 InputColor{Asset::FAnimTransitionGraph::IsBoolInput(Node.mType) ? BoolColor : Editor::FAnimGraphNodeBuilder::FloatColor()};
            const Core::Int32 InputCount{Asset::FAnimTransitionGraph::GetInputCount(Node.mType)};

            for (Core::Index_t Pin{}; Pin < InputCount; ++Pin) {
                const Core::Int32 Input{Pin == 0 ? Node.mInputA : Node.mInputB};

                if (Input >= 0) {
                    ed::Link(ed::LinkId{Key + Pin + 1}, ed::PinId{StateNodeKey(Input) + 3}, ed::PinId{Key + Pin + 1}, InputColor, 2.0f);
                }
            }
        }

        if (Graph.GetResult() >= 0) {
            ed::Link(ed::LinkId{1}, ed::PinId{StateNodeKey(Graph.GetResult()) + 3}, ed::PinId{1}, BoolColor, 2.0f);
        }

        if (ed::BeginCreate()) {
            ed::PinId A{};
            ed::PinId B{};

            if (ed::QueryNewLink(&A, &B) && A && B) {
                if (A.Get() % 16 != 3) {
                    std::swap(A, B);
                }

                const Core::Int32 From{StateNodeId(A.Get())};
                const Core::Int32 To{StateNodeId(B.Get())};
                const Core::Index_t Pin{static_cast<Core::Index_t>(static_cast<Core::Int32>(B.Get() % 16) - 1)};
                auto Candidate{Graph};
                const bool Valid{A.Get() % 16 == 3 && (B.Get() == 1 ? Candidate.SetResult(From) : Candidate.Connect(From, To, Pin))};

                if (!Valid) {
                    ed::RejectNewItem(BoolColor, 2.0f);
                } else if (ed::AcceptNewItem(StateColor, 2.0f)) {
                    mDirty |= B.Get() == 1 ? Graph.SetResult(From) : Graph.Connect(From, To, Pin);
                }
            }
        }

        ed::EndCreate();

        if (ed::BeginDelete()) {
            ed::NodeId Node{};
            ed::LinkId Link{};

            while (ed::QueryDeletedNode(&Node)) {
                if (Node.Get() == 1) {
                    ed::RejectDeletedItem();
                } else if (ed::AcceptDeletedItem()) {
                    mDirty |= Graph.RemoveNode(StateNodeId(Node.Get()));
                }
            }

            while (ed::QueryDeletedLink(&Link)) {
                if (ed::AcceptDeletedItem()) {
                    mDirty |= Link.Get() == 1 ? Graph.SetResult(-1) : Graph.Disconnect(StateNodeId(Link.Get()), static_cast<Core::Int32>(Link.Get() % 16) - 1);
                }
            }
        }

        ed::EndDelete();
        std::array<ed::NodeId, 1> Selected{};
        mSelectedNode = ed::GetSelectedNodes(Selected.data(), 1) == 1 ? StateNodeId(Selected[0].Get()) : -2;
        const ImVec2 Mouse{ImGui::GetMousePos()};
        ed::NodeId ContextNode{};
        ed::Suspend();

        if (ed::ShowNodeContextMenu(&ContextNode)) {
            mContextNode = ContextNode.Get();
            ed::SelectNode(ContextNode);
            ImGui::OpenPopup("AnimRuleNodeMenu");
        } else if (ed::ShowBackgroundContextMenu()) {
            mCreateX = Mouse.x;
            mCreateY = Mouse.y;
            ImGui::OpenPopup("CreateRuleNode");
        }

        if (ImGui::BeginPopup("CreateRuleNode")) {
            for (Core::Index_t Index{}; Index < static_cast<Core::Uint32>(Asset::EAnimRuleNodeType::Count); ++Index) {
                const auto Type{static_cast<Asset::EAnimRuleNodeType>(Index)};

                if (ImGui::MenuItem(Asset::FAnimTransitionGraph::GetTitle(Type))) {
                    const Core::Int32 Id{Graph.AddNode(Type, mCreateX, mCreateY)};

                    if (Id >= 0) {
                        ed::SetNodePosition(ed::NodeId{StateNodeKey(Id)}, Mouse);
                        mDirty = true;
                    }
                }
            }

            ImGui::EndPopup();
        }

        ImGui::SetNextWindowSizeConstraints({360.0f, 0.0f}, {500.0f, ImGui::GetIO().DisplaySize.y});

        if (ImGui::BeginPopup("AnimRuleNodeMenu")) {
            mSelectedNode = StateNodeId(mContextNode);
            ImGui::PushItemWidth(220.0f);
            DrawRuleSelection();
            ImGui::PopItemWidth();
            ImGui::Separator();

            if (ImGui::MenuItem("Delete Node", "Delete", false, Graph.FindNode(mSelectedNode) != nullptr)) {
                mDirty |= Graph.RemoveNode(mSelectedNode);
                mSelectedNode = -2;
            }

            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropTargetCustom(ImRect{Origin, {Origin.x + Size.x, Origin.y + Size.y}}, ImGui::GetID("RuleVariableDrop"))) {
            if (const ImGuiPayload* Payload{ImGui::AcceptDragDropPayload("MACAW_ANIM_GRAPH_VARIABLE")}; Payload != nullptr && Payload->Data != nullptr && Payload->DataSize > 1 && static_cast<const char*>(Payload->Data)[Payload->DataSize - 1] == '\0') {
                const Core::FString Name{static_cast<const char*>(Payload->Data)};
                const auto& Variables{mRootEditing->GetVariables()};
                const auto Found{std::find_if(Variables.begin(), Variables.end(), [&Name](const auto& Variable) {
                    return Variable.mName == Name;
                })};

                if (Found != Variables.end()) {
                    const Core::Int32 Id{Graph.AddNode(Found->mType == Asset::EAnimGraphVariableType::Bool ? Asset::EAnimRuleNodeType::GetBool : Asset::EAnimRuleNodeType::GetFloat, Mouse.x, Mouse.y)};

                    if (auto* Node{Graph.FindNode(Id)}) {
                        Node->mName = Name;
                        ed::SetNodePosition(ed::NodeId{StateNodeKey(Id)}, Mouse);
                        mDirty = true;
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }

        ed::Resume();

        if (!mApplyPositions) {
            for (const auto& Node : Graph.GetNodes()) {
                const ImVec2 Position{ed::GetNodePosition(ed::NodeId{StateNodeKey(Node.mId)})};

                if (std::isfinite(Position.x) && std::isfinite(Position.y) && (std::abs(Position.x - Node.mX) > 0.5f || std::abs(Position.y - Node.mY) > 0.5f)) {
                    auto* Data{Graph.FindNode(Node.mId)};
                    Data->mX = Position.x;
                    Data->mY = Position.y;
                    mDirty = true;
                }
            }
        }

        if (mFitRequested) {
            ed::NavigateToContent();
            mFitRequested = false;
        }

        mApplyPositions = false;
        ed::End();
        ed::SetCurrentEditor(nullptr);
    }

    void FAnimGraphEditor::DrawRuleSelection() {
        auto* Machine{mEditing->FindStateMachine(mView.mMachine)};
        auto* Transition{Machine != nullptr ? Machine->FindTransition(mView.mTransition) : nullptr};
        auto* Node{Transition != nullptr ? Transition->mRule.FindNode(mSelectedNode) : nullptr};

        if (Node == nullptr) {
            ImGui::TextWrapped("Connect a Bool output to Can Enter Transition. An unconnected result is false.");

            return;
        }

        ImGui::SeparatorText(Asset::FAnimTransitionGraph::GetTitle(Node->mType));

        if (Node->mType == Asset::EAnimRuleNodeType::Float) {
            mDirty |= ImGui::DragFloat("Value", &Node->mValue, 0.01f);
        } else if (Node->mType == Asset::EAnimRuleNodeType::Bool) {
            mDirty |= ImGui::Checkbox("Value", &Node->mBoolValue);
        } else if (Node->mType == Asset::EAnimRuleNodeType::GetFloat || Node->mType == Asset::EAnimRuleNodeType::GetBool) {
            if (ImGui::BeginCombo("Variable", Node->mName.empty() ? "Select Variable" : Node->mName.c_str())) {
                for (const auto& Variable : mRootEditing->GetVariables()) {
                    if ((Variable.mType == Asset::EAnimGraphVariableType::Bool) == (Node->mType == Asset::EAnimRuleNodeType::GetBool) && ImGui::Selectable(Variable.mName.c_str(), Variable.mName == Node->mName)) {
                        Node->mName = Variable.mName;
                        mDirty = true;
                    }
                }

                ImGui::EndCombo();
            }
        } else if (Node->mType == Asset::EAnimRuleNodeType::TimeRemaining || Node->mType == Asset::EAnimRuleNodeType::Finished) {
            mDirty |= EditString("Player Name", Node->mName);
            ImGui::TextWrapped("Leave empty when the source state contains a single animation player. Time Remaining uses seconds at the current play rate.");
        }
    }

}
