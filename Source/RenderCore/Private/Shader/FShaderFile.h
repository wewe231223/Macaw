/**
 * @file FShaderFile.h
 * @brief 셰이더 경로·파일 의존성·해시와 캐시 데이터 입출력에 대한 선언입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/Shader/FShaderCompilation.h"

#include <span>
#include <string_view>

/**
 * @brief 셰이더 파일·의존성·캐시 데이터의 공통 입출력을 제공합니다.
 * @namespace ShaderFile
 */
namespace ShaderFile {
    /**
     * @brief 경로를 절대 경로로 정규화하고 실패 진단을 반환합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[out] Normalized 정규화한 절대 경로를 받을 출력입니다.
     * @param[out] Diagnostics 실패 원인과 컴파일 메시지를 받을 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool NormalizePath(const std::filesystem::path& Path, std::filesystem::path& Normalized, FString& Diagnostics);
    /**
     * @brief 파일의 존재 여부·크기·수정 시각을 의존성 스냅샷에 기록합니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[out] Dependency 파일의 경로·존재 여부·크기·수정 시각 스냅샷입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool Inspect(const std::filesystem::path& Path, FShaderDependency& Dependency);
    /**
     * @brief 셰이더 파일의 크기를 검증하여 바이트와 의존성 스냅샷을 읽습니다.
     * @param[in] Path 조회·로드·저장 또는 정규화할 파일 경로입니다.
     * @param[in,out] Data 처리할 바이트 데이터 또는 include 파일 버퍼입니다.
     * @param[out] Dependency 파일의 경로·존재 여부·크기·수정 시각 스냅샷입니다.
     * @param[out] Diagnostics 실패 원인과 컴파일 메시지를 받을 문자열입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool Read(const std::filesystem::path& Path, TArray<Uint8>& Data, FShaderDependency& Dependency, FString& Diagnostics);
    /**
     * @brief 의존 파일의 존재·크기·수정 시각이 저장된 스냅샷과 일치하는지 검사합니다.
     * @param[in] Dependencies 현재 파일 상태와 비교할 의존성 스냅샷 목록입니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    bool IsSnapshotCurrent(const TArray<FShaderDependency>& Dependencies);
    /**
     * @brief 입력 바이트의 SHA-256 해시를 16진수 문자열로 반환합니다.
     * @param[in] Data 처리할 바이트 데이터 또는 include 파일 버퍼입니다.
     * @return SHA-256 해시의 16진수 문자열이며 계산 실패 시 빈 문자열입니다.
     */
    FString Hash(std::span<const Uint8> Data);
    /**
     * @brief 셰이더 캐시 구분에 사용하는 컴파일러 식별 문자열을 반환합니다.
     * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
     */
    const FString& GetCompilerIdentity();
    /**
     * @brief 64비트 부호 없는 값을 리틀 엔디언으로 버퍼 끝에 기록합니다.
     * @param[in,out] Data 처리할 바이트 데이터 또는 include 파일 버퍼입니다.
     * @param[in] Value 읽거나 기록·변환할 값입니다.
     */
    void AppendUint64(TArray<Uint8>& Data, Uint64 Value);
    /**
     * @brief 버퍼 범위를 검사하여 리틀 엔디언 64비트 값을 읽고 위치를 전진시킵니다.
     * @param[in] Data 처리할 바이트 데이터 또는 include 파일 버퍼입니다.
     * @param[in,out] Position 다음에 읽을 바이트 위치이며 성공한 읽기만큼 전진합니다.
     * @param[in,out] Value 읽거나 기록·변환할 값입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool ReadUint64(std::span<const Uint8> Data, std::size_t& Position, Uint64& Value);
    /**
     * @brief 문자열 길이와 바이트를 셰이더 캐시 버퍼 끝에 기록합니다.
     * @param[in,out] Data 처리할 바이트 데이터 또는 include 파일 버퍼입니다.
     * @param[in] Value 읽거나 기록·변환할 값입니다.
     */
    void AppendString(TArray<Uint8>& Data, std::string_view Value);
    /**
     * @brief 버퍼 범위를 검사하여 길이 접두사가 있는 문자열을 읽고 위치를 전진시킵니다.
     * @param[in] Data 처리할 바이트 데이터 또는 include 파일 버퍼입니다.
     * @param[in,out] Position 다음에 읽을 바이트 위치이며 성공한 읽기만큼 전진합니다.
     * @param[in,out] Value 읽거나 기록·변환할 값입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool ReadString(std::span<const Uint8> Data, std::size_t& Position, FString& Value);
}
