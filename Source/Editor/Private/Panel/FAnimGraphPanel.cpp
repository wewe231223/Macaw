/**
 * @file FAnimGraphPanel.cpp
 * @brief 애니메이션 인스턴스의 노드 그래프를 언리얼 AnimGraph처럼 오른쪽의 Output Pose로 모이는 노드와 링크로 그립니다.
 * @ingroup Editor
 */

#include "Core/Types.h"
#include "Editor/Panel/FAnimGraphPanel.h"
#include "Editor/FEditorContext.h"
#include "Engine/AActor.h"
#include "Asset/Animation/UBlendSpace.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Engine/Animation/FAnimNode_Base.h"
#include "Engine/Animation/FAnimNode_BlendSpacePlayer.h"
#include "Engine/Animation/FAnimValueNode.h"
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

    /**
     * @brief 애니메이션 인스턴스의 노드 그래프를 언리얼 AnimGraph처럼 오른쪽의 Output Pose로 모이는 노드와 링크로 그립니다.
     */
    namespace {
        // Output Pose 노드, 그 입력 핀, 루트에서 들어오는 링크는 다른 식별자와 겹치지 않는 1을 씁니다(노드·핀·링크는 서로 다른 식별자 공간입니다).

        /**
         * @brief 최종 포즈 출력 노드의 예약 키입니다.
         */
        constexpr Core::Uint64 OutputKey{1};

        /**
         * @brief 출력 핀을 입력 핀·노드 키와 구분하는 비트입니다.
         */
        constexpr Core::Uint64 OutputPinBits{0xFF};

        /**
         * @brief Float 값 입력 핀을 다른 키와 구분하는 비트입니다.
         */
        constexpr Core::Uint64 FloatInputBit{Core::Uint64{1} << 60};

        /**
         * @brief 변수 Getter 노드를 다른 키와 구분하는 비트입니다.
         */
        constexpr Core::Uint64 GetterBit{Core::Uint64{1} << 61};

        /**
         * @brief 저장된 Getter ID를 임시 실행 키와 구분하는 비트입니다.
         */
        constexpr Core::Uint64 SavedGetterBit{Core::Uint64{1} << 59};

        /**
         * @brief 실행 그래프 열 사이 간격(픽셀)입니다.
         */
        constexpr float ColumnSpacing{340.0f};

        /**
         * @brief 실행 그래프 행 사이 간격(픽셀)입니다.
         */
        constexpr float RowSpacing{260.0f};

        /**
         * @brief 실행 그래프 노드의 최소 너비(픽셀)입니다.
         */
        constexpr float MinNodeWidth{230.0f};

        /**
         * @brief 활성 포즈 경로의 표시 색상입니다.
         */
        constexpr ImVec4 ActiveColor{0.96f, 0.77f, 0.40f, 1.0f};

        /**
         * @brief 비활성 포즈 경로의 표시 색상입니다.
         */
        constexpr ImVec4 InactiveColor{0.33f, 0.40f, 0.51f, 1.0f};

        /**
         * @brief 실행 그래프에서 Float 입력과 변수 Getter 사이에 표시할 링크입니다.
         * @ingroup Editor
         */
        struct FFloatLink {
        public:

            /**
             * @brief 연결 대상 입력 핀의 노드 편집기 키입니다.
             */
            Core::Uint64 mInput{};

            /**
             * @brief 값 입력을 소유하는 대상 노드의 편집기 키입니다.
             */
            Core::Uint64 mTarget{};

            /**
             * @brief Getter 노드를 표시할 노드 편집기 키입니다.
             */
            Core::Uint64 mGetterKey{};

            /**
             * @brief 원본 그래프가 소유하는 변수 Getter입니다.
             */
            const Asset::FAnimGraphVariableGetter* mGetter{};

            /**
             * @brief 애니메이션 인스턴스가 소유하는 실행 값 노드입니다.
             */
            const Engine::FAnimValueNode_Base* mValueNode{};

            /**
             * @brief 연결한 노드 파라미터 이름입니다.
             */
            Core::FString mParameter{};

            /**
             * @brief 이 입력에서 현재 평가한 Float 값입니다.
             */
            float mValue{};
        };

        /**
         * @brief 실행 중인 값 입력의 표시 이름과 원본 Getter 연결입니다.
         * @ingroup Editor
         */
        struct FAnimNodeValueDisplay {
        public:

            /**
             * @brief 실행 값 입력의 표시 이름입니다.
             */
            Core::FString mName{};

            /**
             * @brief Float 입력의 현재 값을 표시할 문자열입니다.
             */
            Core::FString mFloatLabel{};

            /**
             * @brief 애니메이션 인스턴스가 소유하는 실행 값 노드입니다.
             */
            const Engine::FAnimValueNode_Base* mValueNode{};

            /**
             * @brief 원본 그래프가 소유하는 변수 Getter입니다.
             */
            const Asset::FAnimGraphVariableGetter* mGetter{};
        };

        // 플레이할 때마다 노드 객체가 새로 만들어져도 같은 노드에 같은 값이 나오는 식별자입니다. 아래 8비트는 핀 번호로 씁니다.
        // 그래프 에셋 노드는 에셋 안의 식별자로, C++ 인스턴스의 멤버 노드는 인스턴스 안의 오프셋과 클래스 이름으로 만듭니다.

        /**
         * @brief 원본 또는 실행 노드를 편집기에서 식별할 키로 변환합니다.
         * @param Instance 노드의 원본 대응과 식별 키를 제공하는 인스턴스입니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @return 노드를 식별하는 편집기 키입니다.
         */
        Core::Uint64 NodeKey(const Engine::UAnimInstance& Instance, const Engine::FAnimNode_Base* Node) {
            if (const auto* Graph{CoreUObject::Cast<Engine::UAnimGraphInstance>(&Instance)}) {
                if (const Core::Uint64 Key{Graph->GetNodeKey(Node)}; Key != 0) {
                    return (Key << 8) | (Core::Uint64{1} << 56);
                }
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

        /**
         * @brief 노드 키와 입력 번호를 편집기 입력 핀 키로 인코딩합니다.
         * @param Node 키로 변환하거나 조회할 노드 ID입니다.
         * @param Index 대상 배열의 항목 인덱스입니다.
         * @return 입력 핀을 식별하는 키입니다.
         */
        Core::Uint64 InputPinKey(Core::Uint64 Node, Core::Index_t Index) {
            return Node | static_cast<Core::Uint64>(Index + 1);
        }

        /**
         * @brief 노드 키를 편집기 출력 핀 키로 인코딩합니다.
         * @param Node 키로 변환하거나 조회할 노드 ID입니다.
         * @return 출력 핀을 식별하는 키입니다.
         */
        Core::Uint64 OutputPinKey(Core::Uint64 Node) {
            return Node | OutputPinBits;
        }

        /**
         * @brief 두 UI 색상을 가중치로 선형 보간합니다.
         * @param From 연결을 시작할 원본 항목 ID입니다.
         * @param To 연결할 대상 항목 ID입니다.
         * @param Alpha 0~1의 보간 가중치입니다.
         * @return 보간한 RGBA 색상입니다.
         */
        ImVec4 Mix(const ImVec4& From, const ImVec4& To, float Alpha) {
            return {From.x + (To.x - From.x) * Alpha, From.y + (To.y - From.y) * Alpha, From.z + (To.z - From.z) * Alpha, From.w + (To.w - From.w) * Alpha};
        }

        /**
         * @brief 포즈 연결을 순회하여 중복 없는 실행 노드 목록을 수집합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @param Nodes 발견한 비소유 실행 노드를 추가할 배열입니다.
         */
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

        /**
         * @brief 포즈 연결을 따라 노드별 표시 열 깊이를 계산합니다.
         * @param Node 조회·연결·갱신할 노드입니다.
         * @param Depth 탐색할 상위 보기의 깊이 또는 노드 배치 깊이입니다.
         * @param Depths 노드별 표시 깊이를 기록할 맵입니다.
         * @param Guard 잘못된 연결 순회에 대한 깊이 제한입니다.
         */
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
        const auto* Graph{CoreUObject::Cast<Engine::UAnimGraphInstance>(&Instance)};
        Core::TArray<Engine::FAnimNodeValue> RuntimeVariables{};

        Instance.GetDebugVariables(RuntimeVariables);

        if (!RuntimeVariables.empty()) {
            ImGui::SeparatorText("Graph Variables");
        }

        for (const Engine::FAnimNodeValue& Variable : RuntimeVariables) {
            HasVariables = true;

            ImGui::PushID(Variable.mName.c_str());

            if (const Engine::FAnimNodeFloatValue* Float{std::get_if<Engine::FAnimNodeFloatValue>(&Variable.mValue)}; Float != nullptr && Float->mValue != nullptr) {
                const bool OwnerSpeed{Graph != nullptr && Graph->GetVariableSource(Variable.mName) == Asset::EAnimGraphVariableSource::OwnerSpeed};

                if (Playing && !OwnerSpeed) {
                    ImGui::SetNextItemWidth(-1.0f);
                    ImGui::DragFloat("##Value", Float->mValue, 0.01f, 0.0f, 0.0f, (Variable.mName + ": %.3f").c_str());
                } else {
                    ImGui::Text("%s: %.3f", Variable.mName.c_str(), *Float->mValue);

                    if (OwnerSpeed) {
                        ImGui::SameLine();
                        ImGui::TextDisabled("<- Owner Speed");
                    }
                }
            }

            if (bool* const* Bool{std::get_if<bool*>(&Variable.mValue)}; Bool != nullptr && *Bool != nullptr) {
                ImGui::BeginDisabled(!Playing);
                ImGui::Checkbox(Variable.mName.c_str(), *Bool);
                ImGui::EndDisabled();
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

            for (Core::Index_t Row{}; Row < Column.size(); ++Row) {
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
        Core::TArray<FFloatLink> FloatLinks{};
        Core::TMap<const Engine::FAnimValueNode_Base*, Core::Uint64> ValueNodeKeys{};

        Core::TMap<const Engine::FAnimNode_Base*, float> NodeWeights{};
        Core::TMap<const Engine::FPoseLink*, float> LinkWeights{};
        Core::TMap<const Engine::FAnimNode_Base*, Core::TArray<Asset::FBlendSampleWeight>> SampleWeights{};

        if (Playing) {
            const auto& Visits{Instance.GetDebugVisits()};
            Core::TArray<float> Weights{};
            Core::TArray<float> SampleTotals{};

            Weights.resize(Visits.size());
            SampleTotals.resize(Visits.size());

            for (const Engine::FAnimNodeVisit& Visit : Visits) {
                if (Visit.mSampleIndex >= 0) {
                    SampleTotals[static_cast<Core::Index_t>(Visit.mParentIndex)] += Visit.mWeight;
                }
            }

            for (Core::Index_t Index{}; Index < Visits.size(); ++Index) {
                const Engine::FAnimNodeVisit& Visit{Visits[Index]};
                float Weight{Visit.mWeight};

                if (Visit.mParentIndex >= 0) {
                    const Core::Index_t Parent{static_cast<Core::Index_t>(static_cast<Core::Index_t>(Visit.mParentIndex))};
                    Weight *= Weights[Parent];

                    if (Visit.mSampleIndex >= 0) {
                        Weight /= SampleTotals[Parent];
                    }
                }

                Weights[Index] = Weight;

                if (Visit.mSampleIndex >= 0) {
                    auto& Samples{SampleWeights[Visit.mNode]};
                    const auto Existing{std::find_if(Samples.begin(), Samples.end(), [&Visit](const Asset::FBlendSampleWeight& Sample) {
                        return Sample.mSampleIndex == Visit.mSampleIndex;
                    })};

                    if (Existing != Samples.end()) {
                        Existing->mWeight += Weight;
                    } else {
                        Samples.push_back({Visit.mSampleIndex, Weight});
                    }

                    continue;
                }

                NodeWeights[Visit.mNode] += Weight;

                if (Visit.mLink != nullptr) {
                    LinkWeights[Visit.mLink] += Weight;
                }
            }
        }

        const auto WeightOf{[](const auto& Weights, const auto* Item) {
            const auto Weight{Weights.find(Item)};
            return Weight != Weights.end() ? std::clamp(Weight->second, 0.0f, 1.0f) : 0.0f;
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
        ed::PushStyleColor(ed::StyleColor_NodeBorder, Root != nullptr && WeightOf(NodeWeights, Root) > 0.0f ? ActiveColor : InactiveColor);
        OutputBuilder.FitHeader("Output Pose", "Output Pose");
        OutputBuilder.FitPins("Result");
        OutputBuilder.Begin(ed::NodeId{static_cast<std::uintptr_t>(OutputKey)}, 160.0f);
        OutputBuilder.Header("Output Pose", "Output Pose");
        OutputBuilder.InputPin(ed::PinId{static_cast<std::uintptr_t>(OutputKey)}, "Result", Root != nullptr, Editor::FAnimGraphNodeBuilder::PoseColor());
        OutputBuilder.End();
        ed::PopStyleColor();

        for (Engine::FAnimNode_Base* Node : Nodes) {
            const Core::Uint64 Key{NodeKey(Instance, Node)};
            const float Weight{WeightOf(NodeWeights, Node)};
            const auto* BlendSpacePlayer{dynamic_cast<const Engine::FAnimNode_BlendSpacePlayer*>(Node)};
            const char* Type{Node->GetNodeType().c_str()};
            const char* Title{Node->mName.empty() ? Type : Node->mName.c_str()};
            const Core::FString WeightText{Playing ? std::format("{:.0f}%", Weight * 100.0f) : Core::FString{}};
            Core::TArray<Engine::FAnimNodePoseInput> Inputs{};
            Core::TArray<Engine::FAnimNodeValueInput> ValueInputs{};
            Core::TArray<Engine::FAnimNodeValue> Values{};
            Core::TArray<Core::FString> InputLabels{};
            Core::TArray<FAnimNodeValueDisplay> Displays{};
            Editor::FAnimGraphNodeBuilder Builder{};

            Node->GetPoseInputs(Inputs);
            Node->GetValueInputs(ValueInputs);
            Node->GetDebugValues(Values);

            const auto RecordedSamples{SampleWeights.find(Node)};

            if (BlendSpacePlayer != nullptr && BlendSpacePlayer->mBlendSpace != nullptr && RecordedSamples != SampleWeights.end()) {
                const auto& Samples{BlendSpacePlayer->mBlendSpace->GetSamples()};

                for (const Asset::FBlendSampleWeight& SampleWeight : RecordedSamples->second) {
                    if (SampleWeight.mSampleIndex < 0 || static_cast<Core::Index_t>(SampleWeight.mSampleIndex) >= Samples.size()) {
                        continue;
                    }

                    const Asset::FBlendSample& Sample{Samples[static_cast<Core::Index_t>(SampleWeight.mSampleIndex)]};
                    Engine::FAnimNodeValue Entry{};
                    Entry.mName = std::format("Sample {}", SampleWeight.mSampleIndex);
                    Entry.mValue = std::format("{} @ {:.2f} : {:.0f}%", Sample.mAnimation != nullptr ? Sample.mAnimation->GetName() : "?", Sample.mSampleValue.x, SampleWeight.mWeight * 100.0f);
                    Values.push_back(std::move(Entry));
                }
            }

            Displays.resize(Values.size());
            InputLabels.reserve(Inputs.size());
            Builder.FitHeader(Title, Type);

            for (const Engine::FAnimNodePoseInput& Input : Inputs) {
                InputLabels.push_back(Playing ? std::format("{}  {:.0f}%", Input.mName, WeightOf(LinkWeights, Input.mLink) * 100.0f) : Input.mName);
                Builder.FitPins(InputLabels.back().c_str(), InputLabels.size() == 1 ? "Pose" : nullptr);
            }

            if (Inputs.empty()) {
                Builder.FitPins(nullptr, "Pose");
            }

            if (Playing) {
                Builder.FitProperty("Weight", WeightText.c_str());
            }

            for (Core::Index_t Index{}; Index < Values.size(); ++Index) {
                const Engine::FAnimNodeValue& Value{Values[Index]};
                FAnimNodeValueDisplay& Display{Displays[Index]};

                Display.mName = Value.mName;

                if (const Engine::FAnimNodeFloatValue* Float{std::get_if<Engine::FAnimNodeFloatValue>(&Value.mValue)}; Float != nullptr && Float->mValue != nullptr) {
                    const bool BlendSpaceX{BlendSpacePlayer != nullptr && Float->mValue == &BlendSpacePlayer->mX};

                    if (BlendSpaceX && BlendSpacePlayer->mBlendSpace != nullptr) {
                        const auto& Axis{BlendSpacePlayer->mBlendSpace->GetBlendParameter(Asset::EBlendSpaceAxis::X)};
                        Display.mName = std::format("{} ({})", Value.mName, Axis.mDisplayName);
                    }

                    Display.mFloatLabel = std::format("{:.3f}", *Float->mValue);
                    const auto Input{std::find_if(ValueInputs.begin(), ValueInputs.end(), [&Value](const Engine::FAnimNodeValueInput& Candidate) {
                        return Candidate.mName == Value.mName;
                    })};

                    if (Input != ValueInputs.end()) {
                        Display.mValueNode = Input->mLink->GetLinkedNode();
                    }

                    if (Display.mValueNode != nullptr) {
                        Display.mGetter = Graph != nullptr ? Graph->GetSourceGetter(Display.mValueNode) : nullptr;
                        Display.mFloatLabel = Playing ? Display.mName + "  " + Display.mFloatLabel : Display.mName;
                        Builder.FitPins(Display.mFloatLabel.c_str());
                        continue;
                    }

                    if (Playing) {
                        const Core::FString WidgetText{Display.mName + ": " + Display.mFloatLabel};

                        Builder.FitText(WidgetText.c_str(), 2.0f * Style.FramePadding.x);

                        continue;
                    }
                } else if (bool* const* Bool{std::get_if<bool*>(&Value.mValue)}; Bool != nullptr && *Bool != nullptr) {
                    Builder.FitText(Value.mName.c_str(), ImGui::GetFrameHeight() + Style.ItemInnerSpacing.x);

                    continue;
                }

                const Core::FString* Text{std::get_if<Core::FString>(&Value.mValue)};
                Builder.FitProperty(Display.mName.c_str(), Text != nullptr ? Text->c_str() : Display.mFloatLabel.c_str());
            }

            ed::PushStyleColor(ed::StyleColor_NodeBorder, Mix(ed::GetStyle().Colors[ed::StyleColor_NodeBorder], ActiveColor, 0.55f * Weight));
            ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 1.0f + 0.75f * Weight);
            Builder.Begin(ed::NodeId{static_cast<std::uintptr_t>(Key)}, MinNodeWidth);
            Builder.Header(Title, Type);

            // 첫 줄에 첫 입력 핀과 출력 핀을 나란히 두고, 나머지 입력 핀은 아래 줄에 둡니다.
            for (Core::Index_t Pin{}; Pin < Inputs.size(); ++Pin) {
                Builder.InputPin(ed::PinId{static_cast<std::uintptr_t>(InputPinKey(Key, Pin))}, InputLabels[Pin].c_str(), Inputs[Pin].mLink->mLinkedNode != nullptr, Editor::FAnimGraphNodeBuilder::PoseColor());

                if (Pin == 0) {
                    Builder.OutputPin(ed::PinId{static_cast<std::uintptr_t>(OutputPinKey(Key))}, "Pose", true, Editor::FAnimGraphNodeBuilder::PoseColor(), true);
                }
            }

            if (Inputs.empty()) {
                Builder.OutputPin(ed::PinId{static_cast<std::uintptr_t>(OutputPinKey(Key))}, "Pose", true, Editor::FAnimGraphNodeBuilder::PoseColor(), false);
            }

            for (Core::Index_t Index{}; Index < Values.size(); ++Index) {
                const FAnimNodeValueDisplay& Display{Displays[Index]};

                if (Display.mValueNode != nullptr) {
                    const Engine::FAnimNodeValue& Value{Values[Index]};
                    const Core::Uint64 Pin{FloatInputBit | InputPinKey(Key, Index)};
                    const Asset::FAnimGraphVariableGetter* Getter{Display.mGetter};
                    const Core::Uint64 ValueKey{ValueNodeKeys.try_emplace(Display.mValueNode, ValueNodeKeys.size() + 1).first->second};
                    const Core::Uint64 GetterKey{GetterBit | GraphKey | (Graph != nullptr ? SavedGetterBit | (Graph->GetValueNodeKey(Display.mValueNode) << 8) : ValueKey << 8)};

                    Builder.InputPin(ed::PinId{static_cast<std::uintptr_t>(Pin)}, Display.mFloatLabel.c_str(), true, FloatColor);
                    FloatLinks.push_back({Pin, Key, GetterKey, Getter, Display.mValueNode, Display.mName, *std::get<Engine::FAnimNodeFloatValue>(Value.mValue).mValue});
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

            for (Core::Index_t Index{}; Index < Values.size(); ++Index) {
                const FAnimNodeValueDisplay& Display{Displays[Index]};

                if (Display.mValueNode != nullptr) {
                    continue;
                }

                const Engine::FAnimNodeValue& Value{Values[Index]};
                ImGui::PushID(Value.mName.c_str());

                if (const Engine::FAnimNodeFloatValue* Float{std::get_if<Engine::FAnimNodeFloatValue>(&Value.mValue)}; Float != nullptr && Float->mValue != nullptr) {
                    if (!Playing) {
                        Builder.DrawProperty(Display.mName.c_str(), Display.mFloatLabel.c_str());
                    } else {
                        ImGui::SetNextItemWidth(Builder.GetWidth());

                        if (Float->mMin < Float->mMax) {
                            ImGui::SliderFloat("##Value", Float->mValue, Float->mMin, Float->mMax, (Display.mName + ": %.3f").c_str());
                        } else {
                            ImGui::DragFloat("##Value", Float->mValue, 0.01f, 0.0f, 0.0f, (Display.mName + ": %.3f").c_str());
                        }
                    }
                } else if (bool* const* Bool{std::get_if<bool*>(&Value.mValue)}; Bool != nullptr && *Bool != nullptr) {
                    ImGui::BeginDisabled(!Playing);
                    ImGui::Checkbox(Value.mName.c_str(), *Bool);
                    ImGui::EndDisabled();
                } else {
                    const Core::FString* Text{std::get_if<Core::FString>(&Value.mValue)};
                    Builder.DrawProperty(Value.mName.c_str(), Text != nullptr ? Text->c_str() : "");
                }

                ImGui::PopID();
            }

            Builder.End();
            ed::PopStyleVar();
            ed::PopStyleColor();
        }

        std::unordered_set<Core::Uint64> DrawnGetters{};

        for (const FFloatLink& Link : FloatLinks) {
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

            const Core::FString Value{std::format("{:.3f}", Link.mValue)};
            Editor::FAnimGraphNodeBuilder::DrawFloatGetter(ed::NodeId{static_cast<std::uintptr_t>(Key)}, ed::PinId{static_cast<std::uintptr_t>(OutputPinKey(Key))}, Link.mValueNode->GetDisplayName().c_str(), Value.c_str(), true);
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
                ImGui::SetTooltip("%s -> %s  %.3f", Link->mValueNode->GetDisplayName().c_str(), Link->mParameter.c_str(), Link->mValue);
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
            DrawLink(OutputKey, OutputPinKey(NodeKey(Instance, Root)), OutputKey, WeightOf(NodeWeights, Root));
        }

        for (Engine::FAnimNode_Base* Node : Nodes) {
            const Core::Uint64 Key{NodeKey(Instance, Node)};
            Core::TArray<Engine::FAnimNodePoseInput> Inputs{};

            Node->GetPoseInputs(Inputs);

            for (Core::Index_t Pin{}; Pin < Inputs.size(); ++Pin) {
                if (Engine::FAnimNode_Base* Linked{Inputs[Pin].mLink->mLinkedNode}) {
                    const float Weight{WeightOf(LinkWeights, Inputs[Pin].mLink)};

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
