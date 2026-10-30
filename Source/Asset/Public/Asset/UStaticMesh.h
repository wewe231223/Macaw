/**
 * @file UStaticMesh.h
 * @brief CPU 메시 데이터와 Direct3D 정점·인덱스 버퍼 관리에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "CoreUObject/UObject.h"
#include "Asset/FMeshData.h"

#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

namespace Asset {

    /**
     * @brief CPU 메시 데이터와 Direct3D 정점·인덱스 버퍼 관리를 담당합니다.
     */
    class UStaticMesh final : public CoreUObject::UObject {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(UStaticMesh, CoreUObject::UObject);

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
         * @brief 이 타입의 클래스 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UStaticMesh);

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
         * @brief 정점 위치 GPU 버퍼의 비소유 주소를 반환합니다.
         * @return GPU 자원의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        ID3D11Buffer* GetPositionBuffer() const;

        /**
         * @brief 법선과 탄젠트 GPU 버퍼의 비소유 주소를 반환합니다.
         * @return GPU 자원의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        ID3D11Buffer* GetTangentBuffer() const;

        /**
         * @brief UV 채널을 담은 GPU 버퍼의 비소유 주소를 반환합니다.
         * @return GPU 자원의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        ID3D11Buffer* GetUVBuffer() const;

        /**
         * @brief 정점 색상 GPU 버퍼의 비소유 주소를 반환합니다.
         * @return GPU 자원의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        ID3D11Buffer* GetColorBuffer() const;

        /**
         * @brief 삼각형 인덱스 GPU 버퍼의 비소유 주소를 반환합니다.
         * @return GPU 자원의 비소유 주소이며, 아직 생성하지 않았으면 nullptr입니다.
         */
        ID3D11Buffer* GetIndexBuffer() const;

        /**
         * @brief 정점 하나의 모든 UV 채널이 차지하는 바이트 수를 반환합니다.
         * @return 정점 하나의 모든 UV 채널이 차지하는 바이트 수를 나타내는 정수 값입니다.
         */
        Core::Uint32 GetUVStride() const;

        /**
         * @brief 현재 보유한 GPU 리소스의 추정 바이트 수를 반환합니다.
         * @return 현재 보유한 GPU 리소스의 추정 바이트 수를 나타내는 정수 값입니다.
         */
        std::size_t GetResourceBytes() const;

        /**
         * @brief 정점 하나의 위치 데이터가 차지하는 바이트 수를 반환합니다.
         * @return 정점 하나의 위치 데이터가 차지하는 바이트 수를 나타내는 정수 값입니다.
         */
        static Core::Uint32 GetPositionStride();

        /**
         * @brief 정점 하나의 법선·탄젠트 데이터가 차지하는 바이트 수를 반환합니다.
         * @return 정점 하나의 법선·탄젠트 데이터가 차지하는 바이트 수를 나타내는 정수 값입니다.
         */
        static Core::Uint32 GetTangentStride();

        /**
         * @brief 정점 하나의 색상 데이터가 차지하는 바이트 수를 반환합니다.
         * @return 정점 하나의 색상 데이터가 차지하는 바이트 수를 나타내는 정수 값입니다.
         */
        static Core::Uint32 GetColorStride();

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();

        /**
         * @brief 크기와 입력을 검증하여 초기 데이터가 있는 GPU 버퍼를 생성합니다.
         * @param[in] Device GPU 리소스를 생성할 Direct3D 장치의 비소유 주소입니다.
         * @param[in] Data 버퍼 초기 데이터의 주소입니다.
         * @param[in] Bytes 직렬화할 바이트 수입니다.
         * @param[in] BindFlags 생성할 Direct3D 버퍼의 바인딩 용도입니다.
         * @param[out] Buffer 생성한 GPU 버퍼를 받을 COM 소유 참조입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        static bool CreateBuffer(ID3D11Device* Device, const void* Data, std::size_t Bytes, Core::Uint32 BindFlags, Microsoft::WRL::ComPtr<ID3D11Buffer>& Buffer);

    private:
        /**
         * @brief 메시의 CPU 정점 속성·인덱스·경계 데이터입니다.
         */
        Asset::FMeshData mMeshData{};
        ReflectProperty(mMeshData, "MeshData", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정점 위치를 보관하는 소유 GPU 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mPositionBuffer{};
        ReflectProperty(mPositionBuffer, "PositionBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정점 법선과 탄젠트를 보관하는 소유 GPU 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mTangentBuffer{};
        ReflectProperty(mTangentBuffer, "TangentBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정점별 UV 채널을 보관하는 소유 GPU 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mUVBuffer{};
        ReflectProperty(mUVBuffer, "UVBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정점 색상을 보관하는 소유 GPU 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mColorBuffer{};
        ReflectProperty(mColorBuffer, "ColorBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 삼각형 인덱스를 보관하는 소유 GPU 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer{};
        ReflectProperty(mIndexBuffer, "IndexBuffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 보유한 GPU 리소스의 추정 바이트 수입니다.
         */
        std::size_t mResourceBytes{};
        ReflectProperty(mResourceBytes, "ResourceBytes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
