/**
 * @file FPipelineDescription.h
 * @brief 셰이더, 정점 입력과 래스터화·깊이·스텐실·혼합 상태의 파이프라인 기술자를 정의합니다.
 */
#pragma once

#include "Core/Common.h"
#include <filesystem>

/**
 * @brief 정점 입력 요소의 성분 수와 저장 타입입니다.
 */
enum class EVertexFormat {
    /**
     * @brief 32비트 float 성분 두 개입니다.
     */
    Float2,
    /**
     * @brief 32비트 float 성분 세 개입니다.
     */
    Float3,
    /**
     * @brief 32비트 float 성분 네 개입니다.
     */
    Float4,
    /**
     * @brief 32비트 부호 없는 정수 성분 하나입니다.
     */
    Uint
};

/**
 * @brief 입력 요소를 정점별 또는 인스턴스별로 읽는 방식입니다.
 */
enum class EInputClassification {
    /**
     * @brief 정점마다 입력 요소를 읽습니다.
     */
    PerVertex,
    /**
     * @brief 인스턴스 진행에 따라 입력 요소를 읽습니다.
     */
    PerInstance
};

/**
 * @brief 입력 정점과 인덱스를 기본 도형으로 조립하는 방식입니다.
 */
enum class EPrimitiveTopology {
    /**
     * @brief 각 정점을 독립된 점으로 처리합니다.
     */
    PointList,
    /**
     * @brief 정점 두 개씩을 독립된 선분으로 처리합니다.
     */
    LineList,
    /**
     * @brief 인접 정점들을 연결한 연속 선분으로 처리합니다.
     */
    LineStrip,
    /**
     * @brief 정점 세 개씩을 독립된 삼각형으로 처리합니다.
     */
    TriangleList,
    /**
     * @brief 이어지는 정점을 공유하는 연속 삼각형으로 처리합니다.
     */
    TriangleStrip
};

/**
 * @brief 삼각형 내부를 채울지 모서리만 그릴지 선택하는 방식입니다.
 */
enum class EFillMode {
    /**
     * @brief 삼각형 내부를 채웁니다.
     */
    Solid,
    /**
     * @brief 삼각형의 모서리를 선으로 표시합니다.
     */
    Wireframe
};

/**
 * @brief 삼각형의 앞면 또는 뒷면을 제거하는 정책입니다.
 */
enum class ECullMode {
    /**
     * @brief 앞면과 뒷면 모두 제거하지 않습니다.
     */
    None,
    /**
     * @brief 앞면으로 판정한 삼각형을 제거합니다.
     */
    Front,
    /**
     * @brief 뒷면으로 판정한 삼각형을 제거합니다.
     */
    Back
};

/**
 * @brief 깊이와 스텐실 테스트에서 사용하는 비교 함수입니다.
 */
enum class ECompareFunc {
    /**
     * @brief 항상 비교에 실패합니다.
     */
    Never,
    /**
     * @brief 왼쪽 값이 오른쪽 값보다 작으면 통과합니다.
     */
    Less,
    /**
     * @brief 두 값이 같으면 통과합니다.
     */
    Equal,
    /**
     * @brief 왼쪽 값이 오른쪽 값보다 작거나 같으면 통과합니다.
     */
    LessEqual,
    /**
     * @brief 왼쪽 값이 오른쪽 값보다 크면 통과합니다.
     */
    Greater,
    /**
     * @brief 두 값이 다르면 통과합니다.
     */
    NotEqual,
    /**
     * @brief 왼쪽 값이 오른쪽 값보다 크거나 같으면 통과합니다.
     */
    GreaterEqual,
    /**
     * @brief 항상 비교를 통과합니다.
     */
    Always
};

/**
 * @brief 원본 또는 대상 색상·알파에 적용할 혼합 계수입니다.
 */
enum class EBlend {
    /**
     * @brief 혼합 항의 계수로 0을 사용합니다.
     */
    Zero,
    /**
     * @brief 혼합 항의 계수로 1을 사용합니다.
     */
    One,
    /**
     * @brief 원본 알파를 계수로 사용합니다.
     */
    SrcAlpha,
    /**
     * @brief 1에서 원본 알파를 뺀 값을 계수로 사용합니다.
     */
    InvSrcAlpha,
    /**
     * @brief 대상 알파를 계수로 사용합니다.
     */
    DestAlpha,
    /**
     * @brief 1에서 대상 알파를 뺀 값을 계수로 사용합니다.
     */
    InvDestAlpha,
    /**
     * @brief 원본 색상 성분을 계수로 사용합니다.
     */
    SrcColor,
    /**
     * @brief 1에서 원본 색상 성분을 뺀 값을 계수로 사용합니다.
     */
    InvSrcColor,
    /**
     * @brief 대상 색상 성분을 계수로 사용합니다.
     */
    DestColor,
    /**
     * @brief 1에서 대상 색상 성분을 뺀 값을 계수로 사용합니다.
     */
    InvDestColor
};

/**
 * @brief 원본과 대상의 색상 또는 알파를 결합하는 연산입니다.
 */
enum class EBlendOp {
    /**
     * @brief 원본 혼합 항과 대상 혼합 항을 더합니다.
     */
    Add,
    /**
     * @brief 원본 혼합 항에서 대상 혼합 항을 뺍니다.
     */
    Subtract,
    /**
     * @brief 대상 혼합 항에서 원본 혼합 항을 뺍니다.
     */
    RevSubtract,
    /**
     * @brief 원본과 대상의 성분별 최솟값을 사용합니다.
     */
    Min,
    /**
     * @brief 원본과 대상의 성분별 최댓값을 사용합니다.
     */
    Max
};

/**
 * @brief 스텐실 테스트 결과에 따라 저장 값을 갱신하는 연산입니다.
 */
enum class EStencillOp {
    /**
     * @brief 기존 스텐실 값을 유지합니다.
     */
    Keep,
    /**
     * @brief 스텐실 저장 값을 0으로 설정합니다.
     */
    Zero,
    /**
     * @brief 스텐실 값을 바인딩된 참조 값으로 교체합니다.
     */
    Replace,
    /**
     * @brief 스텐실 값을 증가시키되 상한에서 포화시킵니다.
     */
    IncrementClamp,
    /**
     * @brief 스텐실 값을 증가시키며 상한을 넘으면 0으로 순환합니다.
     */
    IncrementWrap,
    /**
     * @brief 스텐실 값을 감소시키되 0에서 포화시킵니다.
     */
    DecrementClamp,
    /**
     * @brief 스텐실 값을 감소시키며 0보다 작아지면 상한으로 순환합니다.
     */
    DecrementWrap,
    /**
     * @brief 스텐실 값의 비트를 반전합니다.
     */
    Invert,
    /**
     * @brief 유효한 스텐실 연산 범위의 끝을 나타내며 실제 연산으로 사용하지 않습니다.
     */
    Max
};

/**
 * @brief 셰이더 정점 입력의 시맨틱, 버퍼 슬롯과 원소 배치를 기술합니다.
 */
struct FInputElementDescription {
    /**
     * @brief POSITION, TEXCOORD 등 셰이더 입력 시맨틱 이름입니다.
     */
    std::string mSemanticName{};
    /**
     * @brief 같은 시맨틱 이름 안에서 입력을 구별하는 인덱스입니다.
     */
    Uint32 mSemanticIndex{0};
    /**
     * @brief 입력 성분의 저장 포맷입니다.
     */
    EVertexFormat mFormat{EVertexFormat::Float3};
    /**
     * @brief 입력을 읽을 정점 버퍼 슬롯입니다.
     */
    Uint32 mInputSlot{0};
    /**
     * @brief 해당 슬롯의 정점 레코드 안에서 요소가 시작하는 바이트 오프셋입니다.
     */
    Uint32 mAlignedByteOffset{0};
    /**
     * @brief 정점별 또는 인스턴스별 입력 진행 방식입니다.
     */
    EInputClassification mInputClassification{EInputClassification::PerVertex};
    /**
     * @brief 인스턴스 입력 원소를 전진시키는 인스턴스 수입니다. 정점별 입력에서는 0을 사용합니다.
     */
    Uint32 mInstanceDataStepRate{0};
};

/**
 * @brief 삼각형 채움, 면 제거와 클리핑 정책을 기술합니다.
 */
struct FRasterizerDescription {
    /**
     * @brief 면 채움 방식입니다.
     */
    EFillMode mFillMode{EFillMode::Solid};
    /**
     * @brief 제거할 면의 종류입니다.
     */
    ECullMode mCullMode{ECullMode::Back};
    /**
     * @brief 반시계 방향 와인딩을 앞면으로 판정할지 여부입니다. false이면 시계 방향을 앞면으로 판정합니다.
     */
    bool mFrontCounterClockwise{false};
    /**
     * @brief 깊이 범위 클리핑을 활성화할지 여부입니다.
     */
    bool mDepthClipEnable{true};
    /**
     * @brief 시저 사각형에 의한 화면 영역 제한을 활성화할지 여부입니다.
     */
    bool mScissorEnable{false};
};

/**
 * @brief 깊이와 스텐실 테스트·쓰기 정책을 기술합니다.
 * @note 현재 렌더 백엔드는 같은 스텐실 설정을 앞면과 뒷면 모두에 적용합니다.
 */
struct FDepthStencilDescription {
    /**
     * @brief 깊이 테스트를 활성화할지 여부입니다.
     */
    bool mDepthEnable{true};
    /**
     * @brief 깊이 버퍼 쓰기를 활성화할지 여부입니다.
     */
    bool mDepthWriteEnable{true};
    /**
     * @brief 입력 깊이와 기존 깊이의 비교 함수입니다.
     */
    ECompareFunc mDepthFunc{ECompareFunc::LessEqual};

    /**
     * @brief 스텐실 테스트를 활성화할지 여부입니다.
     */
    bool mStencilEnable{true};
    /**
     * @brief 스텐실 비교에서 읽을 비트 마스크입니다.
     */
    Uint8 mStencilReadMask{255};
    /**
     * @brief 스텐실 저장 값에서 변경을 허용할 비트 마스크입니다.
     */
    Uint8 mStencilWriteMask{255};

    /**
     * @brief 스텐실 참조 값과 저장 값에 적용할 비교 함수입니다.
     */
    ECompareFunc mStencilFunc{ECompareFunc::Always};
    /**
     * @brief 스텐실과 깊이 테스트를 통과했을 때의 스텐실 연산입니다.
     */
    EStencillOp mStencilPassOp{EStencillOp::Replace};
    /**
     * @brief 스텐실 테스트에 실패했을 때의 스텐실 연산입니다.
     */
    EStencillOp mStencilFailOp{EStencillOp::Keep};
    /**
     * @brief 스텐실 테스트는 통과하고 깊이 테스트에 실패했을 때의 스텐실 연산입니다.
     */
    EStencillOp mStencilDepthFailOp{EStencillOp::Keep};
};

/**
 * @brief 색상과 알파의 독립적인 혼합 계수·연산을 기술합니다.
 */
struct FBlendDescription {
    /**
     * @brief 렌더 타깃의 색상 혼합을 활성화할지 여부입니다.
     */
    bool mBlendEnable{false};
    /**
     * @brief 원본 색상에 곱할 혼합 계수입니다.
     */
    EBlend mSrcBlend{EBlend::One};
    /**
     * @brief 대상 색상에 곱할 혼합 계수입니다.
     */
    EBlend mDestBlend{EBlend::Zero};
    /**
     * @brief 원본과 대상 색상 혼합 항을 결합할 연산입니다.
     */
    EBlendOp mBlendOp{EBlendOp::Add};
    /**
     * @brief 원본 알파에 곱할 혼합 계수입니다.
     */
    EBlend mSrcBlendAlpha{EBlend::One};
    /**
     * @brief 대상 알파에 곱할 혼합 계수입니다.
     */
    EBlend mDestBlendAlpha{EBlend::Zero};
    /**
     * @brief 원본과 대상 알파 혼합 항을 결합할 연산입니다.
     */
    EBlendOp mBlendOpAlpha{EBlendOp::Add};
};

/**
 * @brief 컴파일·바인딩할 그래픽 셰이더 단계입니다.
 */
enum class EShaderStage {
    /**
     * @brief 정점 셰이더 단계입니다.
     */
    Vertex,
    /**
     * @brief 픽셀 셰이더 단계입니다.
     */
    Pixel,
    /**
     * @brief 지오메트리 셰이더 단계입니다.
     */
    Geometry
};

/**
 * @brief 셰이더 소스 경로와 컴파일 진입점·프로파일·단계를 기술합니다.
 */
struct FShaderDescription {
    /**
     * @brief 셰이더 소스 파일 경로입니다.
     */
    std::filesystem::path mSource{};
    /**
     * @brief 컴파일할 셰이더 함수의 진입점 이름입니다.
     */
    std::string mEntryPoint{};
    /**
     * @brief vs_5_0 등 셰이더 컴파일 대상 프로파일입니다.
     */
    std::string mProfile{};
    /**
     * @brief 생성할 셰이더의 단계입니다. 프로파일과 일치하도록 설정해야 합니다.
     */
    EShaderStage mStage{EShaderStage::Vertex};
};

/**
 * @brief 셰이더와 정점 입력, 도형 조립 및 고정 기능 렌더 상태를 묶은 파이프라인 기술자입니다.
 */
struct FPipelineDescription {
    /**
     * @brief 정점 셰이더의 컴파일 설정입니다.
     */
    FShaderDescription mVertexShader{};
    /**
     * @brief 픽셀 셰이더의 컴파일 설정입니다.
     */
    FShaderDescription mPixelShader{};

    /**
     * @brief 선택적으로 사용하는 지오메트리 셰이더의 컴파일 설정입니다.
     */
    FShaderDescription mGeometryShader{};
    /**
     * @brief 지오메트리 셰이더를 생성·사용할지 여부입니다.
     */
    bool mBHasGeometryShader{false};

    /**
     * @brief 셰이더 정점 입력 레이아웃을 이루는 요소 목록입니다.
     */
    std::vector<FInputElementDescription> mInputLayout{};

    /**
     * @brief 입력 정점과 인덱스의 도형 조립 방식입니다.
     */
    EPrimitiveTopology mPrimitiveTopology{EPrimitiveTopology::TriangleList};

    /**
     * @brief 채움·컬링·클리핑 상태입니다.
     */
    FRasterizerDescription mRasterizer{};
    /**
     * @brief 깊이·스텐실 비교와 갱신 상태입니다.
     */
    FDepthStencilDescription mDepthStencil{};
    /**
     * @brief 색상·알파 혼합 상태입니다.
     */
    FBlendDescription mBlend{};
};
