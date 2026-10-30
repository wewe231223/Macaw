/**
 * @file FSubMesh.h
 * @brief 공유 정점 버퍼에서 재질별로 그릴 인덱스 구간입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"

/**
 * @brief 하나의 재질 슬롯을 사용하는 연속 삼각형 인덱스 구간입니다.
 */
class FSubMesh {
public:
    /**
     * @brief 리플렉션 소유 타입의 별칭입니다.
     */
    ReflectTypes(FSubMesh);

public:
    /**
     * @brief 비어 있는 구간을 구성합니다.
     */
    FSubMesh() = default;

    /**
     * @brief 공유 인덱스 버퍼의 구간과 재질을 지정합니다.
     * @param FirstIndex 첫 인덱스의 오프셋입니다.
     * @param IndexCount 삼각형 인덱스 개수입니다.
     * @param MaterialIndex 재질 슬롯의 인덱스입니다.
     */
    FSubMesh(Uint32 FirstIndex, Uint32 IndexCount, Uint32 MaterialIndex);

public:
    /**
     * @brief 정적 타입 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터입니다.
     */
    ReflectStruct(FSubMesh);

    /**
     * @brief 첫 인덱스 오프셋을 반환합니다.
     * @return 공유 인덱스 버퍼의 오프셋입니다.
     */
    Uint32 GetFirstIndex() const;

    /**
     * @brief 인덱스 개수를 반환합니다.
     * @return 삼각형 인덱스 개수입니다.
     */
    Uint32 GetIndexCount() const;

    /**
     * @brief 재질 슬롯을 반환합니다.
     * @return 메시 재질 배열의 인덱스입니다.
     */
    Uint32 GetMaterialIndex() const;

private:
    /**
     * @brief 생성된 리플렉션 멤버 등록 함수의 접근을 허용합니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 공유 인덱스 버퍼의 시작 오프셋입니다.
     */
    Uint32 mFirstIndex{};
    ReflectProperty(mFirstIndex, "FirstIndex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이 구간의 인덱스 개수입니다.
     */
    Uint32 mIndexCount{};
    ReflectProperty(mIndexCount, "IndexCount", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이 구간에 할당된 재질 슬롯입니다.
     */
    Uint32 mMaterialIndex{};
    ReflectProperty(mMaterialIndex, "MaterialIndex", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
