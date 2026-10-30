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

#include <algorithm>

namespace CoreUObject {

    /**
     * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 프로세스 수명 동안 유지하는 네이티브 타입 등록 맵을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        Core::TMap<Core::FString, CoreUObject::FRegisteredType>& GetRegisteredTypes() {
            static Core::TMap<Core::FString, CoreUObject::FRegisteredType> Types{};
            return Types;
        }

        /**
         * @brief 이미 등록된 정의의 일치 여부를 검사하고 대응 메타데이터를 반환합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Definition 기존 등록과 비교할 타입 정의입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        CoreUObject::UStruct* FindDefinition(const Core::FString& Name, const CoreUObject::FReflectionDefinition& Definition) {
            const auto Position{GetRegisteredTypes().find(Name)};

            if (Position == GetRegisteredTypes().end()) {
                return nullptr;
            }

            if (!(Position->second.mDefinition == Definition)) {
                Core::ErrorHandler::Fatal("FReflectionRegistry", "Conflicting reflected type: " + Name);
            }

            return Position->second.mInformation.get();
        }

        /**
         * @brief 이름과 타입 종류가 일치하는 등록 메타데이터를 조회합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Kind 조회할 메타데이터 종류입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        CoreUObject::UStruct* FindType(const Core::FString& Name, CoreUObject::EReflectionType Kind) {
            const auto Position{GetRegisteredTypes().find(Name)};
            return Position != GetRegisteredTypes().end() && Position->second.mDefinition.mKind == Kind ? Position->second.mInformation.get() : nullptr;
        }
    }

    /**
     * @brief 같은 이름의 정의와 호환성을 검사한 뒤 클래스 메타데이터를 등록합니다.
     */
    CoreUObject::UClass* CoreUObject::FReflectionRegistry::DefineClass(Core::FString Name, CoreUObject::UClass* (*Parent)(), const CoreUObject::FNativeType* NativeType, CoreUObject::EClassFlags Flags, FFactory Factory, FClassBuilder Builder) {
        Core::ErrorHandler::Report(Name.empty() || NativeType == nullptr, "FReflectionRegistry", "Invalid class definition", Core::ErrorHandler::EErrorLevel::Critical);

        const CoreUObject::FReflectionDefinition Definition{CoreUObject::EReflectionType::Class, NativeType, Parent, Flags, Factory, Builder};

        if (CoreUObject::UStruct* Existing{FindDefinition(Name, Definition)}) {
            return static_cast<CoreUObject::UClass*>(Existing);
        }

        CoreUObject::UClass* Super{Parent != nullptr ? Parent() : nullptr};
        auto Owner{std::make_unique<CoreUObject::UClass>(Name, NativeType->mSize, Super, Factory, Flags)};
        CoreUObject::UClass* Class{Owner.get()};

        GetRegisteredTypes().emplace(std::move(Name), CoreUObject::FRegisteredType{Definition, std::move(Owner)});

        if (Builder != nullptr) {
            Builder(Class);
        }

        Class->Seal();

        return Class;
    }

    /**
     * @brief 같은 이름의 정의와 호환성을 검사한 뒤 구조체 메타데이터를 등록합니다.
     */
    CoreUObject::UScriptStruct* CoreUObject::FReflectionRegistry::DefineStruct(Core::FString Name, const CoreUObject::FNativeType* NativeType, FStructBuilder Builder, const CoreUObject::UScriptStruct* Parent) {
        Core::ErrorHandler::Report(Name.empty() || NativeType == nullptr, "FReflectionRegistry", "Invalid struct definition", Core::ErrorHandler::EErrorLevel::Critical);

        CoreUObject::FReflectionDefinition Definition{CoreUObject::EReflectionType::Struct, NativeType};
        Definition.mStructBuilder = Builder;
        Definition.mStructParent = Parent;

        if (CoreUObject::UStruct* Existing{FindDefinition(Name, Definition)}) {
            return static_cast<CoreUObject::UScriptStruct*>(Existing);
        }

        auto Owner{std::make_unique<CoreUObject::UScriptStruct>(Name, NativeType->mSize, Parent)};
        CoreUObject::UScriptStruct* Struct{Owner.get()};

        GetRegisteredTypes().emplace(std::move(Name), CoreUObject::FRegisteredType{Definition, std::move(Owner)});

        if (Builder != nullptr) {
            Builder(Struct);
        }

        Struct->Seal();

        return Struct;
    }

    /**
     * @brief 같은 이름의 정의와 호환성을 검사한 뒤 열거형 메타데이터를 등록합니다.
     */
    CoreUObject::UEnum* CoreUObject::FReflectionRegistry::DefineEnum(Core::FString Name, const CoreUObject::FNativeType* NativeType, FEnumBuilder Builder) {
        Core::ErrorHandler::Report(Name.empty() || NativeType == nullptr, "FReflectionRegistry", "Invalid enum definition", Core::ErrorHandler::EErrorLevel::Critical);

        CoreUObject::FReflectionDefinition Definition{CoreUObject::EReflectionType::Enum, NativeType};
        Definition.mEnumBuilder = Builder;

        if (CoreUObject::UStruct* Existing{FindDefinition(Name, Definition)}) {
            return static_cast<CoreUObject::UEnum*>(Existing);
        }

        auto Owner{std::make_unique<CoreUObject::UEnum>(Name, NativeType->mSize)};
        CoreUObject::UEnum* Enum{Owner.get()};

        GetRegisteredTypes().emplace(std::move(Name), CoreUObject::FRegisteredType{Definition, std::move(Owner)});

        if (Builder != nullptr) {
            Builder(Enum);
        }

        Enum->Seal();

        return Enum;
    }

    /**
     * @brief 열거형 빌더에서 전달한 열거값 목록을 등록합니다.
     */
    void CoreUObject::FReflectionRegistry::AddEnumValues(CoreUObject::UEnum* Information, std::initializer_list<CoreUObject::FEnumValue> Values) {
        Information->AddValues(Values);
    }

    /**
     * @brief 등록된 클래스 메타데이터를 이름으로 조회합니다.
     */
    CoreUObject::UClass* CoreUObject::FReflectionRegistry::FindClass(const Core::FString& Name) {
        return static_cast<CoreUObject::UClass*>(FindType(Name, CoreUObject::EReflectionType::Class));
    }

    Core::TArray<const CoreUObject::UClass*> CoreUObject::FReflectionRegistry::GetDerivedClasses(const CoreUObject::UClass* Parent) {
        Core::TArray<const CoreUObject::UClass*> Classes{};

        if (Parent == nullptr) {
            return Classes;
        }

        for (const auto& Entry : GetRegisteredTypes()) {
            if (Entry.second.mDefinition.mKind != CoreUObject::EReflectionType::Class) {
                continue;
            }

            const auto* Class{static_cast<const CoreUObject::UClass*>(Entry.second.mInformation.get())};

            if (Class != Parent && Class->IsChildOf(Parent)) {
                Classes.push_back(Class);
            }
        }

        std::sort(Classes.begin(), Classes.end(), [](const CoreUObject::UClass* First, const CoreUObject::UClass* Second) {
            return First->GetName() < Second->GetName();
        });

        return Classes;
    }

    /**
     * @brief 등록된 구조체 메타데이터를 이름으로 조회합니다.
     */
    CoreUObject::UScriptStruct* CoreUObject::FReflectionRegistry::FindStruct(const Core::FString& Name) {
        return static_cast<CoreUObject::UScriptStruct*>(FindType(Name, CoreUObject::EReflectionType::Struct));
    }

    /**
     * @brief 등록된 열거형 메타데이터를 이름으로 조회합니다.
     */
    CoreUObject::UEnum* CoreUObject::FReflectionRegistry::FindEnum(const Core::FString& Name) {
        return static_cast<CoreUObject::UEnum*>(FindType(Name, CoreUObject::EReflectionType::Enum));
    }

}
