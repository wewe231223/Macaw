/**
 * @file Renderer.h
 * @brief Direct3D 11 렌더러와 프레임 제출을 제공합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FMeshData.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/EEditorMesh.h"
#include "RenderCore/FRenderStats.h"
#include "RenderCore/IRendererModule.h"
#include "RenderCore/Shader/FShaderManager.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UTexture2D.h"
#include "Renderer/FLineVertex.h"

#include <d3d11.h>
#include <wrl/client.h>

/**
 * @brief Direct3D 11 장치, 스왑 체인, 메시와 프레임 렌더링을 관리합니다.
 * @ingroup Renderer
 */
class FRenderer final : public IRendererModule {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FRenderer, IRendererModule);

public:
    /**
     * @brief FRenderer의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    FRenderer();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FRenderer() override;
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 복사 또는 이동 생성을 금지합니다.
     */
    FRenderer(const FRenderer&) = delete;
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 대입을 금지합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FRenderer& operator=(const FRenderer&) = delete;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FRenderer);

    /**
     * @brief Direct3D 11 장치, 스왑 체인과 렌더 파이프라인을 준비합니다.
     * @param[in] WindowHandle 렌더링할 네이티브 Win32 창 핸들입니다.
     * @param[in] Width 클라이언트 영역 또는 타깃의 가로 픽셀 수입니다.
     * @param[in] Height 클라이언트 영역 또는 타깃의 세로 픽셀 수입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Initialize(void* WindowHandle, Uint32 Width, Uint32 Height) override;
    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Shutdown() override;

    /**
     * @brief 렌더러 장치를 참조하는 새로운 렌더 장면을 생성합니다.
     * @return 생성한 객체의 단독 소유권이며, 생성 또는 검증 실패 시 nullptr입니다.
     */
    std::unique_ptr<FSceneInterface> AllocateScene() override;
    /**
     * @brief CPU 메시를 종류별 GPU 정점·인덱스 버퍼에 업로드합니다.
     * @param[in] Type 생성, 조회 또는 표시할 타입입니다.
     * @param[in] Mesh 업로드할 CPU 메시입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool UploadMesh(EPrimitiveType Type, const FMeshData& Mesh) override;

    /**
     * @brief 지정한 픽셀 크기의 오프스크린 렌더 타깃을 생성합니다.
     * @param[in] Width 클라이언트 영역 또는 타깃의 가로 픽셀 수입니다.
     * @param[in] Height 클라이언트 영역 또는 타깃의 세로 픽셀 수입니다.
     * @return 생성한 객체의 단독 소유권이며, 생성 또는 검증 실패 시 nullptr입니다.
     */
    std::unique_ptr<FRenderTarget> CreateRenderTarget(Uint32 Width, Uint32 Height) override;
    /**
     * @brief 기존 오프스크린 렌더 타깃을 새 픽셀 크기로 변경합니다.
     * @param[in,out] Target 렌더 타깃 또는 셰이더 컴파일 프로파일입니다.
     * @param[in] Width 클라이언트 영역 또는 타깃의 가로 픽셀 수입니다.
     * @param[in] Height 클라이언트 영역 또는 타깃의 세로 픽셀 수입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool ResizeRenderTarget(FRenderTarget* Target, Uint32 Width, Uint32 Height) override;

    /**
     * @brief 백 버퍼 크기를 동기화하고 프레임 렌더링 통계를 초기화합니다.
     * @param[in] Width 클라이언트 영역 또는 타깃의 가로 픽셀 수입니다.
     * @param[in] Height 클라이언트 영역 또는 타깃의 세로 픽셀 수입니다.
     */
    void BeginFrame(Uint32 Width, Uint32 Height) override;
    /**
     * @brief 지정한 장면 뷰를 타깃에 렌더링하고 에디터 보조 선분을 그립니다.
     * @param[in] Family 장면, 출력 타깃과 카메라를 묶은 렌더 요청입니다.
     */
    void BeginRenderingViewFamily(const FSceneViewFamily& Family) override;

    /**
     * @brief UI 렌더링에 사용할 백 버퍼와 뷰포트를 설정합니다.
     */
    void BeginUiRender() override;
    /**
     * @brief 스왑 체인의 백 버퍼를 화면에 표시합니다.
     */
    void Present() override;

    /**
     * @brief Direct3D 11 장치의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    void* GetNativeDevice() const override;
    /**
     * @brief Direct3D 11 즉시 컨텍스트의 비소유 주소를 반환합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    void* GetNativeContext() const override;
    /**
     * @brief 프레임 렌더링 통계를 반환합니다.
     * @return 프레임 렌더링 통계입니다.
     */
    FRenderStats GetStats() const override;
    bool ReloadChangedShaders() override;

private:
    /**
     * @brief 스왑 체인을 조정하고 백 버퍼 렌더 타깃 뷰를 생성합니다.
     * @param[in] Width 클라이언트 영역 또는 타깃의 가로 픽셀 수입니다.
     * @param[in] Height 클라이언트 영역 또는 타깃의 세로 픽셀 수입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool CreateBackBuffer(Uint32 Width, Uint32 Height);
    /**
     * @brief 메시·선분·기즈모 셰이더, 입력 레이아웃과 고정 파이프라인 상태를 생성합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool CreatePipeline();
    bool CreateShaders();
    /**
     * @brief 고정 에디터 메시의 로컬 정점과 인덱스를 종류별 불변 GPU 버퍼에 업로드합니다.
     * @return 모든 버퍼를 생성하면 true이며 GPU 자원 생성 실패는 치명적 오류로 처리합니다.
     */
    bool CreateEditorMeshes();
    /**
     * @brief 내장 HLSL 소스를 지정한 진입점과 프로파일로 컴파일합니다.
     * @param[in] Source 컴파일할 HLSL 소스의 주소입니다.
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     * @param[in] Entry 셰이더 진입점 이름입니다.
     * @param[in] Target 렌더 타깃 또는 셰이더 컴파일 프로파일입니다.
     * @param[in,out] Code 성공 시 컴파일된 셰이더 바이트코드를 받습니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Compile(const char* Source, const char* Entry, const char* Target, std::shared_ptr<const FShader>& Code);
    /**
     * @brief 깊이 검사 정책에 맞는 보조 선분을 동적 정점 버퍼로 그립니다.
     * @param[in] Elements 이번 프레임의 에디터 보조 도형입니다.
     * @param[in] DepthTest 깊이 검사를 적용할지 지정합니다.
     */
    void DrawLines(const FPrimitiveDrawInterface* Elements, bool DepthTest);
    void DrawMeshes(const FPrimitiveDrawInterface* Elements, bool DepthTest);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    FShaderManager mShaderManager{};
    ReflectProperty(mShaderManager, "ShaderManager", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mShadersDirty{};
    ReflectProperty(mShadersDirty, "ShadersDirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief Direct3D 11 장치입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice{};
    ReflectProperty(mDevice, "Device", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief Direct3D 11 즉시 컨텍스트입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 화면 표시용 스왑 체인입니다.
     */
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};
    ReflectProperty(mSwapChain, "SwapChain", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 스왑 체인 백 버퍼의 렌더 타깃 뷰입니다.
     */
    std::unique_ptr<UTexture2D> mBackBuffer{};
    ReflectProperty(mBackBuffer, "BackBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 메시용 정점 셰이더입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mMeshVertexShader{};
    ReflectProperty(mMeshVertexShader, "MeshVertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 메시용 픽셀 셰이더입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mMeshPixelShader{};
    ReflectProperty(mMeshPixelShader, "MeshPixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 메시 정점 입력 레이아웃입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mMeshLayout{};
    ReflectProperty(mMeshLayout, "MeshLayout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 보조 선분 정점 셰이더입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mLineVertexShader{};
    ReflectProperty(mLineVertexShader, "LineVertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 보조 선분 픽셀 셰이더입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mLinePixelShader{};
    ReflectProperty(mLinePixelShader, "LinePixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 선분 정점 입력 레이아웃입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mLineLayout{};
    ReflectProperty(mLineLayout, "LineLayout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 뷰·투영 상수 버퍼입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11Buffer> mViewBuffer{};
    ReflectProperty(mViewBuffer, "ViewBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 보조 선분용 동적 정점 버퍼입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11Buffer> mLineBuffer{};
    ReflectProperty(mLineBuffer, "LineBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 래스터라이저 상태입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
    ReflectProperty(mRasterizer, "Rasterizer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 깊이 검사가 켜진 깊이 상태입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthEnabled{};
    ReflectProperty(mDepthEnabled, "DepthEnabled", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 깊이를 쓰지 않는 깊이 검사 상태입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthReadOnly{};
    ReflectProperty(mDepthReadOnly, "DepthReadOnly", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 깊이 검사가 꺼진 상태입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthDisabled{};
    ReflectProperty(mDepthDisabled, "DepthDisabled", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프리미티브 종류별 GPU 메시입니다.
     */
    std::array<std::unique_ptr<UStaticMesh>, 3> mMeshes{};
    ReflectProperty(mMeshes, "Meshes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 업로드할 보조 선분 정점입니다.
     */
    TArray<FLineVertex> mLineVertices{};
    ReflectProperty(mLineVertices, "LineVertices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 동적 선분 버퍼 정점 용량입니다.
     */
    Uint32 mLineCapacity{};
    ReflectProperty(mLineCapacity, "LineCapacity", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 가로 픽셀 수입니다.
     */
    Uint32 mWidth{};
    ReflectProperty(mWidth, "Width", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 세로 픽셀 수입니다.
     */
    Uint32 mHeight{};
    ReflectProperty(mHeight, "Height", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 프레임 렌더링 통계입니다.
     */
    FRenderStats mStats{};
    ReflectProperty(mStats, "Stats", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 고정 에디터 메시의 로컬 정점에 월드 행렬과 뷰·투영 행렬을 적용하는 기즈모 정점 셰이더입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mGizmoVertexShader{};
    ReflectProperty(mGizmoVertexShader, "GizmoVertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 조명 계산 없이 요청의 RGBA 색상을 출력하는 기즈모 픽셀 셰이더입니다.
     */
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mGizmoPixelShader{};
    ReflectProperty(mGizmoPixelShader, "GizmoPixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mOverlayLayout{};
    ReflectProperty(mOverlayLayout, "OverlayLayout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mEditorMeshBuffer{};
    ReflectProperty(mEditorMeshBuffer, "EditorMeshBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11BlendState> mOverlayBlend{};
    ReflectProperty(mOverlayBlend, "OverlayBlend", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<std::unique_ptr<UStaticMesh>, static_cast<std::size_t>(EEditorMesh::Count)> mEditorMeshes{};
    ReflectProperty(mEditorMeshes, "EditorMeshes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
