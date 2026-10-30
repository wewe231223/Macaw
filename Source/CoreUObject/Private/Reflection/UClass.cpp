/**
 * @file UClass.cpp
 * @brief UObject 상속 관계, 팩터리와 상속된 프로퍼티를 기술하는 메타데이터입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/UClass.h"
#include "Core/Archive.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <utility>

namespace CoreUObject {

    /**
     * @brief UClass의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    CoreUObject::UClass::UClass(Core::FString Name, std::size_t Size, const CoreUObject::UClass* Parent, FFactory Factory, CoreUObject::EClassFlags Flags)
		: CoreUObject::UStruct{std::move(Name), Size},
		  mParent{Parent},
		  mFactory{Factory},
		  mFlags{Flags} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    CoreUObject::UClass::~UClass() = default;

    /**
     * @brief 부모 클래스 메타데이터를 반환합니다.
     */
    const CoreUObject::UClass* CoreUObject::UClass::GetSuperClass() const {
        return mParent;
    }

    /**
     * @brief 자기 자신을 포함한 상속 체인에 지정한 부모 타입이 있는지 검사합니다.
     */
    bool CoreUObject::UClass::IsChildOf(const CoreUObject::UClass* Parent) const {
        for (const CoreUObject::UClass* Current{this}; Current != nullptr; Current = Current->mParent) {
            if (Current == Parent) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief 요청한 클래스 플래그 중 하나라도 설정되어 있는지 검사합니다.
     */
    bool CoreUObject::UClass::HasAnyFlags(CoreUObject::EClassFlags Flags) const {
        return (static_cast<Core::Uint32>(mFlags) & static_cast<Core::Uint32>(Flags)) != 0;
    }

    /**
     * @brief 클래스 정책과 팩터리가 인스턴스 생성을 허용하는지 검사합니다.
     */
    bool CoreUObject::UClass::CanCreate() const {
        return mFactory != nullptr;
    }

    /**
     * @brief 생성 가능한 클래스의 등록된 팩터리를 호출합니다.
     */
    std::unique_ptr<CoreUObject::UObject> CoreUObject::UClass::CreateInstance() const {
        return mFactory != nullptr ? mFactory() : nullptr;
    }

    /**
     * @brief 프로퍼티 등록을 마무리하고 조회용 목록을 확정합니다.
     */
    void CoreUObject::UClass::Seal() {
        if (mClassSealed) {
            return;
        }

        CoreUObject::UStruct::Seal();

        if (mParent != nullptr) {
            mAllProperties = mParent->GetAllProperties();
        }

        for (const CoreUObject::FProperty* Property : GetProperties()) {
            if (std::any_of(mAllProperties.begin(), mAllProperties.end(), [Property](const CoreUObject::FProperty* Existing) {
                return Existing->GetName() == Property->GetName();
            })) {
                Core::ErrorHandler::Fatal("Reflection", "Duplicate inherited property: " + Property->GetName());
            }

            mAllProperties.push_back(Property);
        }

        mClassSealed = true;
    }

    /**
     * @brief 상속된 항목을 포함한 프로퍼티 주소 목록을 반환합니다.
     */
    const Core::TArray<const CoreUObject::FProperty*>& CoreUObject::UClass::GetAllProperties() const {
        return mAllProperties;
    }

    /**
     * @brief 상속된 프로퍼티를 포함하여 이름으로 프로퍼티를 조회합니다.
     */
    const CoreUObject::FProperty* CoreUObject::UClass::FindClassProperty(const Core::FString& Name) const {
        const auto Position{std::find_if(mAllProperties.begin(), mAllProperties.end(), [&Name](const CoreUObject::FProperty* Property) {
            return Property->GetName() == Name;
        })};

        return Position != mAllProperties.end() ? *Position : nullptr;
    }

    /**
     * @brief 상속된 프로퍼티를 포함하여 클래스 값을 직렬화합니다.
     */
    void CoreUObject::UClass::SerializeClassProperties(void* Container, Core::FArchive& Archive, const CoreUObject::FProperty* Excluded) const {
        for (const CoreUObject::FProperty* Property : mAllProperties) {
            if (Property != Excluded) {
                Property->SerializeValue(Container, Archive);
            }
        }
    }

}
