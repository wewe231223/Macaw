#include "Editor/Panel/FAnimGraphPanel.h"
#include "Editor/FEditorContext.h"
#include "Engine/AActor.h"
#include "Engine/Animation/FAnimNode_Base.h"
#include "Engine/Animation/UAnimGraphInstance.h"
#include "Engine/Animation/UAnimInstance.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "FAnimGraphNodeBuilder.h"

#include <imgui.h>
#include <imgui_node_editor.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>

namespace ed = ax::NodeEditor;

namespace Editor {

    namespace {
        // Output Pose 노드, 그 입력 핀, 루트에서 들어오는 링크는 다른 식별자와 겹치지 않는 1을 씁니다(노드·핀·링크는 서로 다른 식별자 공간입니다).
        constexpr Core::Uint64 OutputKey{1};
        constexpr Core::Uint64 OutputPinBits{0xFF};
        constexpr Core::Uint64 FloatInputBit{Core::Uint64{1} << 60};
        constexpr Core::Uint64 GetterBit{Core::Uint64{1} << 61};
        constexpr Core::Uint64 SavedGetterBit{Core::Uint64{1} << 59};

        constexpr float ColumnSpacing{340.0f};
        constexpr float RowSpacing{260.0f};
        constexpr float MinNodeWidth{230.0f};

        constexpr ImVec4 ActiveColor{0.96f, 0.77f, 0.40f, 1.0f};
        constexpr ImVec4 InactiveColor{0.33f, 0.40f, 0.51f, 1.0f};

        struct FFloatLink {
        public:
            Core::Uint64 mInput{};
            Core::Uint64 mTarget{};
            Core::Uint64 mGetterKey{};
            const Asset::FAnimGraphVariableGetter* mGetter{};
            std::size_t mVariable{};
            Core::FString mParameter{};
            float mValue{};
        };

        // 플레이할 때마다 노드 객체가 새로 만들어져도 같은 노드에 같은 값이 나오는 식별자입니다. 아래 8비트는 핀 번호로 씁니다.
        // 그래프 에셋 노드는 에셋 안의 식별자로, C++ 인스턴스의 멤버 노드는 인스턴스 안의 오프셋과 클래스 이름으로 만듭니다.
        Core::Uint64 NodeKey(const Engine::UAnimInstance& Instance, const Engine::FAnimNode_Base* Node) {
            if (Node->mGraphNodeId >= 0) {
                return ((static_cast<Core::Uint64>(Node->mGraphNodeId) + 1) << 8) | (Core::Uint64{1} << 56);
            }

            const auto Base{reinterpret_cast<std::uintptr_t>(&Instance)};
            const auto Address{reinterpret_cast<std::uintptr_t>(Node)};

            if (Address >= Base && Address - Base < (std::uintptr_t{1} << 24)) {
                const Core::Uint64 ClassBits{static_cast<Core::Uint64>(std::hash<Core::FString>{}(Instance.GetClass()->GetName()) & 0xFFFF) << 34};

                return ((static_cast<Core::Uint64>(Address - Base) + 1) << 8) | ClassBits | (Core::Uint64{1} << 57);
            }

            // 인스턴스 밖에 있는 노드는 안정 식별자를 만들 수 없어 주소를 씁니다.
            return (static_cast<Core::Uint64>(Address) << 8) | (Core::Uint64{1} << 62);
        }

        Core::Uint64 InputPinKey(Core::Uint64 Node, std::size_t Index) {
            return Node | static_cast<Core::Uint64>(Index + 1);
        }

        Core::Uint64 OutputPinKey(Core::Uint64 Node) {
            return Node | OutputPinBits;
        }

        ImVec4 Mix(const ImVec4& From, const ImVec4& To, float Alpha) {
            return {From.x + (To.x - From.x) * Alpha, From.y + (To.y - From.y) * Alpha, From.z + (To.z - From.z) * Alpha, From.w + (To.w - From.w) * Alpha};
        }

        void CollectNodes(Engine::FAnimNode_Base* Node, Core::TArray<Engine::FAnimNode_Base*>& Nodes) {
            if (Node == nullptr || std::find(Nodes.begin(), Nodes.end(), Node) != Nodes.end()) {
                return;
            }

            Nodes.push_back(Node);

            Core::TArray<Engine::FAnimNodePoseInput> Inputs{};

            Node->GetPoseInputs(Inputs);

            for (const Engine::FAnimNodePoseInput& Input : Inputs) {
                CollectNodes(Input.mLink->mLinkedNode, Nodes);
            }
        }

        void AssignDepth(Engine::FAnimNode_Base* Node, Core::Int32 Depth, Core::TMap<Engine::FAnimNode_Base*, Core::Int32>& Depths, Core::Int32 Guard) {
            // 순환 연결이 있어도 멈추도록 깊이를 노드 수로 제한합니다.
            if (Node == nullptr || Guard < 0) {
                return;
            }

            auto& Current{Depths[Node]};

            Current = std::max(Current, Depth);

            Core::TArray<Engine::FAnimNodePoseInput> Inputs{};

            Node->GetPoseInputs(Inputs);

            for (const Engine::FAnimNodePoseInput& Input : Inputs) {
                AssignDepth(Input.mLink->mLinkedNode, Depth + 1, Depths, Guard - 1);
            }
        }
    }

    FAnimGraphPanel::FAnimGraphPanel(Editor::FEditorContext* Context)
        : Editor::FEditorPanel{Context, "Anim Graph", true} {
        ed::Config Config{};

        Config.SettingsFile = nullptr;
        mEditor = ed::CreateEditor(&Config);
        SetVisible(false);
    }

    FAnimGraphPanel::~FAnimGraphPanel() {
        ed::DestroyEditor(mEditor);
    }

    void FAnimGraphPanel::RequestLayout() {
        mLayoutRequested = true;
    }

    Engine::USkeletalMeshComponent* FAnimGraphPanel::FindTarget() const {
        const Editor::FEditorContext* Context{GetEditorContext()};

        if (Context == nullptr) {
            return nullptr;
        }

        if (auto* Component{CoreUObject::Cast<Engine::USkeletalMeshComponent>(Context->GetSelectedObject())}) {
            return Component;
        }

        if (const Engine::AActor* Actor{Context->GetSelectedActor()}) {
            for (const auto& Component : Actor->GetComponents()) {
                if (auto* SkeletalMesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(Component.get())}) {
                    return SkeletalMesh;
                }
            }
        }

        return nullptr;
    }

    void FAnimGraphPanel::DrawContent(float) {
        Engine::USkeletalMeshComponent* Component{FindTarget()};
        Engine::UAnimInstance* Instance{Component != nullptr ? Component->GetAnimInstance() : nullptr};

        if (Component == nullptr) {
            ImGui::TextDisabled("스켈레탈 메시 컴포넌트가 있는 액터를 선택하세요.");

            return;
        }

        if (Instance == nullptr) {
            ImGui::TextDisabled("%s에 애니메이션 인스턴스가 없습니다.", Component->GetName().c_str());
            ImGui::TextWrapped("Details 패널에서 AnimClass를 지정하면 그 그래프를 볼 수 있습니다. AnimClass가 비어 있으면 플레이 중에만 단일 노드 인스턴스가 생깁니다.");

            return;
        }

        const bool Playing{Component->HasBegunPlay()};

        ImGui::Text("%s", Component->GetOwner() != nullptr ? Component->GetOwner()->GetName().c_str() : Component->GetName().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("%s · %s", Instance->GetClass()->GetName().c_str(), Playing ? "플레이 중" : "편집 모드 (플레이하면 가중치와 재생 상태가 보입니다)");
        ImGui::SameLine();

        if (ImGui::SmallButton("다시 배치")) {
            mLayoutRequested = true;
        }

        if (ImGui::BeginTable("AnimGraphLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV, ImGui::GetContentRegionAvail())) {
            ImGui::TableSetupColumn("Variables", ImGuiTableColumnFlags_WidthFixed, mVariablesWidth);
            ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            if (ImGui::BeginChild("AnimVariables")) {
                DrawVariables(*Instance, Playing);
            }

            ImGui::EndChild();
            ImGui::TableSetColumnIndex(1);
            DrawGraph(*Instance, Playing);
            ImGui::EndTable();
        }
    }

    void FAnimGraphPanel::DrawVariables(Engine::UAnimInstance& Instance, bool Playing) {
        ImGui::SeparatorText("Variables");

        bool HasVariables{};

        for (const CoreUObject::UClass* Class{Instance.GetClass()}; Class != nullptr && Class->IsChildOf(Engine::UAnimInstance::StaticClass()); Class = Class->GetSuperClass()) {
            for (const CoreUObject::FProperty* Property : Class->GetProperties()) {
                if (!Property->HasFlag(Core::EPropertyFlags::Edit)) {
                    continue;
                }

                HasVariables = true;

                CoreUObject::FPropertyValue Value{Property->ReadValue(&Instance)};
                const bool Editable{Playing && !Property->HasFlag(Core::EPropertyFlags::ReadOnly)};
                const char* Name{Property->GetName().c_str()};

                ImGui::PushID(Property);

                // 플레이 중에는 저장되지 않는 복제 월드이므로 값을 바로 씁니다. 편집 모드의 설정은 실행 취소가 되도록 Details 패널에서 바꿉니다.
                if (auto* Float{std::get_if<float>(&Value)}) {
                    ImGui::SetNextItemWidth(-1.0f);

                    if (Editable) {
                        if (ImGui::DragFloat("##Value", Float, 0.01f, 0.0f, 0.0f, (Core::FString{Name} + ": %.3f").c_str())) {
                            Property->WriteValue(&Instance, Value);
                        }
                    } else {
                        ImGui::Text("%s: %.3f", Name, *Float);
                    }
                } else if (auto* Bool{std::get_if<bool>(&Value)}) {
                    ImGui::BeginDisabled(!Editable);

                    if (ImGui::Checkbox(Name, Bool) && Editable) {
                        Property->WriteValue(&Instance, Value);
                    }

                    ImGui::EndDisabled();
                } else if (auto* String{std::get_if<Core::FString>(&Value)}) {
                    ImGui::TextUnformatted(Name);
                    ImGui::TextDisabled("  %s", String->empty() ? "(없음)" : String->c_str());
                } else {
                    ImGui::TextDisabled("%s", Name);
                }

                ImGui::PopID();
            }
        }

        // 그래프 에셋 변수처럼 리플렉션 프로퍼티가 아닌 실행 중 변수입니다.
        Core::TArray<Engine::FAnimNodeValue> RuntimeVariables{};

        Instance.GetDebugVariables(RuntimeVariables);

        if (!RuntimeVariables.empty()) {
            ImGui::SeparatorText("Graph Variables");
        }

        for (Engine::FAnimNodeValue& Variable : RuntimeVariables) {
            HasVariables = true;

            ImGui::PushID(Variable.mName.c_str());

            if (Variable.mType == Engine::EAnimNodeValueType::Float && Variable.mFloat != nullptr) {
                if (Playing && Variable.mBinding.empty()) {
                    ImGui::SetNextItemWidth(-1.0f);
                    ImGui::DragFloat("##Value", Variable.mFloat, 0.01f, 0.0f, 0.0f, (Variable.mName + ": %.3f").c_str());
                } else {
                    ImGui::Text("%s: %.3f", Variable.mName.c_str(), *Variable.mFloat);

                    if (!Variable.mBinding.empty()) {
                        ImGui::SameLine();
                        ImGui::TextDisabled("<- %s", Variable.mBinding.c_str());
                    }
                }
            }

            ImGui::PopID();
        }

        if (!HasVariables) {
            ImGui::TextDisabled("편집할 변수가 없습니다.");
        }

        if (Playing) {
            const Core::FVector& Velocity{Instance.GetOwnerVelocity()};

            ImGui::SeparatorText("Owner");
            ImGui::Text("Velocity: %.2f, %.2f, %.2f", Velocity.x, Velocity.y, Velocity.z);
            ImGui::Text("Update: %llu", static_cast<unsigned long long>(Instance.GetUpdateCounter()));
        } else {
            ImGui::Spacing();
            ImGui::TextWrapped("설정은 Details 패널의 Anim Instance 항목에서 편집합니다.");
        }
    }


    bool FAnimGraphPanel::PlaceNodes(const Engine::UAnimInstance& Instance, Engine::FAnimNode_Base* Root, bool All) {
        Core::TArray<Engine::FAnimNode_Base*> Nodes{};

        CollectNodes(Root, Nodes);

        if (All) {
            mPlacedNodes.clear();
        }

        const bool Missing{!mPlacedNodes.contains(OutputKey) || std::any_of(Nodes.begin(), Nodes.end(), [this, &Instance](const Engine::FAnimNode_Base* Node) {
            return !mPlacedNodes.contains(NodeKey(Instance, Node));
        })};

        if (!Missing) {
            return false;
        }

        // 자동 배치 위치입니다. 언리얼 AnimGraph처럼 Output Pose를 오른쪽 끝에 두고, 입력 쪽 노드일수록 왼쪽 열에 놓습니다.
        Core::TMap<Engine::FAnimNode_Base*, Core::Int32> Depths{};
        Core::TMap<Core::Int32, Core::TArray<Engine::FAnimNode_Base*>> Columns{};

        AssignDepth(Root, 1, Depths, static_cast<Core::Int32>(Nodes.size()));

        for (Engine::FAnimNode_Base* Node : Nodes) {
            Columns[Depths[Node]].push_back(Node);
        }

        const auto Place{[this](Core::Uint64 Key, float X, float Y) {
            ed::SetNodePosition(ed::NodeId{static_cast<std::uintptr_t>(Key)}, {X, Y});
            mPlacedNodes.insert(Key);
        }};
        float X{};
        float Y{};

        // 그래프 에셋으로 만든 인스턴스는 에디터에서 놓은 위치를 씁니다.
        if (!mPlacedNodes.contains(OutputKey)) {
            if (Instance.GetNodeLayout(nullptr, X, Y)) {
                Place(OutputKey, X, Y);
            } else {
                Place(OutputKey, 0.0f, 0.0f);
            }
        }

        for (const auto& [Depth, Column] : Columns) {
            const float Top{-0.5f * static_cast<float>(Column.size() - 1) * RowSpacing};

            for (std::size_t Row{}; Row < Column.size(); ++Row) {
                const Core::Uint64 Key{NodeKey(Instance, Column[Row])};

                if (mPlacedNodes.contains(Key)) {
                    continue;
                }

                if (Instance.GetNodeLayout(Column[Row], X, Y)) {
                    Place(Key, X, Y);
                } else {
                    Place(Key, -static_cast<float>(Depth) * ColumnSpacing, Top + static_cast<float>(Row) * RowSpacing);
                }
            }
        }

        return true;
    }

    void FAnimGraphPanel::DrawGraph(Engine::UAnimInstance& Instance, bool Playing) {
        Engine::FAnimNode_Base* Root{Instance.GetRootNode()};
        const auto* Graph{CoreUObject::Cast<Engine::UAnimGraphInstance>(&Instance)};
        const Core::Uint64 GraphKey{Graph != nullptr ? (static_cast<Core::Uint64>(std::hash<Core::FString>{}(Graph->GetAnimGraph())) & 0xFFFFF) << 39 : 0};
        Core::TArray<Engine::FAnimNodeValue> Variables{};
        Core::TArray<FFloatLink> FloatLinks{};

        Instance.GetDebugVariables(Variables);

        const Core::Uint64 Counter{Instance.GetUpdateCounter()};
        const auto WeightOf{[Playing, Counter](const Engine::FAnimNode_Base* Node) {
            return Playing && Node != nullptr ? std::clamp(Node->GetDebugWeight(Counter), 0.0f, 1.0f) : 0.0f;
        }};
        const auto LinkColor{[](float Weight) {
            return Weight > 0.0f ? Mix(InactiveColor, ActiveColor, 0.35f + 0.65f * Weight) : InactiveColor;
        }};

        ed::SetCurrentEditor(mEditor);
        Editor::FAnimGraphNodeBuilder::ApplyStyle();
        ed::Begin("AnimGraphEditor", ImGui::GetContentRegionAvail());

        // 처음 보는 노드만 배치하므로, 플레이를 다시 시작해도 사용자가 옮긴 위치가 유지됩니다.
        bool Placed{PlaceNodes(Instance, Root, mLayoutRequested)};
        mLayoutRequested = false;

        Core::TArray<Engine::FAnimNode_Base*> Nodes{};

        CollectNodes(Root, Nodes);

        Editor::FAnimGraphNodeBuilder OutputBuilder{};
        const ImGuiStyle& Style{ImGui::GetStyle()};
        const ImVec4 FloatColor{Editor::FAnimGraphNodeBuilder::FloatColor()};

        // Output Pose 노드입니다. 루트 노드의 포즈가 최종 포즈가 됩니다.
        ed::PushStyleColor(ed::StyleColor_NodeBorder, Root != nullptr && WeightOf(Root) > 0.0f ? ActiveColor : InactiveColor);
        OutputBuilder.FitHeader("Output Pose", "Output Pose");
        OutputBuilder.FitPins("Result");
        OutputBuilder.Begin(ed::NodeId{static_cast<std::uintptr_t>(OutputKey)}, 160.0f);
        OutputBuilder.Header("Output Pose", "Output Pose");
        OutputBuilder.InputPin(ed::PinId{static_cast<std::uintptr_t>(OutputKey)}, "Result", Root != nullptr, Editor::FAnimGraphNodeBuilder::PoseColor());
        OutputBuilder.End();
        ed::PopStyleColor();

        for (Engine::FAnimNode_Base* Node : Nodes) {
            const Core::Uint64 Key{NodeKey(Instance, Node)};
            const float Weight{WeightOf(Node)};
            const char* Type{Node->GetNodeType()};
            const char* Title{Node->mName.empty() ? Type : Node->mName.c_str()};
            const Core::FString WeightText{Playing ? std::format("{:.0f}%", Weight * 100.0f) : Core::FString{}};
            Core::TArray<Engine::FAnimNodePoseInput> Inputs{};
            Core::TArray<Engine::FAnimNodeValue> Values{};
            Core::TArray<Core::FString> InputLabels{};
            Editor::FAnimGraphNodeBuilder Builder{};

            Node->GetPoseInputs(Inputs);
            Node->GetDebugValues(Values);
            Core::TArray<Core::Int32> VariableIndices{};

            VariableIndices.resize(Values.size(), -1);
            InputLabels.reserve(Inputs.size());
            Builder.FitHeader(Title, Type);

            for (const Engine::FAnimNodePoseInput& Input : Inputs) {
                InputLabels.push_back(Playing ? std::format("{}  {:.0f}%", Input.mName, Input.mWeight * 100.0f) : Input.mName);
                Builder.FitPins(InputLabels.back().c_str(), InputLabels.size() == 1 ? "Pose" : nullptr);
            }

            if (Inputs.empty()) {
                Builder.FitPins(nullptr, "Pose");
            }

            if (Playing) {
                Builder.FitProperty("Weight", WeightText.c_str());
            }

            for (std::size_t Index{}; Index < Values.size(); ++Index) {
                Engine::FAnimNodeValue& Value{Values[Index]};

                if (Value.mType == Engine::EAnimNodeValueType::Float && Value.mFloat != nullptr) {
                    Value.mText = std::format("{:.3f}", *Value.mFloat);

                    if (!Value.mBinding.empty()) {
                        const auto Variable{std::find_if(Variables.begin(), Variables.end(), [&Value](const Engine::FAnimNodeValue& Candidate) {
                            return Candidate.mName == Value.mBinding && Candidate.mFloat != nullptr;
                        })};

                        if (Variable != Variables.end()) {
                            VariableIndices[Index] = static_cast<Core::Int32>(Variable - Variables.begin());
                            Value.mText = Playing ? Value.mName + "  " + Value.mText : Value.mName;
                            Builder.FitPins(Value.mText.c_str());

                            continue;
                        }

                        Value.mText += " <- " + Value.mBinding;
                    } else if (Playing) {
                        const Core::FString WidgetText{Value.mName + ": " + Value.mText};

                        Builder.FitText(WidgetText.c_str(), 2.0f * Style.FramePadding.x);

                        continue;
                    }
                } else if (Value.mType == Engine::EAnimNodeValueType::Bool && Value.mBool != nullptr) {
                    Builder.FitText(Value.mName.c_str(), ImGui::GetFrameHeight() + Style.ItemInnerSpacing.x);

                    continue;
                }

                Builder.FitProperty(Value.mName.c_str(), Value.mText.c_str());
            }

            ed::PushStyleColor(ed::StyleColor_NodeBorder, Mix(ed::GetStyle().Colors[ed::StyleColor_NodeBorder], ActiveColor, 0.55f * Weight));
            ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 1.0f + 0.75f * Weight);
            Builder.Begin(ed::NodeId{static_cast<std::uintptr_t>(Key)}, MinNodeWidth);
            Builder.Header(Title, Type);

            // 첫 줄에 첫 입력 핀과 출력 핀을 나란히 두고, 나머지 입력 핀은 아래 줄에 둡니다.
            for (std::size_t Pin{}; Pin < Inputs.size(); ++Pin) {
                Builder.InputPin(ed::PinId{static_cast<std::uintptr_t>(InputPinKey(Key, Pin))}, InputLabels[Pin].c_str(), Inputs[Pin].mLink->mLinkedNode != nullptr, Editor::FAnimGraphNodeBuilder::PoseColor());

                if (Pin == 0) {
                    Builder.OutputPin(ed::PinId{static_cast<std::uintptr_t>(OutputPinKey(Key))}, "Pose", true, Editor::FAnimGraphNodeBuilder::PoseColor(), true);
                }
            }

            if (Inputs.empty()) {
                Builder.OutputPin(ed::PinId{static_cast<std::uintptr_t>(OutputPinKey(Key))}, "Pose", true, Editor::FAnimGraphNodeBuilder::PoseColor(), false);
            }

            for (std::size_t Index{}; Index < Values.size(); ++Index) {
                if (VariableIndices[Index] >= 0) {
                    const Engine::FAnimNodeValue& Value{Values[Index]};
                    const Core::Uint64 Pin{FloatInputBit | InputPinKey(Key, Index)};
                    const std::size_t Variable{static_cast<std::size_t>(VariableIndices[Index])};
                    const Asset::FAnimGraphVariableGetter* Getter{Graph != nullptr && Value.mBindingParameter != nullptr ? Graph->GetVariableGetter(Node->mGraphNodeId, Value.mBindingParameter) : nullptr};
                    const Core::Uint64 GetterKey{GetterBit | GraphKey | (Getter != nullptr ? SavedGetterBit | ((static_cast<Core::Uint64>(Getter->mId) + 1) << 8) : ((static_cast<Core::Uint64>(Variable) + 1) << 8))};

                    Builder.InputPin(ed::PinId{static_cast<std::uintptr_t>(Pin)}, Value.mText.c_str(), true, FloatColor);
                    FloatLinks.push_back({Pin, Key, GetterKey, Getter, Variable, Value.mName, *Value.mFloat});
                }
            }

            Builder.BeginBody();

            if (Playing) {
                Builder.DrawProperty("Weight", WeightText.c_str());
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, {0.19f, 0.23f, 0.30f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ActiveColor);
                ImGui::ProgressBar(Weight, {Builder.GetWidth(), 4.0f}, "");
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar();
            }

            for (std::size_t Index{}; Index < Values.size(); ++Index) {
                if (VariableIndices[Index] >= 0) {
                    continue;
                }

                Engine::FAnimNodeValue& Value{Values[Index]};
                ImGui::PushID(Value.mName.c_str());

                if (Value.mType == Engine::EAnimNodeValueType::Float && Value.mFloat != nullptr) {
                    if (!Value.mBinding.empty() || !Playing) {
                        Builder.DrawProperty(Value.mName.c_str(), Value.mText.c_str(), !Value.mBinding.empty());
                    } else {
                        ImGui::SetNextItemWidth(Builder.GetWidth());

                        if (Value.mMin < Value.mMax) {
                            ImGui::SliderFloat("##Value", Value.mFloat, Value.mMin, Value.mMax, (Value.mName + ": %.3f").c_str());
                        } else {
                            ImGui::DragFloat("##Value", Value.mFloat, 0.01f, 0.0f, 0.0f, (Value.mName + ": %.3f").c_str());
                        }
                    }
                } else if (Value.mType == Engine::EAnimNodeValueType::Bool && Value.mBool != nullptr) {
                    ImGui::BeginDisabled(!Playing || !Value.mBinding.empty());
                    ImGui::Checkbox(Value.mName.c_str(), Value.mBool);
                    ImGui::EndDisabled();
                } else {
                    Builder.DrawProperty(Value.mName.c_str(), Value.mText.c_str());
                }

                ImGui::PopID();
            }

            Builder.End();
            ed::PopStyleVar();
            ed::PopStyleColor();
        }

        std::unordered_set<Core::Uint64> DrawnGetters{};

        for (const FFloatLink& Link : FloatLinks) {
            const Engine::FAnimNodeValue& Variable{Variables[Link.mVariable]};
            const Core::Uint64 Key{Link.mGetterKey};

            if (!DrawnGetters.insert(Key).second) {
                continue;
            }

            if (mPlacedNodes.insert(Key).second) {
                const ImVec2 Target{ed::GetNodePosition(ed::NodeId{static_cast<std::uintptr_t>(Link.mTarget)})};
                const ImVec2 Position{Link.mGetter != nullptr ? ImVec2{Link.mGetter->mX, Link.mGetter->mY} : ImVec2{Target.x - 245.0f, Target.y - 170.0f}};

                ed::SetNodePosition(ed::NodeId{static_cast<std::uintptr_t>(Key)}, Position);
                Placed = true;
            }

            const Core::FString Value{std::format("{:.3f}", *Variable.mFloat)};
            Editor::FAnimGraphNodeBuilder::DrawFloatGetter(ed::NodeId{static_cast<std::uintptr_t>(Key)}, ed::PinId{static_cast<std::uintptr_t>(OutputPinKey(Key))}, Variable.mName.c_str(), Value.c_str(), true);
        }

        for (const FFloatLink& Link : FloatLinks) {
            const auto Input{static_cast<std::uintptr_t>(Link.mInput)};
            const auto Output{static_cast<std::uintptr_t>(OutputPinKey(Link.mGetterKey))};

            ed::Link(ed::LinkId{Input}, ed::PinId{Output}, ed::PinId{Input}, FloatColor, 2.0f);
        }

        if (Playing) {
            const std::uintptr_t Hovered{ed::GetHoveredLink().Get()};
            const auto Link{std::find_if(FloatLinks.begin(), FloatLinks.end(), [Hovered](const FFloatLink& Candidate) {
                return Candidate.mInput == Hovered;
            })};

            if (Link != FloatLinks.end()) {
                ed::Suspend();
                ImGui::SetTooltip("%s -> %s  %.3f", Variables[Link->mVariable].mName.c_str(), Link->mParameter.c_str(), Link->mValue);
                ed::Resume();
            }
        }

        // 링크는 입력 노드의 출력 핀에서 받는 노드의 입력 핀으로 이어지며, 이번 갱신에서 입력 노드가 기여한 가중치만큼 굵고 밝게 그립니다.
        // 활성 링크에는 흐름 표시를 띄워 포즈가 어느 쪽으로 흐르는지 보여 줍니다.
        // 흐름 표시는 한 번 띄우면 잠시 이어지므로 1.5초마다 다시 띄웁니다.
        const bool Pulse{Playing && std::fmod(ImGui::GetTime(), 1.5) < static_cast<double>(ImGui::GetIO().DeltaTime)};
        const auto DrawLink{[Pulse, &LinkColor](Core::Uint64 Id, Core::Uint64 From, Core::Uint64 To, float Weight) {
            const ed::LinkId Link{static_cast<std::uintptr_t>(Id)};

            ed::Link(Link, ed::PinId{static_cast<std::uintptr_t>(From)}, ed::PinId{static_cast<std::uintptr_t>(To)}, LinkColor(Weight), 1.5f + 2.0f * Weight);

            if (Pulse && Weight > 0.0f) {
                ed::Flow(Link);
            }
        }};

        if (Root != nullptr) {
            DrawLink(OutputKey, OutputPinKey(NodeKey(Instance, Root)), OutputKey, WeightOf(Root));
        }

        for (Engine::FAnimNode_Base* Node : Nodes) {
            const Core::Uint64 Key{NodeKey(Instance, Node)};
            Core::TArray<Engine::FAnimNodePoseInput> Inputs{};

            Node->GetPoseInputs(Inputs);

            for (std::size_t Pin{}; Pin < Inputs.size(); ++Pin) {
                if (Engine::FAnimNode_Base* Linked{Inputs[Pin].mLink->mLinkedNode}) {
                    const float Weight{Playing ? std::clamp(Inputs[Pin].mLink->GetDebugWeight(Counter), 0.0f, 1.0f) : 0.0f};

                    DrawLink(InputPinKey(Key, Pin), OutputPinKey(NodeKey(Instance, Linked)), InputPinKey(Key, Pin), Weight);
                }
            }
        }

        if (mFitRequested && !Placed) {
            ed::NavigateToContent(0.0f);
        }

        mFitRequested = Placed;

        // 그래프는 인스턴스가 만들므로 이 패널에서는 연결을 만들거나 지우지 않습니다. 편집은 그래프 에셋 편집 창에서 합니다.
        if (ed::BeginCreate()) {
            ed::PinId Start{};
            ed::PinId End{};

            if (ed::QueryNewLink(&Start, &End)) {
                ed::RejectNewItem(InactiveColor);
            }
        }

        ed::EndCreate();

        if (ed::BeginDelete()) {
            ed::LinkId Link{};
            ed::NodeId DeletedNode{};

            while (ed::QueryDeletedLink(&Link)) {
                ed::RejectDeletedItem();
            }

            while (ed::QueryDeletedNode(&DeletedNode)) {
                ed::RejectDeletedItem();
            }
        }

        ed::EndDelete();
        ed::End();
        ed::SetCurrentEditor(nullptr);
    }

}
