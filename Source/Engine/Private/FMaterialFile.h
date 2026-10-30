/**
 * @file FMaterialFile.h
 * @brief 단일 MTL 머티리얼의 검증, 로드와 안전한 저장을 선언합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>

class FAssetManager;
class UMaterial;

/**
 * @brief 프로젝트 상대 참조를 검증하며 MTL 머티리얼을 읽고 기록합니다.
 */
class FMaterialFile {
public:
    /**
     * @brief 리플렉션 소유 타입입니다.
     */
    ReflectTypes(FMaterialFile);

public:
    /**
     * @brief 머티리얼 파일 도구의 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FMaterialFile);

    /**
     * @brief 단일 머티리얼 설정과 텍스처 참조를 읽고 검증합니다.
     * @param[in] Path MTL 파일 경로입니다.
     * @param[in,out] Assets 참조를 검증하고 텍스처를 로드할 관리자입니다.
     * @param[in,out] Material 읽은 설정을 적용할 대상입니다.
     * @param[out] Error 실패 원인입니다.
     * @return 모든 설정과 텍스처를 읽으면 true입니다.
     */
    static bool Load(const std::filesystem::path& Path, FAssetManager& Assets, UMaterial& Material, FString& Error);

    /**
     * @brief 설정을 기록한 임시 파일로 기존 MTL 파일을 교체합니다.
     * @param[in] Path 저장할 MTL 파일 경로입니다.
     * @param[in] Assets 프로젝트 경로를 검증할 관리자입니다.
     * @param[in] Material 저장할 설정입니다.
     * @param[out] Error 실패 원인입니다.
     * @return 검증과 파일 교체에 성공하면 true입니다.
     */
    static bool Save(const std::filesystem::path& Path, const FAssetManager& Assets, const UMaterial& Material, FString& Error);

private:
    /**
     * @brief 리플렉션 등록 함수에 접근을 허용합니다.
     */
    ReflectMembers();
};
