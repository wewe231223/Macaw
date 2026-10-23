/**
 * @file FReflectionRegistry.h
 * @brief 이름과 네이티브 타입으로 클래스·구조체·열거형 메타데이터를 정의하고 조회합니다.
 */
#pragma once

#include "Core/Base/FName.h"
#include "CoreUObject/Reflection/EClassFlags.h"
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <utility>

class UObject;
class UClass;
class UScriptStruct;
class UEnum;
struct FNativeType;

/**
 * @brief 메타데이터를 소유하며 동일한 정의를 재사용하는 전역 리플렉션 레지스트리입니다.
 * @note 정의와 검색은 재귀 뮤텍스로 보호하며 같은 이름의 충돌 정의는 Critical 오류를 보고합니다.
 */
class FReflectionRegistry {
public:
    /**
     * @brief 네이티브 객체의 소유권을 반환하는 생성 팩터리 타입입니다.
     */
    using FFactory = std::unique_ptr<UObject> (*)();
    /**
     * @brief 클래스의 프로퍼티와 함수를 등록하는 빌더 타입입니다.
     */
    using FClassBuilder = void (*)(UClass&);
    /**
     * @brief 구조체 프로퍼티를 등록하는 빌더 타입입니다.
     */
    using FStructBuilder = void (*)(UScriptStruct&);
    /**
     * @brief 열거형 항목을 등록하는 빌더 타입입니다.
     */
    using FEnumBuilder = void (*)(UEnum&);

public:
    /**
     * @brief 클래스 메타데이터를 정의하거나 동일한 기존 정의를 반환합니다.
     * @param Name 전체 리플렉션 레지스트리에서 사용하는 타입 이름입니다.
     * @param Parent 기반 클래스를 조회할 함수입니다. nullptr이면 기반 클래스가 없습니다.
     * @param Size 네이티브 객체의 바이트 크기입니다.
     * @param Alignment 네이티브 객체의 바이트 정렬입니다.
     * @param Flags 클래스 정책 플래그입니다.
     * @param Factory 객체 생성 팩터리입니다.
     * @param Builder 최초 정의 시 실행할 멤버 등록 함수입니다.
     * @return 레지스트리가 소유한 클래스 메타데이터입니다.
     */
    static UClass* DefineClass(FName Name, UClass* (*Parent)(), std::size_t Size, std::size_t Alignment, EClassFlags Flags, FFactory Factory, FClassBuilder Builder = nullptr);
    /**
     * @brief 구조체 메타데이터를 정의하거나 동일한 기존 정의를 반환합니다.
     * @param Name 레지스트리 타입 이름입니다.
     * @param ValueType 유효한 네이티브 구조체 타입 정보입니다.
     * @param Builder 최초 정의 시 실행할 멤버 등록 함수입니다.
     * @return 레지스트리가 소유한 구조체 메타데이터입니다.
     */
    static UScriptStruct* DefineStruct(FName Name, const FNativeType* ValueType, FStructBuilder Builder = nullptr);
    /**
     * @brief 열거형 메타데이터를 정의하거나 동일한 기존 정의를 반환합니다.
     * @param Name 레지스트리 타입 이름입니다.
     * @param ValueType 네이티브 열거형 타입 정보입니다.
     * @param Builder 최초 정의 시 실행할 항목 등록 함수입니다. nullptr를 허용합니다.
     * @return 레지스트리가 소유한 열거형 메타데이터입니다.
     */
    static UEnum* DefineEnum(FName Name, const FNativeType* ValueType, FEnumBuilder Builder);
    /**
     * @brief 이름·값 목록을 주어진 순서로 열거형에 등록합니다.
     * @param Information 대상 열거형 메타데이터입니다.
     * @param Values 등록할 항목 목록입니다.
     */
    static void AddEnumValues(UEnum& Information, std::initializer_list<std::pair<FName, Int64>> Values);
    /**
     * @brief 등록된 클래스 이름을 검색합니다.
     * @param Name 검색할 이름입니다.
     * @return 클래스 정보입니다. 없거나 다른 종류의 타입이면 nullptr입니다.
     */
    static UClass* FindClass(FName Name);
    /**
     * @brief 등록된 구조체 이름을 검색합니다.
     * @param Name 검색할 이름입니다.
     * @return 구조체 정보입니다. 없거나 다른 종류의 타입이면 nullptr입니다.
     */
    static UScriptStruct* FindStruct(FName Name);
    /**
     * @brief 등록된 열거형 이름을 검색합니다.
     * @param Name 검색할 이름입니다.
     * @return 열거형 정보입니다. 없거나 다른 종류의 타입이면 nullptr입니다.
     */
    static UEnum* FindEnum(FName Name);
    /**
     * @brief 정확한 네이티브 타입 포인터로 등록 열거형을 찾습니다.
     * @param ValueType 검색할 타입 기술자입니다.
     * @return 일치하는 열거형입니다. 없으면 nullptr입니다.
     */
    static UEnum* FindEnum(const FNativeType* ValueType);
    /**
     * @brief 등록된 클래스 정보를 이름 문자열 순서로 정렬해 반환합니다.
     * @return 비소유 클래스 포인터 배열입니다.
     */
    static TArray<const UClass*> GetClasses();
    /**
     * @brief 현재 스레드가 메타데이터 정의의 부트스트랩 범위 안인지 검사합니다.
     * @return 메타데이터 정의 중이면 true입니다.
     */
    static bool IsBootstrapping();
};

/**
 * @brief 모듈별 정적 리플렉션 타입의 등록 진입점을 제공합니다.
 */
namespace ReflectionModules {
    /**
     * @brief CoreUObject 모듈의 리플렉션 타입을 등록합니다.
     */
    void RegisterCoreUObject();
    /**
     * @brief RenderCore 모듈의 리플렉션 타입을 등록합니다.
     */
    void RegisterRenderCore();
    /**
     * @brief Asset 모듈의 리플렉션 타입을 등록합니다.
     */
    void RegisterAsset();
    /**
     * @brief World 모듈의 리플렉션 타입을 등록합니다.
     */
    void RegisterWorld();
    /**
     * @brief Engine 모듈의 리플렉션 타입을 등록합니다.
     */
    void RegisterEngine();
    /**
     * @brief Editor 모듈의 리플렉션 타입을 등록합니다.
     */
    void RegisterEditor();
}
