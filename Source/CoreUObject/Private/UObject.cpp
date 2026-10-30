/**
 * @file UObject.cpp
 * @brief GUID, 세대 핸들, 리플렉션과 직렬화를 제공하는 객체의 기반 클래스입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/UObject.h"
#include "Core/Archive.h"
#include "CoreUObject/FObjectHandle.h"
#include "CoreUObject/FPropertyChangedEvent.h"
#include "CoreUObject/FObjectRegistry.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Memory.h"
#include "Core/Base/ErrorHandler.h"

namespace CoreUObject {

    /**
     * @brief UObject의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    CoreUObject::UObject::UObject() {
        Core::FMemory::ObjectCreated();

        if (!CoreUObject::FObjectRegistry::Get()->Register(this)) {
            Core::ErrorHandler::Fatal("Object registry", "Object registration failed");
        }
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    CoreUObject::UObject::~UObject() {
        CoreUObject::FObjectRegistry::Get()->Unregister(this);
        Core::FMemory::ObjectDestroyed();
    }

    /**
     * @brief 리플렉션 상속 관계를 기준으로 객체가 요청한 타입인지 검사합니다.
     */
    bool CoreUObject::UObject::IsA(const CoreUObject::UClass* Class) const {
        return GetClass()->IsChildOf(Class);
    }

    /**
     * @brief 객체의 영속 식별자를 반환합니다.
     */
    const Core::FGuid& CoreUObject::UObject::GetGuid() const {
        return mGuid;
    }

    /**
     * @brief 레지스트리 슬롯과 세대 핸들을 반환합니다.
     */
    CoreUObject::FObjectHandle CoreUObject::UObject::GetHandle() const {
        return mHandle;
    }

    /**
     * @brief 객체 이름을 반환합니다.
     */
    const Core::FString& CoreUObject::UObject::GetName() const {
        return mName.ToString();
    }

    /**
     * @brief 비어 있지 않은 이름을 검증하여 객체 이름을 변경합니다.
     */
    bool CoreUObject::UObject::SetName(const Core::FString& Name) {
        if (Name.empty() || Name.size() > 255 || Name.find('\0') != Core::FString::npos) {
            return false;
        }

        mName = Core::FName{Name};

        return true;
    }

    /**
     * @brief 유효한 장면 GUID로 객체의 영속 식별자를 복원합니다.
     */
    bool CoreUObject::UObject::RestoreGuid(const Core::FGuid& Guid) {
        if (!Guid.IsValid()) {
            return false;
        }

        mGuid = Guid;

        return true;
    }

    const Core::FName& CoreUObject::UObject::GetFName() const {
        return mName;
    }

    CoreUObject::UObject* CoreUObject::UObject::GetOuter() const {
        return CoreUObject::FObjectRegistry::Get()->Resolve(mOuter);
    }

    bool CoreUObject::UObject::SetOuter(CoreUObject::UObject* Outer) {
        for (const CoreUObject::UObject* Ancestor{Outer}; Ancestor != nullptr; Ancestor = Ancestor->GetOuter()) {
            if (Ancestor == this) {
                return false;
            }
        }

        mOuter = Outer != nullptr ? Outer->GetHandle() : CoreUObject::FObjectHandle{};

        return true;
    }

    Core::FString CoreUObject::UObject::GetPathName() const {
        Core::FString Path{GetName()};

        for (const CoreUObject::UObject* Ancestor{GetOuter()}; Ancestor != nullptr; Ancestor = Ancestor->GetOuter()) {
            Path = Ancestor->GetName() + "." + Path;
        }

        return Path;
    }

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
    void CoreUObject::UObject::Serialize(Core::FArchive& Archive) {
        if (!Archive.IsLoading() || Archive.HasField("Id")) {
            Core::FGuid Guid{mGuid};
            Archive.SerializeObjectGuid("Id", Guid);

            if (Archive.IsLoading() && !Archive.HasError() && !RestoreGuid(Guid)) {
                Archive.SetError("Invalid object identity");
            }
        }

        GetClass()->SerializeClassProperties(this, Archive, CoreUObject::UObject::StaticClass()->FindClassProperty("Id"));
    }

    /**
     * @brief 복제 그래프 생성 후 사용자 참조를 복제본으로 재연결하는 확장 지점입니다.
     */
    bool CoreUObject::UObject::PostDuplicate(const CoreUObject::UObject* Source, const CoreUObject::FObjectDuplicationContext* Context) {
        return Source != nullptr && Source->GetClass() == GetClass();
    }

    /**
     * @brief 프로퍼티 값을 편집하기 직전에 호출되는 확장 지점입니다.
     */
    void CoreUObject::UObject::PreEditChange(const CoreUObject::FProperty* Property) {
    }

    /**
     * @brief 프로퍼티 값이 변경된 뒤 호출되는 확장 지점입니다.
     */
    void CoreUObject::UObject::PostEditChangeProperty(const CoreUObject::FPropertyChangedEvent& Event) {
    }

    /**
     * @brief 객체 통계에 포함되는 정렬된 메모리를 할당합니다.
     */
    void* CoreUObject::UObject::operator new(std::size_t Size) {
        return Core::FMemory::Allocate(Size, alignof(std::max_align_t));
    }

    /**
     * @brief 객체 통계에 포함되는 정렬된 메모리를 할당합니다.
     */
    void* CoreUObject::UObject::operator new(std::size_t Size, std::align_val_t Alignment) {
        return Core::FMemory::Allocate(Size, static_cast<std::size_t>(Alignment));
    }

    /**
     * @brief 객체 통계에 포함된 정렬 메모리를 해제합니다.
     */
    void CoreUObject::UObject::operator delete(void* Pointer) noexcept {
        Core::FMemory::Free(Pointer);
    }

    /**
     * @brief 객체 통계에 포함된 정렬 메모리를 해제합니다.
     */
    void CoreUObject::UObject::operator delete(void* Pointer, std::align_val_t Alignment) noexcept {
        Core::FMemory::Free(Pointer);
    }

}
