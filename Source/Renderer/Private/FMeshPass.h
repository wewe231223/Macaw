/**
 * @file FMeshPass.h
 * @brief 장면 프리미티브를 선별해 정렬된 메시 드로 목록을 만드는 메시 패스의 기반 클래스입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "EMeshPass.h"
#include "FMeshDrawCommand.h"

#include <array>
#include <memory>

#include <d3d11.h>

namespace Asset {
    class UStaticMesh;
}

namespace Engine {
    struct FSceneViewFamily;
}

namespace Renderer {

    class FRenderer;
    class FScene;
    struct FScenePrimitive;

    /**
     * @brief 장면 프리미티브를 선별해 정렬된 메시 드로 목록을 만드는 메시 패스의 기반 클래스입니다.
     * @details 프러스텀 컬링·메시 준비·정렬은 이 클래스가 공통으로 수행하고, 대상 프리미티브와 파이프라인·머티리얼 선택은 파생 패스가 정합니다. 타깃 바인딩과 드로 제출은 렌더러가 담당합니다.
     */
    class FMeshPass {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FMeshPass);

    public:
        /**
         * @brief 파이프라인 조회와 메시 자원 생성에 사용할 렌더러 의존성을 보관합니다.
         * @param[in] Renderer 파이프라인과 기본 머티리얼을 조회할 렌더러입니다.
         * @param[in] Device 메시·머티리얼 GPU 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         * @param[in] Context 머티리얼 상수 업로드에 사용할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         * @param[in] Meshes 기본 프리미티브 종류별 공유 메시의 소유 배열입니다.
         */
        FMeshPass(Renderer::FRenderer& Renderer, ID3D11Device* Device, ID3D11DeviceContext* Context, const std::array<std::unique_ptr<Asset::UStaticMesh>, 3>& Meshes);

        /**
         * @brief 객체 수명을 끝냅니다.
         */
        virtual ~FMeshPass();

        /**
         * @brief 렌더러가 소유하는 고유 패스이므로 복사를 금지합니다.
         */
        FMeshPass(const Renderer::FMeshPass&) = delete;

        /**
         * @brief 렌더러가 소유하는 고유 패스이므로 대입을 금지합니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Renderer::FMeshPass& operator=(const Renderer::FMeshPass&) = delete;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMeshPass);

        /**
         * @brief 입력 레이아웃과 패스 전후 바인딩 정책을 결정할 패스 종류를 반환합니다.
         * @return 이 패스의 종류입니다.
         */
        virtual Renderer::EMeshPass GetType() const = 0;

        /**
         * @brief 패스 정책으로 프리미티브를 선별하고 파이프라인·머티리얼·메시 순으로 정렬된 드로 목록을 만듭니다.
         * @param[in] Scene 그릴 프리미티브를 가진 렌더 장면입니다.
         * @param[in] Family 표시 모드와 표시 플래그를 가진 현재 뷰 요청입니다.
         * @param[in] ViewProjection 프러스텀 컬링에 사용할 월드→클립 행렬입니다.
         * @param[out] Commands 정렬된 드로 목록을 받습니다.
         * @param[out] Primitives 드로 목록에 포함된 프리미티브 수를 받습니다.
         * @return 패스에 필요한 기본 파이프라인을 준비하면 true입니다.
         */
        bool BuildCommands(const Renderer::FScene& Scene, const Engine::FSceneViewFamily& Family, const Core::FMatrix& ViewProjection, Core::TArray<Renderer::FMeshDrawCommand>& Commands, Core::Uint32& Primitives);

    protected:
        /**
         * @brief 드로 목록을 만들기 전에 패스가 공통으로 사용할 파이프라인과 머티리얼을 준비합니다.
         * @param[in] Family 표시 모드를 가진 현재 뷰 요청입니다.
         * @return 기본 파이프라인을 준비하면 true이며, false이면 이번 패스를 그리지 않습니다.
         */
        virtual bool BeginCommands(const Engine::FSceneViewFamily& Family) = 0;

        /**
         * @brief 프리미티브가 이 패스의 대상인지 검사합니다.
         * @details 텍스트·스프라이트 제외와 프러스텀 컬링은 기반 클래스가 따로 수행합니다.
         * @param[in] Primitive 검사할 렌더 프리미티브입니다.
         * @param[in] Family 표시 플래그를 가진 현재 뷰 요청입니다.
         * @return 이 패스에서 그려야 하면 true입니다.
         */
        virtual bool Includes(const Renderer::FScenePrimitive& Primitive, const Engine::FSceneViewFamily& Family) const = 0;

        /**
         * @brief 준비된 메시의 SubMesh마다 이 패스의 드로 명령을 추가합니다.
         * @param[in] Primitive 월드 행렬과 머티리얼 슬롯을 가진 렌더 프리미티브입니다.
         * @param[in] Mesh GPU 자원이 준비된 정적 메시입니다.
         * @param[in] Family 표시 모드를 가진 현재 뷰 요청입니다.
         * @param[in,out] Commands 드로 명령을 추가할 목록입니다.
         */
        virtual void AddCommands(const Renderer::FScenePrimitive& Primitive, Asset::UStaticMesh& Mesh, const Engine::FSceneViewFamily& Family, Core::TArray<Renderer::FMeshDrawCommand>& Commands) = 0;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    protected:
        /**
         * @brief 파이프라인과 기본 머티리얼을 조회할 렌더러의 비소유 주소입니다.
         */
        Renderer::FRenderer* mRenderer{};
        ReflectProperty(mRenderer, "Renderer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 메시·머티리얼 GPU 자원 생성에 사용할 Direct3D 장치의 비소유 주소입니다.
         */
        ID3D11Device* mDevice{};
        ReflectProperty(mDevice, "Device", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 머티리얼 상수 업로드에 사용할 Direct3D 11 장치 컨텍스트의 비소유 주소입니다.
         */
        ID3D11DeviceContext* mContext{};
        ReflectProperty(mContext, "Context", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 기본 프리미티브 종류별 공유 메시 배열의 비소유 주소입니다.
         */
        const std::array<std::unique_ptr<Asset::UStaticMesh>, 3>* mMeshes{};
        ReflectProperty(mMeshes, "Meshes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
