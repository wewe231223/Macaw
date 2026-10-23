/**
 * @file FSceneDrawContext.h
 * @brief 장면 렌더링에 빌려 쓰는 장치 컨텍스트, 메시 및 파이프라인 리소스입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Renderer/FMeshResource.h"

/**
 * @brief 장면 렌더링에 빌려 쓰는 장치 컨텍스트, 메시 및 파이프라인 리소스입니다.
 * @ingroup Renderer
 */
struct FSceneDrawContext {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FSceneDrawContext);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FSceneDrawContext);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief Direct3D 11 즉시 컨텍스트입니다.
     */
    ID3D11DeviceContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 프리미티브 종류별 GPU 메시입니다.
     */
    const std::array<FMeshResource, 3>* mMeshes{};
    ReflectProperty(mMeshes, "Meshes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 사용할 정점 셰이더의 비소유 주소입니다.
     */
    ID3D11VertexShader* mVertexShader{};
    ReflectProperty(mVertexShader, "VertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 사용할 픽셀 셰이더의 비소유 주소입니다.
     */
    ID3D11PixelShader* mPixelShader{};
    ReflectProperty(mPixelShader, "PixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 사용할 정점 입력 레이아웃의 비소유 주소입니다.
     */
    ID3D11InputLayout* mInputLayout{};
    ReflectProperty(mInputLayout, "InputLayout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 뷰·투영 상수 버퍼입니다.
     */
    ID3D11Buffer* mViewBuffer{};
    ReflectProperty(mViewBuffer, "ViewBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
