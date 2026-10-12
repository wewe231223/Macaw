#pragma once
#include "Core/Common.h"
#include "Math/FMath.h"
#include "Core/Archive/FArchive.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Editor/Settings/FEditorSettings.generated.h"

USTRUCT()
struct FEditorSettings {
public:
    GENERATED_TYPES(FEditorSettings)

public:
    GENERATED_BODY(FEditorSettings)

    void Serialize(FArchive& Ar);

public:
    UPROPERTY(Serialize, Name = "MoveSensitivity", SerializationScope = "Camera", Edit, Category = "Camera", DisplayName = "Move Sensitivity", Min = 1.0f, Max = 100.0f)
    Float32 mMoveSensitivity{5.0f};
    UPROPERTY(Serialize, Name = "RotationSensitivity", SerializationScope = "Camera", Edit, Category = "Camera", DisplayName = "Rotation Sensitivity", Min = 0.1f, Max = 5.0f)
    Float32 mRotationSensitivity{1.0f};
    UPROPERTY(Serialize, Name = "CameraStartPosition", SerializationScope = "Camera", ReadOnly)
    FVector mCameraStartPosition{0.0f, 0.0f, 0.0f};
    UPROPERTY(Serialize, Name = "GridSize", SerializationScope = "Grid", Edit, Category = "Grid", DisplayName = "Grid Size", Min = 0.1f, Max = 100.0f)
    Float32 mGridSize{1.0f};
    UPROPERTY(Serialize, Name = "GridVisible", SerializationScope = "Grid", Edit, Category = "ON/OFF", DisplayName = "Grid")
    bool mGridVisible{true};
    UPROPERTY(Serialize, Name = "GridSnapEnabled", SerializationScope = "Grid", Edit, Category = "Grid", DisplayName = "Snap to Grid")
    bool mGridSnapEnabled{true};
    UPROPERTY(Serialize, Name = "AxisVisible", SerializationScope = "Grid", Edit, Category = "ON/OFF", DisplayName = "World Axis")
    bool mAxisVisible{true};
    UPROPERTY(Serialize, Name = "LastLoadedScenePath", ReadOnly)
    FString mLastLoadedScenePath{};
    UPROPERTY(Serialize, Name = "ViewportLayoutPreset", SerializationScope = "Splitter", ReadOnly)
    Uint8 mViewportLayoutPreset{7U};
    UPROPERTY(Serialize, Name = "ViewportSplitterCount", SerializationScope = "Splitter", ReadOnly)
    Uint32 mViewportSplitterCount{3U};
    UPROPERTY(Serialize, Name = "ViewportSplitterRatio0", SerializationScope = "Splitter", ReadOnly)
    Float32 mViewportSplitterRatio0{0.5f};
    UPROPERTY(Serialize, Name = "ViewportSplitterRatio1", SerializationScope = "Splitter", ReadOnly)
    Float32 mViewportSplitterRatio1{0.5f};
    UPROPERTY(Serialize, Name = "ViewportSplitterRatio2", SerializationScope = "Splitter", ReadOnly)
    Float32 mViewportSplitterRatio2{0.5f};
    UPROPERTY(Serialize, Name = "ControlPanelEnabled", SerializationScope = "Pannel", ReadOnly)
    bool mControlPanelEnabled{true};
    UPROPERTY(Serialize, Name = "ViewportPanelEnabled", SerializationScope = "Pannel", ReadOnly)
    bool mViewportPanelEnabled{true};
    UPROPERTY(Serialize, Name = "PropertyPanelEnabled", SerializationScope = "Pannel", ReadOnly)
    bool mPropertyPanelEnabled{true};
    UPROPERTY(Serialize, Name = "ConsolePanelEnabled", SerializationScope = "Pannel", ReadOnly)
    bool mConsolePanelEnabled{true};
    UPROPERTY(Serialize, Name = "StatPanelEnabled", SerializationScope = "Pannel", ReadOnly)
    bool mStatPanelEnabled{true};
    UPROPERTY(Serialize, Name = "AssetBrowserPanelEnabled", SerializationScope = "Pannel", ReadOnly)
    bool mAssetBrowserPanelEnabled{true};
    UPROPERTY(Serialize, Name = "MaterialEditorPanelEnabled", SerializationScope = "Pannel", ReadOnly)
    bool mMaterialEditorPanelEnabled{true};
    UPROPERTY(Serialize, Name = "OutlinerPanelEnabled", SerializationScope = "Pannel", ReadOnly)
    bool mOutlinerPanelEnabled{true};
    UPROPERTY(Serialize, Name = "ViewerPanelEnabled", SerializationScope = "Pannel", ReadOnly)
    bool mViewerPanelEnabled{true};

};
