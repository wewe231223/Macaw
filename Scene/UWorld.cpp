#include "PCH.h"
#include "UWorld.h"

#include "AActor.h"

std::string GetFilePathFromExplorer() {
	return "./scenes/test.json";
}

UWorld::UWorld()
	: ComponentRegistry(RenderScene, CollisionScene, EditorCameraSubsystem),
	  ActorScene(this),
	  EditorWorldCommandRouter(
		  ActorScene,
		  CollisionScene,
		  EditorCameraSubsystem,
		  EditorSceneController),
	  EditorCameraWriter(EditorCameraSubsystem.GetLegacyWriter()),
	  EditorCameraReader(EditorCameraSubsystem.GetLegacyReader()) {
}

UWorld::~UWorld() = default;

AActor *UWorld::AddActor(std::unique_ptr<AActor> InActor) {
	return ActorScene.AddActor(std::move(InActor));
}

bool UWorld::SpawnActor(const FAssetHandle &MeshHandle, const FAssetHandle &PipelineHandle, const FAssetHandle &MaterialHandle, const FVector3 &Position, UMesh *Mesh, FAssetRegistry * /*AssetRegistry*/) {
	return ActorScene.SpawnActor(MeshHandle, PipelineHandle, MaterialHandle, Position, Mesh);
}

bool UWorld::DestroyActor(AActor *Actor) {
	return ActorScene.DestroyActor(Actor);
}

void UWorld::FlushPendingDestroyActors() {
	ActorScene.FlushPendingDestroyActors();
}

const TArray<std::unique_ptr<AActor>> &UWorld::GetActors() const {
	return ActorScene.GetActors();
}

FRenderProbe &UWorld::BuildRenderProbe() {
	return RenderScene.BuildProbe(EditorCameraSubsystem.GetMainCamera(), EditorSceneController.GetSelectedCollider());
}

FStateChannel<FEditorSelectionState>::FReader UWorld::GetEditorSelectionStateReader() const noexcept {
	return EditorSceneController.GetSelectionStateReader();
}

void UWorld::Tick(float DeltaTime) {
	EditorCameraSubsystem.Tick();
	EditorSceneController.Tick();

	ActorScene.Tick(DeltaTime);
}

void UWorld::RegisterRenderable(UStaticMeshComponent *Component) {
	ComponentRegistry.RegisterRenderable(Component);
}

void UWorld::UnregisterRenderable(UStaticMeshComponent *Component) {
	ComponentRegistry.UnregisterRenderable(Component);
}

void UWorld::SetMainCamera(UCameraComponent *InCamera) {
	ComponentRegistry.SetMainCamera(InCamera);
}

void UWorld::ClearMainCamera(UCameraComponent *InCamera) {
	ComponentRegistry.ClearMainCamera(InCamera);
}

bool UWorld::SaveScene(const FString &SceneName, FAssetRegistry *AssetRegistry) {
	WorldContext.SetAssetRegistry(AssetRegistry);
	return SceneSerializer.Save(*this, SceneName);
}

bool UWorld::LoadScene(const std::filesystem::path &ScenePath, ID3D11Device *Device, FAssetRegistry *AssetRegistry) {
	WorldContext.SetAssetRegistry(AssetRegistry);
	WorldContext.SetDevice(Device);
	return SceneSerializer.Load(*this, ScenePath);
}

void UWorld::InitializeEditorEventSender(FMessageChannel::FSender &&InSender) {
	EditorSceneController.InitializeEventSender(std::move(InSender));
}

void UWorld::InitializeEditorCameraState(
	FStateChannel<FMessageEditorCameraState>::FWriter InWriter,
	FStateChannel<FMessageEditorCameraState>::FReader InReader) {
	EditorCameraSubsystem.InitializeState(std::move(InWriter), std::move(InReader));
}

void UWorld::HandleMousePickRequest(const FMousePickRequestMessage &Message) {
	EditorWorldCommandRouter.HandleMousePickRequest(Message);
}

void UWorld::HandleMousePickReleaseRequest(const FMousePickReleaseRequestMessage &Message) {
	EditorWorldCommandRouter.HandleMousePickReleaseRequest(Message);
}

void UWorld::HandleTransformEditRequest(const FTransformEditRequestMessage &Message) {
	EditorWorldCommandRouter.HandleTransformEditRequest(Message);
}

void UWorld::HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage &Message) {
	EditorWorldCommandRouter.HandleMouseCameraRotateRequest(Message);
}

void UWorld::HandleKeyboardCameraMoveRequest(const FKeyboardCameraMoveRequestMessage &Message) {
	EditorWorldCommandRouter.HandleKeyboardCameraMoveRequest(Message);
}

void UWorld::RegisterCollision(UCollisionComponent *Component) {
	ComponentRegistry.RegisterCollision(Component);
}

void UWorld::UnregisterCollision(UCollisionComponent *Component) {
	ComponentRegistry.UnregisterCollision(Component);
}

void UWorld::HandleSpawnPrimitive(const FMessageSpawnPrimitive &Message, FAssetRegistry &AssetRegistry) {
	EditorWorldCommandRouter.HandleSpawnPrimitive(Message, AssetRegistry);
}

void UWorld::HandleNewScene(const FMessageNewScene &Message) {
	EditorWorldCommandRouter.HandleNewScene(Message);
}

void UWorld::HandleLoadScene(const FMessageLoadScene &Message) {
	EditorWorldCommandRouter.HandleLoadScene(Message);
}

void UWorld::HandleChangeGizmoMode(const FMessageChangeGizmoMode &Message) {
	EditorWorldCommandRouter.HandleChangeGizmoMode(Message);
}

void UWorld::UpdateEditorCameraState() {
	EditorCameraSubsystem.UpdateEditorCameraState();
}

void UWorld::SetAssetRegistry(FAssetRegistry *InAssetRegistry) {
	WorldContext.SetAssetRegistry(InAssetRegistry);
}

void UWorld::SetWindowInfoReader(FStateChannel<RenderWindowInfo>::FReader InReader) {
	EditorCameraSubsystem.SetWindowInfoReader(std::move(InReader));
}

FAssetRegistry *UWorld::GetAssetRegistry() const {
	return WorldContext.GetAssetRegistry();
}

void UWorld::ResetWorld(FAssetRegistry *AssetRegistry, ID3D11Device *Device) {
	WorldContext.SetAssetRegistry(AssetRegistry);
	WorldContext.SetDevice(Device);
	SceneSerializer.Reset(*this);
}

FActorScene &UWorld::GetActorScene() noexcept {
	return ActorScene;
}

const FActorScene &UWorld::GetActorScene() const noexcept {
	return ActorScene;
}

FWorldContext &UWorld::GetWorldContext() noexcept {
	return WorldContext;
}

const FWorldContext &UWorld::GetWorldContext() const noexcept {
	return WorldContext;
}
