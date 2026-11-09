/**
 * @file FShaderIncludeHandler.h
 * @brief 셰이더 include 탐색과 열린 파일 및 의존성 추적에 대한 선언입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"
#include "RenderCore/Shader/FShaderDescription.h"

namespace RenderCore {

    /**
     * @brief 셰이더 include 탐색과 열린 파일 및 의존성 추적을 담당합니다.
     * @class FShaderIncludeHandler
     */
    /**
     * @brief Direct3D 셰이더 컴파일러가 include 파일을 요청하는 인터페이스입니다.
     * @class ID3DInclude
     */
    class FShaderIncludeHandler final : public ID3DInclude {
    private:
        /**
         * @brief 열린 include 파일의 경로와 컴파일러에 전달할 버퍼입니다.
         */
        struct FOpenFile {
        public:
            /**
             * @brief 열린 include 파일의 정규화된 경로입니다.
             */
            std::filesystem::path mPath{};

            /**
             * @brief 열린 include 파일의 내용을 소유하는 바이트 배열입니다.
             */
            Core::TArray<Core::Uint8> mData{};
        };

    public:
        /**
         * @brief 전달받은 설정과 의존성으로 FShaderIncludeHandler의 초기 상태를 구성합니다.
         * @param[in] Description 소스 경로·진입점·프로필·매크로 등의 컴파일 설정입니다.
         * @param[in] RootData 최상위 셰이더 소스 버퍼의 비소유 주소입니다.
         * @param[in] RootDependency 최상위 셰이더 소스 파일의 의존성 스냅샷입니다.
         */
        FShaderIncludeHandler(const RenderCore::FShaderDescription& Description, const void* RootData, const RenderCore::FShaderDependency& RootDependency);

    public:
        /**
         * @brief 부모 파일과 include 경로에서 요청한 파일을 찾아 버퍼와 의존성을 보관합니다.
         * @param[in] IncludeType 로컬 또는 시스템 include 탐색 종류입니다.
         * @param[in] FileName 컴파일러가 요청한 include 파일 이름입니다.
         * @param[in] ParentData 현재 include를 요청한 부모 소스 버퍼의 비소유 주소입니다.
         * @param[out] Data include 내용 버퍼의 비소유 주소를 받을 출력이며 Close까지 유효합니다.
         * @param[out] Bytes 컴파일러에 반환할 include 파일의 바이트 수입니다.
         * @return 성공하면 S_OK, 실패하면 원인을 나타내는 HRESULT입니다.
         */
        HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR FileName, LPCVOID ParentData, LPCVOID* Data, UINT* Bytes) override;

        /**
         * @brief 컴파일러가 반환한 include 데이터에 대응하는 열린 파일 버퍼를 해제합니다.
         * @param[in] Data Open()이 반환한 include 파일 버퍼입니다.
         * @return 성공하면 S_OK, 실패하면 원인을 나타내는 HRESULT입니다.
         */
        HRESULT STDMETHODCALLTYPE Close(LPCVOID Data) override;

        /**
         * @brief 컴파일 중 확인한 소스 및 include 파일의 의존성 스냅샷을 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::TArray<RenderCore::FShaderDependency>& GetDependencies() const;

        /**
         * @brief include 파일 탐색과 읽기 과정의 진단 메시지를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Core::FString& GetDiagnostics() const;

    private:
        /**
         * @brief 파일의 의존성을 등록하고 이미 등록된 상태와 충돌하는지 검사합니다.
         * @param[in] Dependency 파일의 경로·존재 여부·크기·수정 시각 스냅샷입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool TrackDependency(const RenderCore::FShaderDependency& Dependency);

    private:
        /**
         * @brief include 탐색 설정을 제공하는 셰이더 설명의 비소유 주소입니다.
         */
        const RenderCore::FShaderDescription* mDescription{};

        /**
         * @brief 최상위 셰이더 소스 버퍼의 비소유 주소입니다.
         */
        const void* mRootData{};

        /**
         * @brief 소스 및 include 파일의 존재·크기·수정 시각 스냅샷입니다.
         */
        Core::TArray<RenderCore::FShaderDependency> mDependencies{};

        /**
         * @brief 컴파일러에 전달한 데이터 주소별로 소유하는 include 파일 버퍼입니다.
         */
        Core::TMap<const void*, FOpenFile> mOpenFiles{};

        /**
         * @brief include 탐색 또는 파일 읽기 실패의 진단 문자열입니다.
         */
        Core::FString mDiagnostics{};
    };

}
