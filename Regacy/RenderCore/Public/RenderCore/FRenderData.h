/**
 * @file FRenderData.h
 * @brief 텍스트·빌보드 그리기, 뷰 행렬, 렌더 설정과 광원 셰이더 파라미터를 정의합니다.
 */
#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "Core/CoreMinimal.h"
#include "Core/Base/FAssetHandle.h"

/**
 * @brief 텍스트 셰이더가 문자 사각형을 확장할 때 사용하는 글리프별 레코드입니다.
 * @note 로컬 위치와 크기의 단위는 사용하는 텍스트 그리기 경로가 결정합니다.
 */
struct FTextVertex {
public:
    ReflectTypes(FTextVertex);

public:
    /**
     * @brief 글리프 레코드의 구조체 리플렉션 메타데이터를 반환합니다.
     * @return FTextVertex의 등록 구조체 정보입니다.
     */
    ReflectStruct(FTextVertex);

public:
    ReflectProperty(mLocalPosition, "LocalPosition", EPropertyFlags::Serialize, ReflectSerializer(mLocalPosition));
    /**
     * @brief 글리프 사각형 왼쪽 위의 로컬 위치입니다.
     */
    FVector2 mLocalPosition{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Serialize, ReflectSerializer(mSize));
    /**
     * @brief 글리프 사각형의 가로·세로 크기입니다.
     */
    FVector2 mSize{};
    ReflectProperty(mUvMin, "UvMin", EPropertyFlags::Serialize, ReflectSerializer(mUvMin));
    /**
     * @brief 글리프 아틀라스 영역의 최소 UV 좌표입니다.
     */
    FVector2 mUvMin{};
    ReflectProperty(mUvMax, "UvMax", EPropertyFlags::Serialize, ReflectSerializer(mUvMax));
    /**
     * @brief 글리프 아틀라스 영역의 최대 UV 좌표입니다.
     */
    FVector2 mUvMax{};

    ReflectMembers();
};

/**
 * @brief 월드 텍스트의 배치 기준, 글꼴·파이프라인과 글리프 목록입니다.
 */
struct FTextDrawData {
    /**
     * @brief 월드 텍스트의 배치 기준 행렬입니다.
     */
    FMatrix mWorld{};
    /**
     * @brief 글리프 아틀라스를 제공할 글꼴 에셋 핸들입니다.
     */
    FAssetHandle mFontHandle{};
    /**
     * @brief 텍스트 그리기에 사용할 파이프라인 에셋 핸들입니다.
     */
    FAssetHandle mPipelineHandle{};
    /**
     * @brief 텍스트에 적용할 RGBA 색상입니다.
     */
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    /**
     * @brief 문자 사각형을 생성할 글리프 레코드 목록입니다.
     */
    TArray<FTextVertex> mVertices{};
};

/**
 * @brief 카메라를 향하는 텍스처 사각형의 월드 배치와 표면 데이터입니다.
 */
struct FBillboardDrawData {
    /**
     * @brief 빌보드의 월드 배치 행렬입니다.
     */
    FMatrix mWorld{};
    /**
     * @brief 빌보드에서 샘플링할 텍스처 에셋 핸들입니다.
     */
    FAssetHandle mTextureHandle{};
    /**
     * @brief 빌보드 그리기에 사용할 파이프라인 에셋 핸들입니다.
     */
    FAssetHandle mPipelineHandle{};
    /**
     * @brief 빌보드 사각형의 가로·세로 크기입니다.
     */
    FVector2 mSize{};
    /**
     * @brief 빌보드 텍스처 영역의 최소 UV 좌표입니다.
     */
    FVector2 mUvMin{};
    /**
     * @brief 빌보드 텍스처 영역의 최대 UV 좌표입니다.
     */
    FVector2 mUvMax{};
    /**
     * @brief 빌보드 텍스처에 적용할 RGBA 색상입니다.
     */
    FVector4 mColor{};
};

/**
 * @brief 뷰의 공간 변환 행렬과 컬링에 사용할 절두체를 묶은 데이터입니다.
 */
struct FViewMatrices {
    /**
     * @brief 행 벡터 규약에서 mView * mProjection 순서로 합성한 행렬입니다.
     */
    FMatrix mViewProjection{};
    /**
     * @brief 월드 공간을 뷰 공간으로 변환하는 행렬입니다.
     */
    FMatrix mView{};
    /**
     * @brief 뷰 공간을 클립 공간으로 변환하는 투영 행렬입니다.
     */
    FMatrix mProjection{};
    /**
     * @brief 월드 공간의 뷰 절두체입니다. 직교 뷰 등에서는 별도 행렬 컬링을 사용할 수 있습니다.
     */
    FFrustum mViewFrustum{};
};

/**
 * @brief 뷰 후처리 기능의 활성 설정입니다.
 */
struct FPostProcessingSettings {
public:
    ReflectTypes(FPostProcessingSettings);

public:
    /**
     * @brief 후처리 설정의 구조체 리플렉션 메타데이터를 반환합니다.
     * @return FPostProcessingSettings의 등록 구조체 정보입니다.
     */
    ReflectStruct(FPostProcessingSettings);

public:
    ReflectProperty(mFXAA, "FXAA", EPropertyFlags::Serialize, ReflectSerializer(mFXAA));
    /**
     * @brief FXAA 후처리 안티앨리어싱을 활성화할지 여부입니다.
     */
    bool mFXAA{true};

    ReflectMembers();
};

/**
 * @brief 배경 색상, 하늘과 후처리를 제어하는 렌더 설정입니다.
 */
struct FRenderSettings {
public:
    ReflectTypes(FRenderSettings);

public:
    /**
     * @brief 렌더 설정의 구조체 리플렉션 메타데이터를 반환합니다.
     * @return FRenderSettings의 등록 구조체 정보입니다.
     */
    ReflectStruct(FRenderSettings);

public:
    ReflectProperty(mClearColor, "ClearColor", EPropertyFlags::Serialize, ReflectSerializer(mClearColor));
    /**
     * @brief 렌더 타깃을 지울 때 사용할 RGBA 배경 색상입니다.
     */
    FVector4 mClearColor{0.2f, 0.2f, 0.7f, 1.0f};
    ReflectProperty(mBRenderSky, "BRenderSky", EPropertyFlags::Serialize, ReflectSerializer(mBRenderSky));
    /**
     * @brief 하늘 렌더링을 활성화할지 여부입니다.
     */
    bool mBRenderSky{true};
    ReflectProperty(mPostProcessing, "PostProcessing", EPropertyFlags::Serialize, ReflectSerializer(mPostProcessing));
    /**
     * @brief 뷰에 적용할 후처리 설정입니다.
     */
    FPostProcessingSettings mPostProcessing{};

    ReflectMembers();
};

/**
 * @brief 셰이더에서 구별하는 광원 종류입니다.
 */
enum class ELightType : Uint32 {
    /**
     * @brief 일정한 방향에서 비추는 방향성 광원입니다.
     */
    Directional,
    /**
     * @brief 위치와 감쇠 반경을 갖는 점 광원입니다.
     */
    Point,
    /**
     * @brief 위치·방향과 원뿔 범위를 갖는 스포트 광원입니다.
     */
    Spot
};

ReflectEnum(ELightType, ReflectEnumValue(ELightType, Directional), ReflectEnumValue(ELightType, Point), ReflectEnumValue(ELightType, Spot));

/**
 * @brief 광원 셰이더에 전달할 색상, 세기, 공간 정보와 원뿔 조건입니다.
 * @note 전체 저장 크기는 패딩을 포함해 64바이트입니다.
 */
struct FLightShaderParameters {
    /**
     * @brief 광원의 RGB 색상입니다.
     */
    FVector3 mColor{1.0f, 1.0f, 1.0f};
    /**
     * @brief 광원의 밝기 배율입니다.
     */
    float mIntensity{1.0f};
    /**
     * @brief 광원의 월드 공간 위치입니다.
     */
    FVector3 mPosition{};
    /**
     * @brief 점·스포트 광원에서 사용하는 감쇠 반경입니다.
     */
    float mAttenuationRadius{};
    /**
     * @brief 광원의 월드 공간 방향입니다.
     */
    FVector3 mDirection{0.0f, 0.0f, 1.0f};
    /**
     * @brief 스포트 광원의 내부 원뿔 각도에 대한 코사인 값입니다.
     */
    float mInnerConeCos{1.0f};
    /**
     * @brief 스포트 광원의 외부 원뿔 각도에 대한 코사인 값입니다.
     */
    float mOuterConeCos{1.0f};
    /**
     * @brief 적용할 광원 종류입니다.
     */
    ELightType mType{ELightType::Directional};
    /**
     * @brief 셰이더 저장 레이아웃의 64바이트 크기를 맞추는 패딩입니다.
     */
    FVector2 mPadding{};
};

static_assert(sizeof(FLightShaderParameters) == 64);
