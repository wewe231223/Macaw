#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/Common.h"
#include "Math/FMath.h"
#include "Core/Archive/FArchive.h"

struct FEditorSettings {
public:
    ReflectTypes(FEditorSettings);

public:
    ReflectStruct(FEditorSettings);

    void Serialize(FArchive& Ar);

public:
    ReflectProperty(mMoveSensitivity, "MoveSensitivity", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mMoveSensitivity), ReflectEditor("Camera", "Move Sensitivity"), ReflectRange(1.0f, 100.0f), ReflectSerializationScope("Camera"));

    Float32 mMoveSensitivity{5.0f};
    ReflectProperty(mRotationSensitivity, "RotationSensitivity", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mRotationSensitivity), ReflectEditor("Camera", "Rotation Sensitivity"), ReflectRange(0.1f, 5.0f), ReflectSerializationScope("Camera"));

    Float32 mRotationSensitivity{1.0f};
    ReflectReadOnlyProperty(mCameraStartPosition, "CameraStartPosition", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mCameraStartPosition), ReflectSerializationScope("Camera"));

    FVector mCameraStartPosition{0.0f, 0.0f, 0.0f};
    ReflectProperty(mGridSize, "GridSize", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mGridSize), ReflectEditor("Grid", "Grid Size"), ReflectRange(0.1f, 100.0f), ReflectSerializationScope("Grid"));

    Float32 mGridSize{1.0f};
    ReflectProperty(mGridVisible, "GridVisible", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mGridVisible), ReflectEditor("ON/OFF", "Grid"), ReflectSerializationScope("Grid"));

    bool mGridVisible{true};
    ReflectProperty(mGridSnapEnabled, "GridSnapEnabled", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mGridSnapEnabled), ReflectEditor("Grid", "Snap to Grid"), ReflectSerializationScope("Grid"));

    bool mGridSnapEnabled{true};
    ReflectProperty(mRotationSnapEnabled, "RotationSnapEnabled", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mRotationSnapEnabled), ReflectEditor("Gizmo", "Rotation Snap"), ReflectSerializationScope("Gizmo"));

    bool mRotationSnapEnabled{false};
    ReflectProperty(mRotationSnapDegrees, "RotationSnapDegrees", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mRotationSnapDegrees), ReflectEditor("Gizmo", "Rotation Step"), ReflectRange(0.1f, 180.0f), ReflectSerializationScope("Gizmo"));

    float mRotationSnapDegrees{15.0f};
    ReflectProperty(mScaleSnapEnabled, "ScaleSnapEnabled", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mScaleSnapEnabled), ReflectEditor("Gizmo", "Scale Snap"), ReflectSerializationScope("Gizmo"));

    bool mScaleSnapEnabled{false};
    ReflectProperty(mScaleSnapStep, "ScaleSnapStep", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mScaleSnapStep), ReflectEditor("Gizmo", "Scale Step"), ReflectRange(0.001f, 10.0f), ReflectSerializationScope("Gizmo"));

    float mScaleSnapStep{0.1f};
    ReflectProperty(mSurfaceSnapEnabled, "SurfaceSnapEnabled", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mSurfaceSnapEnabled), ReflectEditor("Gizmo", "Surface Snap"), ReflectSerializationScope("Gizmo"));

    bool mSurfaceSnapEnabled{false};
    ReflectProperty(mSurfaceSnapOffset, "SurfaceSnapOffset", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mSurfaceSnapOffset), ReflectEditor("Gizmo", "Surface Offset"), ReflectSerializationScope("Gizmo"));

    float mSurfaceSnapOffset{};
    ReflectProperty(mSurfaceAlignRotation, "SurfaceAlignRotation", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mSurfaceAlignRotation), ReflectEditor("Gizmo", "Align to Surface"), ReflectSerializationScope("Gizmo"));

    bool mSurfaceAlignRotation{false};
    ReflectProperty(mGizmoSize, "GizmoSize", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mGizmoSize), ReflectEditor("Gizmo", "Size"), ReflectRange(0.5f, 2.0f), ReflectSerializationScope("Gizmo"));

    float mGizmoSize{1.0f};
    ReflectProperty(mAxisVisible, "AxisVisible", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mAxisVisible), ReflectEditor("ON/OFF", "World Axis"), ReflectSerializationScope("Grid"));

    bool mAxisVisible{true};
    ReflectReadOnlyProperty(mLastLoadedScenePath, "LastLoadedScenePath", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mLastLoadedScenePath));

    FString mLastLoadedScenePath{};
    ReflectReadOnlyProperty(mViewportLayoutPreset, "ViewportLayoutPreset", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mViewportLayoutPreset), ReflectSerializationScope("Splitter"));

    Uint8 mViewportLayoutPreset{7U};
    ReflectReadOnlyProperty(mViewportSplitterCount, "ViewportSplitterCount", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mViewportSplitterCount), ReflectSerializationScope("Splitter"));

    Uint32 mViewportSplitterCount{3U};
    ReflectReadOnlyProperty(mViewportSplitterRatio0, "ViewportSplitterRatio0", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mViewportSplitterRatio0), ReflectSerializationScope("Splitter"));

    Float32 mViewportSplitterRatio0{0.5f};
    ReflectReadOnlyProperty(mViewportSplitterRatio1, "ViewportSplitterRatio1", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mViewportSplitterRatio1), ReflectSerializationScope("Splitter"));

    Float32 mViewportSplitterRatio1{0.5f};
    ReflectReadOnlyProperty(mViewportSplitterRatio2, "ViewportSplitterRatio2", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mViewportSplitterRatio2), ReflectSerializationScope("Splitter"));

    Float32 mViewportSplitterRatio2{0.5f};
    ReflectReadOnlyProperty(mControlPanelEnabled, "ControlPanelEnabled", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mControlPanelEnabled), ReflectSerializationScope("Pannel"));

    bool mControlPanelEnabled{true};
    ReflectReadOnlyProperty(mViewportPanelEnabled, "ViewportPanelEnabled", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mViewportPanelEnabled), ReflectSerializationScope("Pannel"));

    bool mViewportPanelEnabled{true};
    ReflectReadOnlyProperty(mPropertyPanelEnabled, "PropertyPanelEnabled", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mPropertyPanelEnabled), ReflectSerializationScope("Pannel"));

    bool mPropertyPanelEnabled{true};
    ReflectReadOnlyProperty(mConsolePanelEnabled, "ConsolePanelEnabled", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mConsolePanelEnabled), ReflectSerializationScope("Pannel"));

    bool mConsolePanelEnabled{true};
    ReflectReadOnlyProperty(mStatPanelEnabled, "StatPanelEnabled", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mStatPanelEnabled), ReflectSerializationScope("Pannel"));

    bool mStatPanelEnabled{true};
    ReflectReadOnlyProperty(mAssetBrowserPanelEnabled, "AssetBrowserPanelEnabled", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mAssetBrowserPanelEnabled), ReflectSerializationScope("Pannel"));

    bool mAssetBrowserPanelEnabled{true};
    ReflectReadOnlyProperty(mMaterialEditorPanelEnabled, "MaterialEditorPanelEnabled", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mMaterialEditorPanelEnabled), ReflectSerializationScope("Pannel"));

    bool mMaterialEditorPanelEnabled{true};
    ReflectReadOnlyProperty(mOutlinerPanelEnabled, "OutlinerPanelEnabled", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mOutlinerPanelEnabled), ReflectSerializationScope("Pannel"));

    bool mOutlinerPanelEnabled{true};
    ReflectReadOnlyProperty(mViewerPanelEnabled, "ViewerPanelEnabled", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mViewerPanelEnabled), ReflectSerializationScope("Pannel"));

    bool mViewerPanelEnabled{true};

    ReflectMembers();
};
