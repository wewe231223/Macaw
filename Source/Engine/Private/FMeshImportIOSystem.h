/**
 * @file FMeshImportIOSystem.h
 * @brief MTL 읽기에서 지원하지 않는 Prfl 행을 제거하는 Assimp 파일 입출력입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <assimp/DefaultIOSystem.h>

/**
 * @brief MTL 읽기에서 지원하지 않는 Prfl 행을 제거하는 Assimp 파일 입출력입니다.
 * @ingroup Engine
 */
class FMeshImportIOSystem final : public Assimp::IOSystem {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FMeshImportIOSystem);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FMeshImportIOSystem);

    /**
     * @brief 기본 Assimp 파일 시스템으로 파일의 존재 여부를 검사합니다.
     * @param[in] File 처리할 파일의 경로입니다.
     * @return 파일이 존재하면 true입니다.
     */
    bool Exists(const char* File) const override;
    /**
     * @brief 기본 Assimp 파일 시스템의 경로 구분자를 반환합니다.
     * @return 운영체제의 디렉터리 구분 문자입니다.
     */
    char getOsSeparator() const override;
    /**
     * @brief 파일을 열고 읽기용 MTL이면 Prfl 행을 제거한 메모리 스트림을 생성합니다.
     * @param[in] File 처리할 파일의 경로입니다.
     * @param[in] Mode 읽기·쓰기와 바이너리 여부를 지정하는 Assimp 파일 열기 모드입니다.
     * @return 호출자가 Close()로 해제할 스트림이며 실패하면 nullptr입니다.
     */
    Assimp::IOStream* Open(const char* File, const char* Mode = "rb") override;
    /**
     * @brief Assimp 스트림을 삭제하고 스트림이 소유한 메모리를 해제합니다.
     * @param[in] Stream Close()로 해제할 Assimp 스트림입니다.
     */
    void Close(Assimp::IOStream* Stream) override;
    /**
     * @brief 기본 Assimp 파일 시스템으로 두 경로가 같은지 비교합니다.
     * @param[in] First 비교할 첫 번째 파일 경로입니다.
     * @param[in] Second 비교할 두 번째 파일 경로입니다.
     * @return 두 경로가 같은 파일을 가리키면 true입니다.
     */
    bool ComparePaths(const char* First, const char* Second) const override;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief MTL 필터링 이외의 파일 처리를 위임할 Assimp 기본 입출력입니다.
     */
    Assimp::DefaultIOSystem mDefaultIOSystem{};
    ReflectProperty(mDefaultIOSystem, "DefaultIOSystem", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
