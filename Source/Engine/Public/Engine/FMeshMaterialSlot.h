/** @file FMeshMaterialSlot.h @brief 메시 재질 슬롯의 이름과 에셋 경로입니다. @ingroup Engine */
#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"

/** @brief 임포트한 원본 재질 이름과 Content 기준 재질 경로를 보관합니다. */
class FMeshMaterialSlot {
public:
    /** @brief 리플렉션 소유 타입의 별칭입니다. */
    ReflectTypes(FMeshMaterialSlot);

public:
    /** @brief 비어 있는 재질 슬롯을 구성합니다. */
    FMeshMaterialSlot() = default;
    /** @brief 재질 슬롯을 구성합니다. @param Name 원본 재질 이름입니다. @param MaterialPath Content 기준 재질 경로입니다. */
    FMeshMaterialSlot(FString Name, FString MaterialPath);

public:
    /** @brief 정적 타입 메타데이터를 반환합니다. @return 등록된 구조체 메타데이터입니다. */
    ReflectStruct(FMeshMaterialSlot);
    /** @brief 슬롯 이름을 반환합니다. @return 원본 재질 이름의 참조입니다. */
    const FString& GetName() const;
    /** @brief 재질 경로를 반환합니다. @return Content 기준 재질 경로의 참조입니다. */
    const FString& GetMaterialPath() const;

private:
    /** @brief 생성된 리플렉션 멤버 등록 함수의 접근을 허용합니다. */
    ReflectMembers();

private:
    /** @brief 원본 파일에 기록된 재질 이름입니다. */
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /** @brief Content 기준 .mtl 에셋 경로이며 빈 경로는 기본 재질입니다. */
    FString mMaterialPath{};
    ReflectProperty(mMaterialPath, "MaterialPath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
