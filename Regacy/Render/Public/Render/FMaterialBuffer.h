/**
 * @file FMaterialBuffer.h
 * @brief 전역 머티리얼을 장치별 GPU 버퍼와 동기화하는 저장소를 정의합니다.
 */

#pragma once

#include <d3d11.h>
#include "Render/FRenderAssetStamp.h"
#include <wrl/client.h>
#include "Core/Base/FAssetHandle.h"
#include "RenderCore/FMaterialGPUData.h"

class UMaterial;

class FMaterialBuffer {
private:
    struct FMaterialEntry {
        FAssetHandle mHandle{};

        /**
         * @brief 머티리얼 객체 재생성과 수정 여부를 구분하는 마지막 업로드 상태입니다.
         */
        FRenderAssetStamp mStamp{};
        Uint32 mIndex{UINT32_MAX};
    };

public:
    FMaterialBuffer() = default;
    ~FMaterialBuffer() = default;

    FMaterialBuffer(const FMaterialBuffer&) = delete;

    FMaterialBuffer& operator=(const FMaterialBuffer&) = delete;

    FMaterialBuffer(FMaterialBuffer&&) = delete;

    FMaterialBuffer& operator=(FMaterialBuffer&&) = delete;

public:
    bool Initialize(ID3D11Device* Device, Uint32 MaxMaterialCount = 4096);

    /**
     * @brief 전역 레지스트리의 머티리얼을 이 GPU 버퍼와 동기화합니다.
     * @param Context 업로드할 장치 컨텍스트입니다.
     * @return 활성 서비스가 존재하고 모든 머티리얼을 반영하면 true입니다.
     * @details 서비스가 없으면 기존 인덱스 매핑을 해제합니다. 동일 GUID의 에셋도 객체가 재생성되었으면 다시 업로드합니다.
     */
    bool Synchronize(ID3D11DeviceContext* Context);
    Uint32 GetMaterialIndex(const UMaterial& Material) const;
    void BuildIndexRemapping(const FMaterialBuffer& Target, TMap<Uint32, Uint32>& OutIndices) const;
    Uint64 GetRevision() const;
    ID3D11ShaderResourceView* GetSRV() const;
    void Reset();

private:
    void ReleaseMaterial(FMaterialEntry& Entry);

private:
    TMap<FGuid, FMaterialEntry> mMaterials{};
    TArray<Uint32> mFreeIndices{};
    Uint64 mRevision{1};
    Microsoft::WRL::ComPtr<ID3D11Buffer> mBuffer{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSrv{};
};
