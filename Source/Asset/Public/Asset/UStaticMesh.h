/**
 * @file UStaticMesh.h
 * @brief CPU 메시 데이터와 Direct3D 정점·인덱스 버퍼 관리에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "CoreUObject/UObject.h"
#include "Asset/FMeshData.h"
#include "RenderCore/TIndexBuffer.h"
#include "RenderCore/TVertexBuffer.h"

#include <d3d11.h>
#include <array>
#include <filesystem>
#include "UStaticMesh.generated.h"

namespace Asset {

    /**
     * @brief CPU 메시 데이터와 Direct3D 정점·인덱스 버퍼 관리를 담당합니다.
     */
    UCLASS()
    class UStaticMesh final : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        /**
         * @brief UStaticMesh의 초기 상태를 구성합니다.
         */
        UStaticMesh();

        /**
         * @brief 전달받은 설정과 의존성으로 UStaticMesh의 초기 상태를 구성합니다.
         * @param[in] MeshPath 읽을 메시 파일의 경로입니다.
         */
        explicit UStaticMesh(const std::filesystem::path& MeshPath);

        /**
         * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~UStaticMesh() override;

    public:
        /**
         * @brief 외부 메시 파일을 가져와 정점 속성과 인덱스 및 경계를 구성합니다.
         * @param[in] MeshPath 읽을 메시 파일의 경로입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool LoadFromFile(const std::filesystem::path& MeshPath);

        /**
         * @brief 메시 파일을 읽고 지정된 에셋 관리자로 재질과 텍스처를 해석합니다.
         * @param[in] MeshPath 읽을 메시 파일의 경로입니다.
         * @param[out] Error 실패 원인을 받을 문자열입니다.
         * @return 메시 데이터와 필요한 참조를 구성하면 true이며 실패하면 Error에 원인을 기록합니다.
         */
        bool LoadFromFile(const std::filesystem::path& MeshPath, Core::FString& Error);

        /**
         * @brief CPU 메시 데이터의 유효성과 경계를 검사하여 교체하고 GPU 버퍼를 해제합니다.
         * @param[in] MeshData 검증 후 소유할 새 CPU 메시 데이터입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetMeshData(Asset::FMeshData MeshData);

        /**
         * @brief 메시의 CPU 정점 속성·인덱스 데이터 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Asset::FMeshData& GetMeshData() const;

        /**
         * @brief 메시의 로컬 공간 경계 상자를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::FBox& GetBounds() const;

        /**
         * @brief 메시의 정점 개수를 반환합니다.
         * @return 메시의 정점 개수를 나타내는 정수 값입니다.
         */
        Core::Uint32 GetVertexCount() const;

        /**
         * @brief 메시의 인덱스 개수를 반환합니다.
         * @return 메시의 인덱스 개수를 나타내는 정수 값입니다.
         */
        Core::Uint32 GetIndexCount() const;

        /**
         * @brief 메시에 포함된 UV 채널 수를 반환합니다.
         * @return 메시에 포함된 UV 채널 수를 나타내는 정수 값입니다.
         */
        Core::Uint32 GetUVChannelCount() const;

        /**
         * @brief CPU 메시의 정점·인덱스·속성 배열이 렌더링 조건을 만족하는지 검사합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsValid() const;

        /**
         * @brief 현재 CPU 데이터를 사용하여 Direct3D 리소스를 생성합니다.
         * @param[in] Device GPU 리소스를 생성할 Direct3D 장치의 비소유 주소입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool CreateResources(ID3D11Device* Device);

        /**
         * @brief 소유한 GPU 리소스와 뷰를 해제하고 메모리 집계를 초기화합니다.
         */
        void ReleaseResources();

        /**
         * @brief 사용할 수 있는 GPU 리소스를 보유하는지 검사합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool HasResources() const;

        /**
         * @brief 위치·법선·UV·색상 버퍼를 입력 슬롯 0~3에, 인덱스 버퍼를 함께 연결합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         */
        void Bind(ID3D11DeviceContext* Context) const;

        /**
         * @brief 위치 버퍼만 입력 슬롯 0에, 인덱스 버퍼를 함께 연결합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         */
        void BindPositions(ID3D11DeviceContext* Context) const;

        /**
         * @brief 현재 보유한 GPU 리소스의 추정 바이트 수를 반환합니다.
         * @return 현재 보유한 GPU 리소스의 추정 바이트 수를 나타내는 정수 값입니다.
         */
        std::size_t GetResourceBytes() const;

    private:
        /**
         * @brief 원소 타입 T를 담는 불변 정점 버퍼입니다.
         */
        template <typename T>
        using TMeshVertexBuffer = RenderCore::TVertexBuffer<T, RenderCore::EBufferUpdate::Immutable>;

    private:
        /**
         * @brief 메시의 CPU 정점 속성·인덱스·경계 데이터입니다.
         */
        Asset::FMeshData mMeshData{};

        /**
         * @brief 정점 위치를 보관하는 소유 GPU 버퍼입니다.
         */
        TMeshVertexBuffer<std::array<float, 3>> mPositionBuffer{};

        /**
         * @brief 정점 법선과 탄젠트를 보관하는 소유 GPU 버퍼입니다.
         */
        TMeshVertexBuffer<std::array<float, 7>> mTangentBuffer{};

        /**
         * @brief 정점별 UV 채널을 보관하는 소유 GPU 버퍼이며 정점 하나가 채널 수만큼의 원소를 차지합니다.
         */
        TMeshVertexBuffer<std::array<float, 2>> mUVBuffer{};

        /**
         * @brief 정점 색상을 보관하는 소유 GPU 버퍼입니다.
         */
        TMeshVertexBuffer<std::array<float, 4>> mColorBuffer{};

        /**
         * @brief 삼각형 인덱스를 보관하는 소유 GPU 버퍼입니다.
         */
        RenderCore::TIndexBuffer<Core::Index_t> mIndexBuffer{};
    };

}
