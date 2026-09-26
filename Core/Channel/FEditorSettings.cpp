#include "pch.h"
#include "FEditorSettings.h"
#include "Core/Archive/FArchive.h"

void FEditorSettings::Serialize(FArchive& Ar) {
    Ar.Serialize("MoveSensitivity", mMoveSensitivity);
    Ar.Serialize("RotationSensitivity", mRotationSensitivity);
    Ar.Serialize("CameraStartPosition", mCameraStartPosition);
    Ar.Serialize("GridSize", mGridSize);
    Ar.Serialize("GridVisible", mGridVisible);
    Ar.Serialize("GridSnapEnabled", mGridSnapEnabled);
    Ar.Serialize("AxisVisible", mAxisVisible);
    Ar.Serialize("LastLoadedScenePath", mLastLoadedScenePath);
    Ar.Serialize("ViewportLayoutPreset", mViewportLayoutPreset);
    Ar.Serialize("ViewportSplitterCount", mViewportSplitterCount);
    Ar.Serialize("ViewportSplitterRatio0", mViewportSplitterRatio0);
    Ar.Serialize("ViewportSplitterRatio1", mViewportSplitterRatio1);
    Ar.Serialize("ViewportSplitterRatio2", mViewportSplitterRatio2);
}
