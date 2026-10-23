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

/**
 * @brief UObject의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UObject::UObject() {
    FMemory::ObjectCreated();
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UObject::~UObject() {
    if (mRegistry != nullptr) {
        mRegistry->Unregister(*this);
    }

    FMemory::ObjectDestroyed();
}

/**
 * @brief 리플렉션 상속 관계를 기준으로 객체가 요청한 타입인지 검사합니다.
 */
bool UObject::IsA(const UClass* Class) const {
    return GetClass()->IsChildOf(Class);
}

/**
 * @brief 객체의 영속 식별자를 반환합니다.
 */
const FGuid& UObject::GetGuid() const {
    return mGuid;
}

/**
 * @brief 레지스트리 슬롯과 세대 핸들을 반환합니다.
 */
FObjectHandle UObject::GetHandle() const {
    return mHandle;
}

/**
 * @brief 이름 또는 표시 문자열을 반환합니다.
 */
const FString& UObject::GetName() const {
    return mName;
}

/**
 * @brief 비어 있지 않은 이름을 검증하여 객체 이름을 변경합니다.
 */
bool UObject::SetName(const FString& Name) {
    if (Name.empty() || Name.size() > 255 || Name.find('\0') != FString::npos) {
        return false;
    }

    mName = Name;

    return true;
}

/**
 * @brief 중복되지 않는 유효 GUID로 객체의 식별자를 복원합니다.
 */
bool UObject::RestoreGuid(const FGuid& Guid) {
    if (!Guid.IsValid()) {
        return false;
    }

    if (mRegistry != nullptr) {
        return mRegistry->RestoreGuid(*this, Guid);
    }

    mGuid = Guid;

    return true;
}

/**
 * @brief 객체를 등록한 레지스트리를 반환합니다.
 */
FObjectRegistry* UObject::GetRegistry() const {
    return mRegistry;
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void UObject::Serialize(FArchive& Archive) {
    GetClass()->SerializeClassProperties(this, Archive);
}

/**
 * @brief 프로퍼티 값을 편집하기 직전에 호출되는 확장 지점입니다.
 */
void UObject::PreEditChange(const FProperty& Property) {
    static_cast<void>(Property);
}

/**
 * @brief 프로퍼티 값이 변경된 뒤 호출되는 확장 지점입니다.
 */
void UObject::PostEditChangeProperty(const FPropertyChangedEvent& Event) {
    static_cast<void>(Event);
}

/**
 * @brief 객체 통계에 포함되는 정렬된 메모리를 할당합니다.
 */
void* UObject::operator new(std::size_t Size) {
    return FMemory::Allocate(Size, alignof(std::max_align_t));
}

/**
 * @brief 객체 통계에 포함되는 정렬된 메모리를 할당합니다.
 */
void* UObject::operator new(std::size_t Size, std::align_val_t Alignment) {
    return FMemory::Allocate(Size, static_cast<std::size_t>(Alignment));
}

/**
 * @brief 객체 통계에 포함된 정렬 메모리를 해제합니다.
 */
void UObject::operator delete(void* Pointer) noexcept {
    FMemory::Free(Pointer);
}

/**
 * @brief 객체 통계에 포함된 정렬 메모리를 해제합니다.
 */
void UObject::operator delete(void* Pointer, std::align_val_t Alignment) noexcept {
    static_cast<void>(Alignment);
    FMemory::Free(Pointer);
}
