#include "pch.h"
#include "FEditorInfo.h"

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
