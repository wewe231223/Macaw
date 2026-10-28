/**
 * @file FAtomicFileWriter.h
 * @brief 임시 파일 기록과 원본 파일 교체의 수명을 관리합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <filesystem>
#include <fstream>

/** @brief 완성된 임시 파일만 원본과 교체하고 미완료 파일을 범위 종료 시 정리합니다. */
class FAtomicFileWriter {
public:
    /** @brief 리플렉션 소유 타입입니다. */
    ReflectTypes(FAtomicFileWriter);

public:
    /** @brief 파일이 열리지 않은 저장 객체를 생성합니다. */
    FAtomicFileWriter() = default;
    /** @brief 커밋하지 않은 임시 파일과 스트림을 정리합니다. */
    ~FAtomicFileWriter();
    /** @brief 임시 파일 소유권의 중복을 금지합니다. */
    FAtomicFileWriter(const FAtomicFileWriter&) = delete;
    /** @brief 임시 파일 소유권의 복사 대입을 금지합니다. */
    FAtomicFileWriter& operator=(const FAtomicFileWriter&) = delete;

public:
    /** @brief 저장 객체의 리플렉션 메타데이터를 반환합니다. @return 등록된 구조체 메타데이터의 비소유 주소입니다. */
    ReflectStruct(FAtomicFileWriter);

    /** @brief 대상과 같은 폴더에 고유한 임시 파일을 엽니다. @param[in] Path 교체할 대상 경로입니다. @param[out] Error 실패 원인입니다. @return 임시 파일을 열면 true입니다. */
    bool Open(const std::filesystem::path& Path, FString& Error);
    /** @brief 바이너리 모드로 열린 출력 스트림을 반환합니다. @return 이 객체가 소유하는 출력 스트림입니다. @pre Open이 성공해야 합니다. */
    std::ostream& GetStream();
    /** @brief 기록과 닫기를 검증한 후 임시 파일로 대상을 교체합니다. @param[out] Error 실패 원인입니다. @return 파일 교체에 성공하면 true입니다. */
    bool Commit(FString& Error);

private:
    /** @brief 리플렉션 등록 함수에 접근을 허용합니다. */
    ReflectMembers();
    /** @brief 열린 스트림을 닫고 남아 있는 임시 파일을 제거합니다. */
    void Discard();

private:
    /** @brief 최종 교체할 파일의 절대 경로입니다. */
    std::filesystem::path mTarget{};
    ReflectProperty(mTarget, "Target", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 커밋 전까지 소유하는 임시 파일 경로입니다. */
    std::filesystem::path mTemporary{};
    ReflectProperty(mTemporary, "Temporary", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief 임시 파일에 기록할 출력 스트림입니다. */
    std::ofstream mStream{};
    ReflectProperty(mStream, "Stream", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
