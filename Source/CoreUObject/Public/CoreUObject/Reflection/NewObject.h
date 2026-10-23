/**
 * @file NewObject.h
 * @brief 등록된 UObject 생성과 네이티브 클래스 정의 템플릿을 선언합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/NativeReflection.h"
#include "CoreUObject/UObject.h"

/**
 * @brief UObject 파생 객체를 생성하고 전역 레지스트리에 자동 등록합니다.
 * @tparam T 적용할 네이티브 타입입니다.
 * @tparam TArguments 생성자 인수의 타입 팩입니다.
 * @param[in] Arguments 객체 생성자에 전달할 인수 팩입니다.
 * @return 생성한 객체의 단독 소유권이며, 생성 또는 검증 실패 시 nullptr입니다.
 */
template <typename T, typename... TArguments>
std::unique_ptr<T> NewObject(TArguments&&... Arguments);

/**
 * @brief 구체적이고 기본 생성 가능한 네이티브 타입의 인스턴스를 생성합니다.
 * @tparam T 적용할 네이티브 타입입니다.
 * @return 생성한 객체의 단독 소유권이며, 생성 또는 검증 실패 시 nullptr입니다.
 */
template <typename T>
std::unique_ptr<UObject> CreateNativeInstance();

/**
 * @brief 네이티브 클래스의 상속, 생성 정책과 멤버 빌더를 등록합니다.
 * @tparam T 적용할 네이티브 타입입니다.
 * @param[in] Name 조회하거나 기록할 이름입니다.
 * @param[in] Parent 부모 메타데이터 또는 부모를 조회하는 콜백입니다.
 * @param[in] Flags 타입 또는 프로퍼티의 정책 비트 플래그입니다.
 * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
 */
template <typename T>
UClass* DefineNativeClass(FString Name, UClass* (*Parent)(), EClassFlags Flags = EClassFlags::None);

#include "CoreUObject/Reflection/NewObjectTemplates.h"
