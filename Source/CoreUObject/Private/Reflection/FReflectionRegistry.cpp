/**
 * @file FReflectionRegistry.cpp
 * @brief 네이티브 타입 정의를 검사하고 프로세스 수명의 메타데이터를 등록합니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "FRegisteredType.h"
#include "CoreUObject/Reflection/FEnumValue.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/UScriptStruct.h"
#include "CoreUObject/Reflection/UEnum.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/FNativeType.h"
#include "Core/Base/ErrorHandler.h"

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 프로세스 수명 동안 유지하는 네이티브 타입 등록 맵을 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    TMap<FString, FRegisteredType>& GetRegisteredTypes() {
        static TMap<FString, FRegisteredType> Types{};
        return Types;
    }

    /**
     * @brief 이미 등록된 정의의 일치 여부를 검사하고 대응 메타데이터를 반환합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in] Definition 기존 등록과 비교할 타입 정의입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UStruct* FindDefinition(const FString& Name, const FReflectionDefinition& Definition) {
        const auto Position{GetRegisteredTypes().find(Name)};

        if (Position == GetRegisteredTypes().end()) {
            return nullptr;
        }

        if (!(Position->second.mDefinition == Definition)) {
            ErrorHandler::Fatal("FReflectionRegistry", "Conflicting reflected type: " + Name);
        }

        return Position->second.mInformation.get();
    }

    /**
     * @brief 이름과 타입 종류가 일치하는 등록 메타데이터를 조회합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in] Kind 조회할 메타데이터 종류입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    UStruct* FindType(const FString& Name, EReflectionType Kind) {
        const auto Position{GetRegisteredTypes().find(Name)};
        return Position != GetRegisteredTypes().end() && Position->second.mDefinition.mKind == Kind ? Position->second.mInformation.get() : nullptr;
    }
}

/**
 * @brief 같은 이름의 정의와 호환성을 검사한 뒤 클래스 메타데이터를 등록합니다.
 */
UClass* FReflectionRegistry::DefineClass(FString Name, UClass* (*Parent)(), const FNativeType* NativeType, EClassFlags Flags, FFactory Factory, FClassBuilder Builder) {
    ErrorHandler::Report(Name.empty() || NativeType == nullptr, "FReflectionRegistry", "Invalid class definition", ErrorHandler::EErrorLevel::Critical);
    const FReflectionDefinition Definition{EReflectionType::Class, NativeType, Parent, Flags, Factory, Builder};

    if (UStruct* Existing{FindDefinition(Name, Definition)}) {
        return static_cast<UClass*>(Existing);
    }

    UClass* Super{Parent != nullptr ? Parent() : nullptr};
    auto Owner{std::make_unique<UClass>(Name, NativeType->mSize, Super, Factory, Flags)};
    UClass* Class{Owner.get()};

    GetRegisteredTypes().emplace(std::move(Name), FRegisteredType{Definition, std::move(Owner)});

    if (Builder != nullptr) {
        Builder(Class);
    }

    Class->Seal();

    return Class;
}

/**
 * @brief 같은 이름의 정의와 호환성을 검사한 뒤 구조체 메타데이터를 등록합니다.
 */
UScriptStruct* FReflectionRegistry::DefineStruct(FString Name, const FNativeType* NativeType, FStructBuilder Builder, const UScriptStruct* Parent) {
    ErrorHandler::Report(Name.empty() || NativeType == nullptr, "FReflectionRegistry", "Invalid struct definition", ErrorHandler::EErrorLevel::Critical);
    FReflectionDefinition Definition{EReflectionType::Struct, NativeType};
    Definition.mStructBuilder = Builder;
    Definition.mStructParent = Parent;

    if (UStruct* Existing{FindDefinition(Name, Definition)}) {
        return static_cast<UScriptStruct*>(Existing);
    }

    auto Owner{std::make_unique<UScriptStruct>(Name, NativeType->mSize, Parent)};
    UScriptStruct* Struct{Owner.get()};

    GetRegisteredTypes().emplace(std::move(Name), FRegisteredType{Definition, std::move(Owner)});

    if (Builder != nullptr) {
        Builder(Struct);
    }

    Struct->Seal();

    return Struct;
}

/**
 * @brief 같은 이름의 정의와 호환성을 검사한 뒤 열거형 메타데이터를 등록합니다.
 */
UEnum* FReflectionRegistry::DefineEnum(FString Name, const FNativeType* NativeType, FEnumBuilder Builder) {
    ErrorHandler::Report(Name.empty() || NativeType == nullptr, "FReflectionRegistry", "Invalid enum definition", ErrorHandler::EErrorLevel::Critical);
    FReflectionDefinition Definition{EReflectionType::Enum, NativeType};
    Definition.mEnumBuilder = Builder;

    if (UStruct* Existing{FindDefinition(Name, Definition)}) {
        return static_cast<UEnum*>(Existing);
    }

    auto Owner{std::make_unique<UEnum>(Name, NativeType->mSize)};
    UEnum* Enum{Owner.get()};

    GetRegisteredTypes().emplace(std::move(Name), FRegisteredType{Definition, std::move(Owner)});

    if (Builder != nullptr) {
        Builder(Enum);
    }

    Enum->Seal();

    return Enum;
}

/**
 * @brief 열거형 빌더에서 전달한 열거값 목록을 등록합니다.
 */
void FReflectionRegistry::AddEnumValues(UEnum* Information, std::initializer_list<FEnumValue> Values) {
    Information->AddValues(Values);
}

/**
 * @brief 등록된 클래스 메타데이터를 이름으로 조회합니다.
 */
UClass* FReflectionRegistry::FindClass(const FString& Name) {
    return static_cast<UClass*>(FindType(Name, EReflectionType::Class));
}

/**
 * @brief 등록된 구조체 메타데이터를 이름으로 조회합니다.
 */
UScriptStruct* FReflectionRegistry::FindStruct(const FString& Name) {
    return static_cast<UScriptStruct*>(FindType(Name, EReflectionType::Struct));
}

/**
 * @brief 등록된 열거형 메타데이터를 이름으로 조회합니다.
 */
UEnum* FReflectionRegistry::FindEnum(const FString& Name) {
    return static_cast<UEnum*>(FindType(Name, EReflectionType::Enum));
}
