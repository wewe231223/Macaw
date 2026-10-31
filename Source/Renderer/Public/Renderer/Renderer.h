/**
 * @file Renderer.h
 * @brief Direct3D 11 렌더러와 프레임 제출을 제공합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Asset/FMeshData.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/EEditorMesh.h"
#include "RenderCore/FRenderStats.h"
#include "RenderCore/FDynamicVertexBuffer.h"
#include "RenderCore/IRendererModule.h"
#include "RenderCore/Shader/FShaderManager.h"
#include "Asset/UStaticMesh.h"
#include "Asset/UTexture2D.h"
#include "Asset/UMaterial.h"
#include "Renderer/FLineVertex.h"

#include <d3d11.h>
#include <span>
#include <wrl/client.h>

namespace Renderer {

    class FTextRenderer;
    class FSpriteRenderer;
    class FMaterialPipeline;
    class FLightingResources;
    class FShadowResources;
    class FPostProcessResources;
    class FScene;
    struct FPassTarget;
    struct FMeshPassParameters;
    struct FMeshDrawCommand;
    class FMeshPass;
    class FShadowDepthPass;
    class FBasePass;
    enum class EMeshPass : Core::Uint8;

    /**
     * @brief Direct3D 11 장치, 스왑 체인, 메시와 프레임 렌더링을 관리합니다.
     * @ingroup Renderer
     */
    class FRenderer final : public RenderCore::IRendererModule {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FRenderer, RenderCore::IRendererModule);

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
         * @brief 레지스트리에 등록된 고유 객체이므로 복사·이동 생성을 금지합니다.
         */
        FRenderer(const Renderer::FRenderer&) = delete;

        /**
         * @brief 고유한 객체 수명을 유지하기 위해 대입을 금지합니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Renderer::FRenderer& operator=(const Renderer::FRenderer&) = delete;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FRenderer);

        /**
         * @brief Direct3D 11 장치, 스왑 체인과 렌더 파이프라인을 준비합니다.
         * @param[in] WindowHandle 렌더링할 네이티브 Win32 창 핸들입니다.
         * @param[in] Width 창 클라이언트 영역의 가로 픽셀 수입니다.
         * @param[in] Height 창 클라이언트 영역의 세로 픽셀 수입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool Initialize(void* WindowHandle, Core::Uint32 Width, Core::Uint32 Height) override;

        /**
         * @brief 장면 파이프라인 없이 창과 스왑체인·장치를 초기화합니다.
         * @param[in] WindowHandle 연결할 네이티브 창 핸들입니다.
         * @param[in] Width 창 클라이언트 영역의 가로 픽셀 수입니다.
         * @param[in] Height 창 클라이언트 영역의 세로 픽셀 수입니다.
         * @return 성공하면 true, 실패하면 false입니다.
         */
        bool InitializeUi(void* WindowHandle, Core::Uint32 Width, Core::Uint32 Height);

        /**
         * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
         */
        void Shutdown() override;

        /**
         * @brief 렌더러 장치를 참조하는 새로운 렌더 장면을 생성합니다.
         * @return 생성한 객체의 단독 소유권이며, 실패하면 nullptr입니다.
         */
        std::unique_ptr<Engine::FSceneInterface> AllocateScene() override;

        /**
         * @brief CPU 메시를 종류별 GPU 정점·인덱스 버퍼에 업로드합니다.
         * @param[in] Type 기본 프리미티브 종류입니다.
         * @param[in] Mesh 업로드할 CPU 메시입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool UploadMesh(Engine::EPrimitiveType Type, const Asset::FMeshData& Mesh) override;

        /**
         * @brief 지정한 픽셀 크기의 오프스크린 렌더 타깃을 생성합니다.
         * @param[in] Width 렌더 타깃의 가로 픽셀 수입니다.
         * @param[in] Height 렌더 타깃의 세로 픽셀 수입니다.
         * @return 생성한 객체의 단독 소유권이며, 실패하면 nullptr입니다.
         */
        std::unique_ptr<Engine::FRenderTarget> CreateRenderTarget(Core::Uint32 Width, Core::Uint32 Height) override;

        /**
         * @brief 기존 오프스크린 렌더 타깃을 새 픽셀 크기로 변경합니다.
         * @param[in,out] Target 크기를 바꿀 렌더 타깃입니다.
         * @param[in] Width 렌더 타깃의 가로 픽셀 수입니다.
         * @param[in] Height 렌더 타깃의 세로 픽셀 수입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool ResizeRenderTarget(Engine::FRenderTarget* Target, Core::Uint32 Width, Core::Uint32 Height) override;

        /**
         * @brief 백 버퍼 크기를 동기화하고 프레임 렌더링 통계를 초기화합니다.
         * @param[in] Width 창 클라이언트 영역의 가로 픽셀 수입니다.
         * @param[in] Height 창 클라이언트 영역의 세로 픽셀 수입니다.
         */
        void BeginFrame(Core::Uint32 Width, Core::Uint32 Height) override;

        /**
         * @brief 지정한 장면 뷰를 타깃에 렌더링하고 에디터 보조 선분을 그립니다.
         * @param[in] Family 장면, 출력 타깃과 카메라를 묶은 렌더 요청입니다.
         */
        void BeginRenderingViewFamily(const Engine::FSceneViewFamily& Family) override;

        /**
         * @brief UI 렌더링에 사용할 백 버퍼와 뷰포트를 설정합니다.
         */
        void BeginUiRender() override;

        /**
         * @brief 스왑 체인의 백 버퍼를 화면에 표시합니다.
         */
        void Present() override;

        /**
         * @brief 렌더 타깃의 최종 컬러를 스왑 체인 백 버퍼에 복사합니다.
         * @param[in] Target 복사할 렌더러 소유 렌더 타깃입니다.
         * @return 복사했으면 true, 렌더러 타깃이 아니거나 출력이 없거나 크기가 백 버퍼와 다르면 false입니다.
         * @note 복사 전에 픽셀 셰이더 0번 리소스와 출력 렌더 타깃 바인딩을 해제합니다.
         */
        bool CopyToBackBuffer(Engine::FRenderTarget* Target);

        /**
         * @brief 비동기 프레젠테이션의 화면 찢김 허용 설정을 변경합니다.
         * @param[in] Allow 화면 찢김을 허용하려면 true입니다.
         * @return 요청을 수락하면 true이며 지원하지 않는 허용 요청은 false입니다.
         */
        bool SetAllowTearing(bool Allow) override;

        /**
         * @brief 화면 찢김을 허용하도록 설정했는지 반환합니다.
         * @return 사용자 설정에서 화면 찢김을 허용하면 true입니다.
         */
        bool IsTearingAllowed() const override;

        /**
         * @brief 현재 DXGI 환경이 화면 찢김 허용을 지원하는지 반환합니다.
         * @return DXGI의 프레젠테이션 기능 검사에 성공하면 true입니다.
         */
        bool IsTearingSupported() const override;

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
        RenderCore::FRenderStats GetStats() const override;

        /**
         * @brief 현재 그림자 아틀라스의 검사 정보를 반환합니다.
         * @return 미리보기·깊이 뷰의 비소유 주소와 타일 정보 사본입니다.
         */
        RenderCore::FShadowAtlasView GetShadowAtlasView() const override;

        /**
         * @brief 변경된 셰이더를 다시 컴파일하고 필요한 GPU 셰이더를 갱신합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool ReloadChangedShaders() override;

        /**
         * @brief 폰트 메트릭으로 텍스트의 로컬 평면 경계를 계산합니다.
         * @param[in] Text UTF-8 문자열입니다.
         * @param[out] Minimum 계산한 최소 경계 좌표입니다.
         * @param[out] Maximum 계산한 최대 경계 좌표입니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool GetTextBounds(const Core::FString& Text, Core::FVector2& Minimum, Core::FVector2& Maximum) const override;

        /**
         * @brief 머티리얼별 GPU 파이프라인을 반환합니다.
         * @param[in] Material 조회할 머티리얼 설정입니다.
         * @param[in] Mode 파이프라인에 적용할 조명·와이어프레임·법선 표시 모드입니다.
         * @return 대상의 비소유 주소이며, 없거나 유효하지 않으면 nullptr입니다.
         */
        Renderer::FMaterialPipeline* GetMaterialPipeline(const Asset::UMaterial& Material, Engine::EViewMode Mode = Engine::EViewMode::Lit_Phong);

        /**
         * @brief 그림자 아틀라스 깊이 패스에 사용할 깊이 전용 파이프라인을 반환합니다.
         * @return 대상의 비소유 주소이며, 셰이더를 다시 컴파일하는 중이거나 실패하면 nullptr입니다.
         */
        Renderer::FMaterialPipeline* GetDepthPipeline();

        /**
         * @brief 머티리얼이 없는 메시가 사용할 기본 머티리얼을 반환합니다.
         * @return 대상의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        Asset::UMaterial* GetDefaultMaterial() const;

        /**
         * @brief 프로젝트 콘텐츠의 셰이더 경로와 컴파일 캐시 경로를 설정합니다.
         * @param[in] ContentRoot 프로젝트 에셋 경로의 기준 콘텐츠 폴더입니다.
         * @param[in] CacheDirectory 컴파일된 셰이더의 캐시 파일을 보관할 폴더입니다.
         */
        void SetProjectPaths(const std::filesystem::path& ContentRoot, const std::filesystem::path& CacheDirectory);

    private:
        /**
         * @brief 스왑 체인을 조정하고 백 버퍼 렌더 타깃 뷰를 생성합니다.
         * @param[in] Width 창 클라이언트 영역의 가로 픽셀 수입니다.
         * @param[in] Height 창 클라이언트 영역의 세로 픽셀 수입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool CreateBackBuffer(Core::Uint32 Width, Core::Uint32 Height);

        /**
         * @brief 메시·선분·기즈모 셰이더, 입력 레이아웃과 고정 파이프라인 상태를 생성합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool CreatePipeline();

        /**
         * @brief 장면 렌더링에 사용할 셰이더와 입력 레이아웃을 생성합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool CreateShaders();

        /**
         * @brief 고정 에디터 메시의 로컬 정점과 인덱스를 종류별 불변 GPU 버퍼에 업로드합니다.
         * @return 모든 버퍼를 생성하면 true이며 GPU 자원 생성 실패는 치명적 오류로 처리합니다.
         */
        bool CreateEditorMeshes();

        /**
         * @brief 내장 HLSL 소스를 지정한 진입점과 프로파일로 컴파일합니다.
         * @param[in] Source 컴파일할 HLSL 소스의 주소입니다.
         * @param[in] Entry 셰이더 진입점 이름입니다.
         * @param[in] Target 셰이더 컴파일 프로파일(예: vs_5_0)입니다.
         * @param[out] Code 성공 시 컴파일된 셰이더를 받습니다.
         * @return 성공하면 true, 컴파일에 실패하면 false입니다.
         */
        bool Compile(const char* Source, const char* Entry, const char* Target, std::shared_ptr<const RenderCore::FShader>& Code);

        /**
         * @brief 월드 선은 깊이를 기록하고 편집 오버레이 선은 깊이 검사 없이 그립니다.
         * @param[in] Elements 이번 프레임의 에디터 보조 도형입니다.
         * @param[in] DepthTest 깊이 검사를 적용할지 지정합니다.
         */
        void DrawLines(const Engine::FPrimitiveDrawInterface* Elements, bool DepthTest);

        /**
         * @brief 깊이 검사 단계에 해당하는 에디터 메시들을 GPU에 제출합니다.
         * @param[in] Elements 이번 프레임의 에디터 도형 요청 목록입니다.
         * @param[in] DepthTest 현재 제출할 깊이 검사 단계입니다.
         */
        void DrawMeshes(const Engine::FPrimitiveDrawInterface* Elements, bool DepthTest);

        /**
         * @brief 깊이 검사 없이 표시할 에디터 텍스트를 마지막 오버레이로 그립니다.
         * @param[in] Family 출력 타깃·카메라·표시 필터를 포함한 뷰 요청입니다.
         */
        void DrawOverlayTexts(const Engine::FSceneViewFamily& Family);

        /**
         * @brief 월드 글자와 스프라이트를 함께 깊이 역순으로 정렬하여 투명 영역을 합성합니다.
         * @param[in] EditorIcons true이면 후처리 뒤 에디터 아이콘·글자를, false이면 후처리 앞 월드 투명 프리미티브를 그립니다.
         * @param[in] Family 출력 타깃·카메라·표시 필터를 포함한 뷰 요청입니다.
         */
        void DrawTranslucency(const Engine::FSceneViewFamily& Family, bool EditorIcons);

        /**
         * @brief 장면 패스의 타일 조명 자원과 그림자 아틀라스를 업로드·바인딩합니다.
         * @param[in] Family 장면·카메라·출력 타깃·표시 플래그를 묶은 현재 뷰 요청입니다.
         * @return 필요한 조명 자원 준비에 성공하면 true입니다.
         */
        bool PrepareLighting(const Engine::FSceneViewFamily& Family);

        /**
         * @brief 조명과 그림자 샘플링에 사용한 셰이더 리소스 바인딩을 해제합니다.
         */
        void FinishLighting();

        /**
         * @brief 패스 타깃의 색상과 깊이·스텐실을 지웁니다.
         * @param[in] Target 지울 색상·깊이 뷰를 가진 패스 타깃입니다.
         * @param[in] Color 색상 뷰를 지울 RGBA 값이며, nullptr이면 색상은 유지합니다.
         */
        void ClearTarget(const Renderer::FPassTarget& Target, const float* Color);

        /**
         * @brief 메시 패스 인스턴스가 만든 드로 목록을 패스 타깃에 그립니다.
         * @details 무엇을 그릴지는 패스가 정하고, 모든 메시 패스의 타깃 바인딩·상태 설정·드로 제출은 이 함수 한 곳에서 수행합니다.
         * @param[in,out] Pass 프리미티브 선별과 파이프라인·머티리얼 선택을 맡은 메시 패스입니다.
         * @param[in] Scene 그릴 프리미티브와 광원을 가진 렌더 장면입니다.
         * @param[in] Family 표시 모드와 표시 플래그를 가진 현재 뷰 요청입니다.
         * @param[in] Parameters 출력 타깃과 뷰 투영 행렬입니다.
         */
        void DrawMeshPass(Renderer::FMeshPass& Pass, const Renderer::FScene& Scene, const Engine::FSceneViewFamily& Family, const Renderer::FMeshPassParameters& Parameters);

        /**
         * @brief 정렬된 드로 목록을 상태 변경을 줄여 제출하고 인스턴스를 묶어 그립니다.
         * @param[in] Family 와이어프레임 여부를 정할 현재 뷰 요청입니다.
         * @param[in] Pass 머티리얼 바인딩과 통계 대상을 정할 메시 패스 종류입니다.
         * @param[in] Commands 메시 패스가 만든 정렬된 드로 목록입니다.
         * @return 제출한 드로 호출 수입니다.
         */
        Core::Uint32 SubmitMeshCommands(const Engine::FSceneViewFamily& Family, Renderer::EMeshPass Pass, std::span<const Renderer::FMeshDrawCommand> Commands);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    private:
        std::unique_ptr<Renderer::FLightingResources> mLightingResources{};
        ReflectProperty(mLightingResources, "LightingResources", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        /**
         * @brief 장면 컬러·깊이를 읽는 후처리 셰이더와 GPU 상태의 소유 객체입니다.
         */
        std::unique_ptr<Renderer::FPostProcessResources> mPostProcessResources{};
        ReflectProperty(mPostProcessResources, "PostProcessResources", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그림자 아틀라스와 관련 GPU 자원을 관리하는 소유 객체입니다.
         */
        std::unique_ptr<Renderer::FShadowResources> mShadowResources{};
        ReflectProperty(mShadowResources, "ShadowResources", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그림자 아틀라스 타일마다 그림자 생성 메시의 깊이를 기록하는 메시 패스입니다.
         */
        std::unique_ptr<Renderer::FShadowDepthPass> mShadowDepthPass{};
        ReflectProperty(mShadowDepthPass, "ShadowDepthPass", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 보이는 메시를 머티리얼과 조명으로 장면 타깃에 그리는 메시 패스입니다.
         */
        std::unique_ptr<Renderer::FBasePass> mBasePass{};
        ReflectProperty(mBasePass, "BasePass", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 머티리얼이 없는 메시 드로에 사용할 기본 머티리얼의 소유 객체입니다.
         */
        std::unique_ptr<Asset::UMaterial> mDefaultMaterial{};
        ReflectProperty(mDefaultMaterial, "DefaultMaterial", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 텍스처 없는 머티리얼의 흰색 기본 텍스처입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mDefaultMaterialTexture{};
        ReflectProperty(mDefaultMaterialTexture, "DefaultMaterialTexture", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 머티리얼 텍스처용 반복 샘플러입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11SamplerState> mMaterialSampler{};
        ReflectProperty(mMaterialSampler, "MaterialSampler", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 셰이더와 양면 정책별 파이프라인 캐시입니다.
         */
        Core::TMap<Core::FString, std::unique_ptr<Renderer::FMaterialPipeline>> mMaterialPipelines{};
        ReflectProperty(mMaterialPipelines, "MaterialPipelines", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 폰트 아틀라스 텍스트 렌더러입니다.
         */
        std::unique_ptr<Renderer::FTextRenderer> mTextRenderer{};
        ReflectProperty(mTextRenderer, "TextRenderer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 투명 텍스처 쿼드 렌더러입니다.
         */
        std::unique_ptr<Renderer::FSpriteRenderer> mSpriteRenderer{};
        ReflectProperty(mSpriteRenderer, "SpriteRenderer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 셰이더 컴파일·캐시·재로드 관리자입니다.
         */
        RenderCore::FShaderManager mShaderManager{};
        ReflectProperty(mShaderManager, "ShaderManager", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 프로젝트의 HLSL 소스와 include 파일을 찾는 폴더입니다.
         */
        std::filesystem::path mShaderDirectory{};
        ReflectProperty(mShaderDirectory, "ShaderDirectory", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 다음 렌더링에서 GPU 셰이더를 다시 준비할지 나타냅니다.
         */
        bool mShadersDirty{};
        ReflectProperty(mShadersDirty, "ShadersDirty", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Direct3D 11 장치입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Device> mDevice{};
        ReflectProperty(mDevice, "Device", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Direct3D 11 즉시 컨텍스트입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> mContext{};
        ReflectProperty(mContext, "Context", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 화면 표시용 스왑 체인입니다.
         */
        Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain{};
        ReflectProperty(mSwapChain, "SwapChain", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 비동기 프레젠테이션에서 화면 찢김을 허용하는 사용자 설정입니다.
         */
        bool mAllowTearing{};
        ReflectProperty(mAllowTearing, "AllowTearing", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 DXGI 환경의 화면 찢김 허용 지원 여부입니다.
         */
        bool mTearingSupported{};
        ReflectProperty(mTearingSupported, "TearingSupported", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 스왑 체인 백 버퍼의 렌더 타깃 뷰입니다.
         */
        std::unique_ptr<Asset::UTexture2D> mBackBuffer{};
        ReflectProperty(mBackBuffer, "BackBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);






        /**
         * @brief 보조 선분 정점 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mLineVertexShader{};
        ReflectProperty(mLineVertexShader, "LineVertexShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 보조 선분 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mLinePixelShader{};
        ReflectProperty(mLinePixelShader, "LinePixelShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 선분 정점 입력 레이아웃입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11InputLayout> mLineLayout{};
        ReflectProperty(mLineLayout, "LineLayout", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 뷰·투영 상수 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mViewBuffer{};
        ReflectProperty(mViewBuffer, "ViewBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 화면 크기와 역수를 전달하는 선분 상수 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mLineViewBuffer{};
        ReflectProperty(mLineViewBuffer, "LineViewBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 보조 선분용 동적 정점 버퍼입니다.
         */
        RenderCore::FDynamicVertexBuffer mLineBuffer{};
        ReflectProperty(mLineBuffer, "LineBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메시 인스턴스의 월드 행렬을 업로드하는 동적 정점 버퍼입니다.
         */
        RenderCore::FDynamicVertexBuffer mMeshInstanceBuffer{};
        ReflectProperty(mMeshInstanceBuffer, "MeshInstanceBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 래스터라이저 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
        ReflectProperty(mRasterizer, "Rasterizer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 장면 전체를 와이어프레임으로 표시할 래스터라이저입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> mWireframeRasterizer{};
        ReflectProperty(mWireframeRasterizer, "WireframeRasterizer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이 검사가 켜진 깊이 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthEnabled{};
        ReflectProperty(mDepthEnabled, "DepthEnabled", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이를 쓰지 않는 깊이 검사 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthReadOnly{};
        ReflectProperty(mDepthReadOnly, "DepthReadOnly", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이 검사가 꺼진 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthDisabled{};
        ReflectProperty(mDepthDisabled, "DepthDisabled", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프리미티브 종류별 GPU 메시입니다.
         */
        std::array<std::unique_ptr<Asset::UStaticMesh>, 3> mMeshes{};
        ReflectProperty(mMeshes, "Meshes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 업로드할 보조 선분 정점입니다.
         */
        Core::TArray<Renderer::FLineVertex> mLineVertices{};
        ReflectProperty(mLineVertices, "LineVertices", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 가로 픽셀 수입니다.
         */
        Core::Uint32 mWidth{};
        ReflectProperty(mWidth, "Width", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 세로 픽셀 수입니다.
         */
        Core::Uint32 mHeight{};
        ReflectProperty(mHeight, "Height", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 프레임 렌더링 통계입니다.
         */
        RenderCore::FRenderStats mStats{};
        ReflectProperty(mStats, "Stats", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 고정 에디터 메시의 로컬 정점에 월드 행렬과 뷰·투영 행렬을 적용하는 기즈모 정점 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11VertexShader> mGizmoVertexShader{};
        ReflectProperty(mGizmoVertexShader, "GizmoVertexShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 조명 계산 없이 요청의 RGBA 색상을 출력하는 기즈모 픽셀 셰이더입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11PixelShader> mGizmoPixelShader{};
        ReflectProperty(mGizmoPixelShader, "GizmoPixelShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 고정 에디터 메시의 로컬 위치를 읽는 입력 레이아웃입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11InputLayout> mOverlayLayout{};
        ReflectProperty(mOverlayLayout, "OverlayLayout", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 에디터 메시 요청의 월드 행렬과 RGBA 색상을 전달하는 80바이트 상수 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mEditorMeshBuffer{};
        ReflectProperty(mEditorMeshBuffer, "EditorMeshBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 평면 핸들의 투명도를 표시하는 소스 알파 혼합 상태입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11BlendState> mOverlayBlend{};
        ReflectProperty(mOverlayBlend, "OverlayBlend", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 렌더러가 초기화 때 생성하고 종료 때 해제하는 enum별 불변 GPU 정점·인덱스 버퍼입니다.
         */
        std::array<std::unique_ptr<Asset::UStaticMesh>, static_cast<std::size_t>(Engine::EEditorMesh::Count)> mEditorMeshes{};
        ReflectProperty(mEditorMeshes, "EditorMeshes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
