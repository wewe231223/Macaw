/**
 * @file FShaderCompilation.h
 * @brief 컴파일된 셰이더 바이트코드와 소스 파일 의존성에 대한 선언입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <filesystem>

/**
 * @brief 의존 파일의 경로와 존재 여부·크기·수정 시각을 보관합니다.
 */
struct FShaderDependency {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FShaderDependency);

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FShaderDependency);

    /**
     * @brief 의존 파일의 정규화된 경로입니다.
     */
    std::filesystem::path mPath{};
    ReflectProperty(mPath, "Path", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 의존성 검사 시 파일의 마지막 수정 시각입니다.
     */
    std::filesystem::file_time_type mWriteTime{};
    ReflectProperty(mWriteTime, "WriteTime", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 의존성 검사 시 파일의 바이트 수입니다.
     */
    Uint64 mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 의존성 검사 시 파일이 존재했는지 나타냅니다.
     */
    bool mExists{};
    ReflectProperty(mExists, "Exists", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
};

/**
 * @brief 컴파일 결과의 바이트코드와 의존성 및 생성 시각을 보관합니다.
 */
struct FShaderCompilation {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FShaderCompilation);

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FShaderCompilation);

    /**
     * @brief 컴파일된 셰이더 바이트코드입니다.
     */
    TArray<Uint8> mByteCode{};
    ReflectProperty(mByteCode, "ByteCode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 소스 및 include 파일의 존재·크기·수정 시각 스냅샷입니다.
     */
    TArray<FShaderDependency> mDependencies{};
    ReflectProperty(mDependencies, "Dependencies", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 셰이더 컴파일 결과를 생성한 시각입니다.
     */
    std::filesystem::file_time_type mBuildTime{};
    ReflectProperty(mBuildTime, "BuildTime", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
};
