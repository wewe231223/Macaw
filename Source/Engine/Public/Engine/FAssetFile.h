/**
 * @file FAssetFile.h
 * @brief 에셋 파일의 종류, 버전, 무결성 검사와 바이너리 입출력에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

#include <filesystem>
#include <span>

/**
 * @brief Macaw 에셋 파일의 페이로드 종류를 구분합니다.
 */
enum class EAssetType : Uint32 {
    /**
     * @brief 액터·컴포넌트와 월드 참조를 저장하는 장면 에셋입니다.
     */
    Scene = 1,
    /**
     * @brief 정점 속성과 인덱스를 저장하는 메시 에셋입니다.
     */
    Mesh,
    /**
     * @brief 픽셀 데이터와 텍스처 설정을 저장하는 이미지 에셋입니다.
     */
    Texture,
    /**
     * @brief 셰이더·기본색과 텍스처 참조를 가진 재질 에셋입니다.
     */
    Material
};

/**
 * @brief 이 열거형의 리플렉션 메타데이터를 반환합니다.
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */
ReflectEnum(EAssetType, Scene, Mesh, Texture, Material);
/**
 * @brief 타입 태그로 EAssetType의 열거형 메타데이터를 조회합니다.
 * @fn UEnum* GetNativeEnum(TNativeEnum<EAssetType>)
 * @return 등록된 열거형 메타데이터의 비소유 주소입니다.
 */

/**
 * @brief 에셋 파일의 종류, 버전, 무결성 검사와 바이너리 입출력를 담당합니다.
 */
class FAssetFile {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FAssetFile);

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FAssetFile);
    /**
     * @brief 에셋 파일 헤더를 검사하여 저장된 에셋 종류를 읽습니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[out] Type 에셋 파일에 기록하거나 검사할 종류입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    static bool GetType(const std::filesystem::path& Path, EAssetType& Type, FString& Error);
    /**
     * @brief 에셋 종류와 페이로드 및 무결성 해시를 파일로 저장합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[in] Type 에셋 파일에 기록하거나 검사할 종류입니다.
     * @param[in] Payload 헤더를 제외한 에셋 본문의 바이트 데이터입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    static bool Save(const std::filesystem::path& Path, EAssetType Type, std::span<const Uint8> Payload, FString& Error);
    /**
     * @brief 에셋 종류와 데이터 무결성을 검사하여 페이로드를 읽습니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[in] Type 에셋 파일에 기록하거나 검사할 종류입니다.
     * @param[out] Payload 헤더를 제외한 에셋 본문의 바이트 데이터입니다.
     * @param[out] Error 실패 원인을 받을 오류 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    static bool Load(const std::filesystem::path& Path, EAssetType Type, TArray<Uint8>& Payload, FString& Error);

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
};
