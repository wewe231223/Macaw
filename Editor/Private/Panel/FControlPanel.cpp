#include "pch.h"
#include "Editor/Property/FReflectedDetails.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Editor/Panel/FControlPanel.h"
#include "Editor/FileDialog.h"

#include <windows.h>
#include <filesystem>
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "World/AActor.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/UWorld.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Core/Console/Console.h"
#include "CoreUObject/TObjectIterator.h"

void FControlPanel::DrawPanel() {
    const char* PrimitiveMeshTypes[]{"/Game/System/Mesh/Cube.bin", "/Game/System/Mesh/Sphere.bin", "/Game/System/Mesh/Plane.bin", "/Game/System/Mesh/Cylinder.bin", "/Game/System/Mesh/Capsule.bin", "/Game/System/Mesh/Cone.bin", "/Game/System/Mesh/Torus.bin", "/Game/System/Mesh/Pyramid.bin"};

    if (ImGui::BeginMenu("Create", mEditorContext->IsWorldEditingAllowed())) {
        std::vector<const UClass*> SpawnableComponentTypes{};

        for (const UClass* Type : FReflectionRegistry::GetClasses()) {
            if (Type != nullptr && Type->CanCreateInstance() &&
                Type->IsChildOf(UActorComponent::StaticClass())) {
                SpawnableComponentTypes.push_back(Type);
            }
        }

        ImGui::TextDisabled("Spawn Component");

        if (SpawnableComponentTypes.empty()) {
            ImGui::TextDisabled("No spawnable component types are registered.");
        } else {
            if (mSelectedComponentIndex < 0) {
                const auto StaticMeshType{std::ranges::find_if(SpawnableComponentTypes, [](const UClass* Type) {
                    return Type->IsChildOf(UStaticMeshComponent::StaticClass());
                })};

                mSelectedComponentIndex = StaticMeshType != SpawnableComponentTypes.end() ? static_cast<int>(std::distance(SpawnableComponentTypes.begin(), StaticMeshType)) : 0;
            }

            mSelectedComponentIndex = std::clamp(mSelectedComponentIndex, 0, static_cast<int>(SpawnableComponentTypes.size()) - 1);

            const UClass* SelectedComponentType{SpawnableComponentTypes[mSelectedComponentIndex]};

            ImGui::SetNextItemWidth(220.0f);

            if (ImGui::BeginCombo("Component", SelectedComponentType->GetClassName().ToString().c_str())) {
                for (int Index{0}; Index < static_cast<int>(SpawnableComponentTypes.size()); ++Index) {
                    const bool BIsSelected{Index == mSelectedComponentIndex};

                    if (ImGui::Selectable(SpawnableComponentTypes[Index]->GetClassName().ToString().c_str(), BIsSelected)) {
                        mSelectedComponentIndex = Index;
                        SelectedComponentType = SpawnableComponentTypes[Index];
                    }

                    if (BIsSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            const bool BIsStaticMesh{SelectedComponentType->IsChildOf(UStaticMeshComponent::StaticClass())};

            if (BIsStaticMesh) {
                ImGui::SetNextItemWidth(180.0f);
                ImGui::Combo("Mesh", &mSelectedMeshIndex, PrimitiveMeshTypes, IM_ARRAYSIZE(PrimitiveMeshTypes));
            }

            ImGui::InputInt("Number of Objects to Spawn", &mSpawnCountToRequest);

            if (mSpawnCountToRequest < 1) {
                mSpawnCountToRequest = 1;
            }

            if (ImGui::Button("Spawn Object(s)")) {
                mEditorToWorldSender.TryEmplace<FMessageSpawnComponent>(SelectedComponentType->GetClassName(), FString{BIsStaticMesh ? PrimitiveMeshTypes[mSelectedMeshIndex] : ""}, static_cast<Uint32>(mSpawnCountToRequest));
            }
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene")) {
        const FString ScenePath{mEditorContext != nullptr ? mEditorContext->GetEditorSettings().mLastLoadedScenePath : FString{}};

        if (ScenePath != mDisplayedScenePath) {
            mDisplayedScenePath = ScenePath;

            const std::string SceneName{std::filesystem::path{ScenePath.c_str()}.stem().string()};

            if (!SceneName.empty() && SceneName.size() < sizeof(mSceneNameBuffer)) {
                std::memcpy(mSceneNameBuffer, SceneName.c_str(), SceneName.size() + 1);
            }
        }

        if (!ScenePath.empty()) {
            ImGui::TextDisabled("%s", ScenePath.c_str());
        }

        ImGui::SetNextItemWidth(220.0f);
        ImGui::InputText("Scene Name", mSceneNameBuffer, IM_ARRAYSIZE(mSceneNameBuffer));

        if (ImGui::Button("Save Scene")) {
            mEditorToWorldSender.TryEmplace<FMessageSaveScene>(FString{mSceneNameBuffer});
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(!mEditorContext->IsSceneLoadingAllowed());

        if (ImGui::Button("Load Scene")) {
            const FString FilePath{OpenFileDialog(mWindowHandle, "./scenes", "Scene Files (*.json)\0*.json\0All Files (*.*)\0*.*\0", "json")};

            if (!FilePath.empty()) {
                mEditorToWorldSender.TryEmplace<FMessageLoadScene>(FString{FilePath});
            }
        }

        ImGui::EndDisabled();
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Components", mEditorContext->IsWorldEditingAllowed())) {
        UWorld* World{mEditorContext != nullptr ? mEditorContext->GetWorld() : nullptr};

        if (World == nullptr) {
            ImGui::TextDisabled("World is unavailable.");
        } else {
            struct FComponentTypeState {
                std::size_t mActiveCount{0};
                std::vector<UActorComponent*> mComponents{};
            };

            TMap<FName, FComponentTypeState> ComponentsByType{};

            for (UActorComponent& Component : UObjectSystem::Objects<UActorComponent>()) {
                AActor* Owner{Component.GetOwner()};

                if (Owner == nullptr || Owner->GetWorld() != World) {
                    continue;
                }

                FComponentTypeState& TypeState{ComponentsByType[Component.GetClass()->GetClassName()]};

                TypeState.mComponents.push_back(&Component);
                TypeState.mActiveCount += Component.IsActive() ? 1 : 0;
            }

            TArray<FName> SortedTypes{};

            SortedTypes.reserve(ComponentsByType.size());

            for (const auto& Entry : ComponentsByType) {
                SortedTypes.push_back(Entry.first);
            }

            std::ranges::sort(SortedTypes, [](FName Left, FName Right) {
                return Left.ToString() < Right.ToString();
            });

            mComponentFilter.Draw("Search types##SceneComponents", 240.0f);

            const auto IsTypeVisible{[this](FName TypeName) {
                return mComponentFilter.PassFilter(TypeName.ToString().c_str());
            }};

            const auto SetVisibleTypesActive{[&ComponentsByType, &IsTypeVisible](bool BActive) {
                for (auto& [TypeName, TypeState] : ComponentsByType) {
                    if (!IsTypeVisible(TypeName)) {
                        continue;
                    }

                    for (UActorComponent* Component : TypeState.mComponents) {
                        if (Component != nullptr) {
                            Component->SetActive(BActive);
                        }
                    }
                }
            }};

            if (ImGui::Button("Activate filtered")) {
                SetVisibleTypesActive(true);
            }

            ImGui::SameLine();

            if (ImGui::Button("Deactivate filtered")) {
                SetVisibleTypesActive(false);
            }

            ImGui::Separator();

            bool BHasVisibleType{false};

            for (const FName TypeName : SortedTypes) {
                const FComponentTypeState& TypeState{ComponentsByType.at(TypeName)};

                if (!IsTypeVisible(TypeName)) {
                    continue;
                }

                BHasVisibleType = true;
                ImGui::PushID(TypeName.ToString().c_str());

                const std::size_t ComponentCount{TypeState.mComponents.size()};
                const bool BAllActive{TypeState.mActiveCount == ComponentCount};
                const bool BMixed{TypeState.mActiveCount != 0 && !BAllActive};
                bool BTypeActive{BAllActive};

                ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, BMixed);

                FString Label{TypeName.ToString()};

                Label += " (";
                Label += std::to_string(TypeState.mActiveCount).c_str();
                Label += "/";
                Label += std::to_string(ComponentCount).c_str();
                Label += ")";

                const bool BTypeChanged{ImGui::Checkbox(Label.c_str(), &BTypeActive)};

                ImGui::PopItemFlag();

                if (BTypeChanged) {
                    for (UActorComponent* Component : TypeState.mComponents) {
                        if (Component != nullptr) {
                            Component->SetActive(BTypeActive);
                        }
                    }
                }

                ImGui::PopID();
            }

            if (!BHasVisibleType) {
                ImGui::TextDisabled("No component types match the current filter.");
            }
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("EditorSettings")) {
        for (const char* Category : {"Camera", "Grid", "ON/OFF"}) {
            if (ImGui::BeginMenu(Category)) {
                DrawSettings(Category);
                ImGui::EndMenu();
            }
        }

        ImGui::EndMenu();
    }

    ImGui::Separator();

    const ERenderMode RenderModeValues[]{ERenderMode::Lit, ERenderMode::Unlit, ERenderMode::Wireframe};
    int RenderIndex{0};

    for (int Index{0}; Index < IM_ARRAYSIZE(RenderModeValues); ++Index) {
        if (mEditorContext->GetRenderModeState() == static_cast<std::size_t>(RenderModeValues[Index])) {
            RenderIndex = Index;
            break;
        }
    }

    const char* RenderModes[]{"Lit", "Unlit", "Wireframe"};

    ImGui::SetNextItemWidth(110.0f);

    if (ImGui::Combo("Render Mode", &RenderIndex, RenderModes, IM_ARRAYSIZE(RenderModes))) {
        mEditorContext->SetRenderModeState(static_cast<std::size_t>(RenderModeValues[RenderIndex]));
    }

    if (ImGui::Button("Import")) {
        Console::AddLog(Console::STDOutHandle, ELogLevel::Log, ELogCategory::Etc, "Import Button Click");

        const FString FilePath{OpenFileDialog(mWindowHandle, "./Content/ModelingFiles", "OBJ Files(*.obj)\0*.obj\0All Files(*.*)\0*.*\0", "obj")};

        mEditorToWorldSender.TryEmplace<FMessageImportMesh>(FString{"ObjImport"}, FString{FilePath}, FString{"./Content/Metadata/MonkeyMesh.meta"});
    }
}

FControlPanel::FControlPanel(FWorldEditorContext& InEditorContext, HWND InputWindowHandle, FMessageChannel::FSender InEditorToWorldSender)
	: mEditorContext{&InEditorContext},
	  mEditorToWorldSender{std::move(InEditorToWorldSender)},
	  mWindowHandle{InputWindowHandle} {
}

void FControlPanel::DrawSettings(const char* Category) {
    const FEditorSettings Settings{mEditorContext->GetEditorSettings()};

    FReflectedDetails::Draw(*FEditorSettings::StaticStruct(), &Settings, mPropertyEditor, [this](const FProperty& Property, const FPropertyValue& Value) {
        FEditorSettings Edited{mEditorContext->GetEditorSettings()};

        if (Property.WriteContainer(&Edited, Value)) {
            mEditorContext->SetEditorSettings(Edited);
        }
    }, FString{Category});
}
