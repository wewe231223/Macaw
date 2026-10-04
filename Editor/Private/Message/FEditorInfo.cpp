#include "pch.h"
#include "Editor/Message/FEditorInfo.h"

FMessageSpawnComponent::FMessageSpawnComponent(FString InputComponentType, FString InputMeshType, Uint32 InputCount) noexcept
    : mComponentType(std::move(InputComponentType)),
      mMeshType(std::move(InputMeshType)),
      mSpawnCount(InputCount) {
}

FMessageSaveScene::FMessageSaveScene(FString InputSceneName) noexcept
    : mSceneName(std::move(InputSceneName)) {
}

FMessageLoadScene::FMessageLoadScene(FString InputFilePath) noexcept
    : mFilePath(std::move(InputFilePath)) {
}

FMessageImportMesh::FMessageImportMesh(FString InputAssetName, FString InputFilePath, FString InputMetaPath) noexcept
    : mAssetName(std::move(InputAssetName)),
      mFilePath(std::move(InputFilePath)),
      mMetaPath(std::move(InputMetaPath)) {
}

const FMessageTypeInfo& FMessageSpawnComponent::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMessageSpawnComponent"};
    return Information;
}
const FMessageTypeInfo& FMessageSaveScene::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMessageSaveScene"};
    return Information;
}
const FMessageTypeInfo& FMessageLoadScene::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMessageLoadScene"};
    return Information;
}
const FMessageTypeInfo& FMessageImportMesh::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMessageImportMesh"};
    return Information;
}
