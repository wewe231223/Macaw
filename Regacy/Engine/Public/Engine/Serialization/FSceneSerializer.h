/**
 * @file FSceneSerializer.h
 * @brief 씬 JSON 파일의 저장, 검증, 호환 데이터 변환과 월드 로드를 제공합니다.
 */

#pragma once

#include <filesystem>

class UWorld;

/**
 * @brief 형식 버전 3의 JSON 씬을 파일과 월드 객체 사이에서 직렬화하는 진입점입니다.
 */
class FSceneSerializer final {
public:
    /**
     * @brief 월드를 JSON 문서로 직렬화하고 임시 파일을 거쳐 씬 파일을 저장합니다.
     * @param World 초기화되어 있고 바쁜 상태가 아닌 대상 월드입니다.
     * @param ScenePath 저장할 파일 경로입니다. 상위 디렉터리는 필요하면 생성합니다.
     * @return 월드 직렬화와 파일 교체가 모두 성공하면 true입니다.
     * @note 저장 준비 시 월드의 지연 액터 삭제를 처리합니다.
     */
    static bool Save(UWorld& World, const std::filesystem::path& ScenePath);

    /**
     * @brief JSON 씬을 검사·변환하고 GUID 충돌을 재매핑한 뒤 월드에 적용합니다.
     * @param World 로드 대상 월드입니다. 초기화되어 있고 바쁘지 않으며 에셋 레지스트리가 있어야 합니다.
     * @param ScenePath 읽을 JSON 씬 파일 경로입니다.
     * @return 파일 읽기, 형식·참조·이름 검증과 월드 설치가 모두 성공하면 true입니다.
     * @note 로드 범위 동안 플레이를 종료하고 이전 플레이 상태를 복원합니다. 설치 단계 실패 시 월드 액터가 비워질 수 있습니다.
     */
    static bool Load(UWorld& World, const std::filesystem::path& ScenePath);

private:
    /**
     * @brief JSON 파일을 검사·변환하고 GUID 충돌을 재매핑한 뒤 내부 월드 로드를 수행합니다.
     * @param World 로드 범위가 설정된 대상 월드입니다.
     * @param ScenePath 읽을 씬 JSON 파일 경로입니다.
     * @pre 호출자가 FLoadScope로 월드 플레이·로딩 상태를 관리해야 합니다.
     * @return 파일 읽기, 형식·참조 검증과 내부 월드 로드가 모두 성공하면 true입니다.
     */
    static bool LoadInternal(UWorld& World, const std::filesystem::path& ScenePath);
};
