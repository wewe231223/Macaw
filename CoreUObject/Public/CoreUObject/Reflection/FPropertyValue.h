/**
 * @file FPropertyValue.h
 * @brief 네이티브 타입 기술자, 객체 참조 방문과 타입 소거 값 소유 컨테이너를 제공합니다.
 */
#pragma once

#include "Core/Common.h"
#include "Core/Base/FName.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FTransform.h"
#include <functional>
#include <memory>
#include <type_traits>

class UObject;
class UClass;
class UScriptStruct;
class FPropertyValue;

/**
 * @brief 에디터와 리플렉션에서 사용하는 네이티브 값의 분류입니다.
 */
enum class EPropertyKind {
    /**
     * @brief 분류 정보가 없는 값입니다.
     */
    Unknown,
    /**
     * @brief 불리언 값입니다.
     */
    Bool,
    /**
     * @brief Uint8 바이트 값입니다.
     */
    Byte,
    /**
     * @brief 정수 값입니다.
     */
    Integer,
    /**
     * @brief 부동소수점 값입니다.
     */
    Float,
    /**
     * @brief FString 문자열 값입니다.
     */
    String,
    /**
     * @brief FName 이름 값입니다.
     */
    Name,
    /**
     * @brief FGuid 식별자 값입니다.
     */
    Guid,
    /**
     * @brief 2차원 벡터 값입니다.
     */
    Vector2,
    /**
     * @brief 3차원 벡터 값입니다.
     */
    Vector3,
    /**
     * @brief 4차원 벡터 값입니다.
     */
    Vector4,
    /**
     * @brief 위치·회전·스케일 변환 값입니다.
     */
    Transform,
    /**
     * @brief 구조체 또는 기타 네이티브 값입니다.
     */
    Struct,
    /**
     * @brief 열거형 값입니다.
     */
    Enum,
    /**
     * @brief 원시 포인터나 Get/get 접근자를 갖는 참조 값입니다.
     */
    Object,
    /**
     * @brief begin/end로 순회 가능한 컨테이너 값입니다.
     */
    Array
};

/**
 * @brief 값의 종류, 저장 규격과 복사·파괴·참조 연산을 제공하는 타입 기술자입니다.
 * @note GetNativeType의 타입별 정적 인스턴스를 사용하며 타입 포인터 일치로 정확한 타입을 구별합니다.
 */
struct FNativeType {
    /**
     * @brief 네이티브 값의 종류입니다.
     */
    EPropertyKind mKind{};
    /**
     * @brief 네이티브 값의 바이트 크기입니다.
     */
    std::size_t mSize{};
    /**
     * @brief 네이티브 값의 바이트 정렬입니다.
     */
    std::size_t mAlignment{};
    /**
     * @brief 값을 새 저장 공간에 복사하는 함수입니다. 지원하지 않으면 nullptr를 반환합니다.
     */
    void* (*mCopy)(const void*){};
    /**
     * @brief 해당 타입으로 할당된 저장 값을 삭제하는 함수입니다.
     */
    void (*mDestroy)(void*){};
    /**
     * @brief 저장 값에서 UObject 참조를 방문하는 함수입니다.
     */
    void (*mVisitReferences)(const void*, const std::function<void(UObject*)>&){};
    /**
     * @brief 연결된 구조체 메타데이터를 조회하는 함수입니다.
     */
    UScriptStruct* (*mGetStruct)(){};
    /**
     * @brief 객체 참조의 클래스 메타데이터를 조회하는 함수입니다.
     */
    UClass* (*mGetObjectClass)(){};
    /**
     * @brief 컨테이너 원소 타입 정보를 조회하는 함수입니다.
     */
    const FNativeType* (*mGetElementType)(){};
    /**
     * @brief 저장 값에서 직접 객체 포인터를 읽는 함수입니다.
     */
    UObject* (*mReadObject)(const void*){};
    /**
     * @brief 호환 객체 포인터로 해당 타입의 소유 값을 구성하는 함수입니다.
     */
    FPropertyValue (*mMakeObjectValue)(UObject*){};
};

/**
 * @brief T의 값 분류와 네이티브 연산을 갖는 정적 타입 기술자를 반환합니다.
 * @tparam T 기술할 네이티브 타입입니다.
 * @return 타입별 정적 기술자 포인터입니다.
 */
template <typename T>
const FNativeType* GetNativeType();

/**
 * @brief T의 복사 생성 가능 여부에 따라 값을 새 저장 공간에 복사합니다.
 * @tparam T 복사할 네이티브 타입입니다.
 * @param Value 유효한 T 값의 저장 공간입니다.
 * @return 복사한 T의 소유 포인터입니다. 지원하는 복사 조건을 만족하지 못하면 nullptr입니다.
 * @note value_type을 갖는 타입은 원소 타입의 복사 생성 가능 여부도 검사합니다.
 */
template <typename T>
void* CopyNativeValue(const void* Value);

/**
 * @brief T로 생성한 저장 값을 삭제합니다.
 * @tparam T 저장 값의 정확한 타입입니다.
 * @param Value CopyNativeValue로 확보한 저장 공간 또는 nullptr입니다.
 */
template <typename T>
void DestroyNativeValue(void* Value);

/**
 * @brief 원시 객체 포인터, 객체 래퍼, 컨테이너와 리플렉션 구조체의 객체 참조를 방문합니다.
 * @tparam T 저장 값의 정확한 타입입니다.
 * @param Value 유효한 T 값의 저장 공간입니다.
 * @param Visitor 발견한 null이 아닌 객체를 처리할 콜백입니다.
 * @note Get 또는 get으로 UObject 포인터를 얻는 래퍼를 지원하고 FString은 원소 순회에서 제외합니다.
 */
template <typename T>
void VisitNativeReferences(const void* Value, const std::function<void(UObject*)>& Visitor);

/**
 * @brief T가 StaticStruct를 제공하면 구조체 정보를 반환합니다.
 * @tparam T 조회할 타입입니다.
 * @return 구조체 정보입니다. 지원하지 않으면 nullptr입니다.
 */
template <typename T>
UScriptStruct* GetNativeStruct();

/**
 * @brief 객체 포인터 또는 Get/get 래퍼의 참조 대상 클래스 정보를 반환합니다.
 * @tparam T 조회할 참조 타입입니다.
 * @return UObject 호환 포인터의 클래스 정보입니다. 지원하지 않으면 nullptr입니다.
 */
template <typename T>
UClass* GetNativeObjectClass();

/**
 * @brief 문자열을 제외한 순회 가능한 컨테이너의 value_type 정보를 반환합니다.
 * @tparam T 조회할 컨테이너 타입입니다.
 * @return 원소 타입 기술자입니다. 지원하지 않으면 nullptr입니다.
 */
template <typename T>
const FNativeType* GetNativeElementType();

/**
 * @brief 객체가 지정한 클래스에 호환되는지 검사합니다.
 * @param Object 검사할 객체입니다. nullptr를 허용합니다.
 * @param Class 비교할 클래스입니다.
 * @return null 객체이거나 IsA 검사를 통과하면 true입니다.
 */
bool IsNativeObjectCompatible(const UObject* Object, const UClass* Class);

/**
 * @brief UObject로 변환 가능한 원시 포인터 값에서 객체를 읽습니다.
 * @tparam T 저장된 원시 포인터 타입입니다.
 * @param Value 유효한 T 값의 저장 공간입니다.
 * @return 저장된 객체 포인터입니다. 원시 UObject 호환 포인터 타입이 아니면 nullptr입니다.
 */
template <typename T>
UObject* ReadNativeObject(const void* Value);

/**
 * @brief 클래스 호환성을 검사해 T 원시 객체 포인터의 소유 값 복사본을 만듭니다.
 * @tparam T UObject로 변환 가능한 원시 포인터 타입입니다.
 * @param Object 저장할 객체입니다. nullptr를 허용합니다.
 * @return 포인터 값을 소유하는 FPropertyValue입니다. 지원하지 않거나 타입이 호환되지 않으면 빈 값입니다.
 * @note 참조된 UObject의 소유권은 인수하지 않습니다.
 */
template <typename T>
FPropertyValue MakeNativeObjectValue(UObject* Object);

/**
 * @brief 구조체 메타데이터로 저장 값의 객체 참조를 방문합니다.
 * @param Struct 사용할 구조체 정보입니다. nullptr이면 방문하지 않습니다.
 * @param Value 구조체 타입의 저장 공간입니다.
 * @param Visitor 발견한 객체를 처리할 콜백입니다.
 */
void VisitScriptStructReferences(const UScriptStruct* Struct, const void* Value, const std::function<void(UObject*)>& Visitor);

/**
 * @brief UClass 포인터 타입 기술자의 외부 명시적 인스턴스화 선언입니다.
 * @return UClass 포인터 값의 정적 네이티브 타입 정보입니다.
 */
extern template const FNativeType* GetNativeType<UClass*>();
/**
 * @brief UClass 포인터 참조 방문 함수의 외부 명시적 인스턴스화 선언입니다.
 * @param Value 유효한 UClass 포인터 값의 저장 공간입니다.
 * @param Visitor null이 아닌 클래스 객체를 처리할 콜백입니다.
 */
extern template void VisitNativeReferences<UClass*>(const void* Value, const std::function<void(UObject*)>& Visitor);

/**
 * @brief 정확한 네이티브 타입 정보와 복사한 값의 저장 공간을 소유하는 타입 소거 컨테이너입니다.
 * @note 타입 변환을 수행하지 않으며 Get은 타입 기술자 포인터가 일치할 때만 값을 반환합니다.
 */
class FPropertyValue {
public:
    /**
     * @brief 타입과 저장 값이 없는 빈 컨테이너를 생성합니다.
     */
    FPropertyValue() = default;
    /**
     * @brief 네이티브 값의 복사본을 소유합니다.
     * @tparam T 저장할 정확한 네이티브 타입입니다.
     * @param Value 복사할 값입니다.
     * @note 복사 어댑터가 값을 복사할 수 없으면 빈 컨테이너가 됩니다.
     */
    template <typename T>
    explicit FPropertyValue(const T& Value);
    /**
     * @brief 타입별 파괴 함수로 저장 값을 삭제합니다.
     */
    ~FPropertyValue();
    /**
     * @brief Other의 저장 값을 타입별 복사 함수로 복제합니다.
     * @param Other 복사할 값 컨테이너입니다.
     */
    FPropertyValue(const FPropertyValue& Other);
    /**
     * @brief 복사본을 만든 뒤 현재 저장 값을 교체합니다.
     * @param Other 복사할 값 컨테이너입니다.
     * @return 현재 컨테이너의 참조입니다.
     */
    FPropertyValue& operator=(const FPropertyValue& Other);
    /**
     * @brief Other의 타입 정보와 저장 공간 소유권을 이동합니다.
     * @param Other 이동 후 비워질 컨테이너입니다.
     */
    FPropertyValue(FPropertyValue&& Other) noexcept;
    /**
     * @brief 현재 값을 해제하고 Other의 저장 공간 소유권을 이동합니다.
     * @param Other 이동 후 비워질 컨테이너입니다.
     * @return 현재 컨테이너의 참조입니다.
     */
    FPropertyValue& operator=(FPropertyValue&& Other) noexcept;

public:
    /**
     * @brief 저장 값이 있고 정확한 타입이 T인지 검사합니다.
     * @tparam T 검사할 네이티브 타입입니다.
     * @return 타입 기술자가 일치하고 저장 값이 있으면 true입니다.
     */
    template <typename T>
    bool Is() const;

    /**
     * @brief 정확한 타입 검사를 거쳐 저장 값의 포인터를 반환합니다.
     * @tparam T 요청할 네이티브 타입입니다.
     * @return 내부 값의 상수 포인터입니다. 빈 값 또는 타입 불일치이면 nullptr입니다.
     */
    template <typename T>
    const T* Get() const;

    /**
     * @brief 저장 값의 네이티브 타입 정보를 반환합니다.
     * @return 타입 기술자입니다. 빈 기본 상태이면 nullptr입니다.
     */
    const FNativeType* GetType() const;
    /**
     * @brief 저장 값의 읽기 전용 주소를 반환합니다.
     * @return 내부 저장 공간입니다. 비어 있으면 nullptr입니다.
     */
    const void* GetData() const;
    /**
     * @brief 저장 값의 수정 가능한 주소를 반환합니다.
     * @return 내부 저장 공간입니다. 정확한 타입으로 접근해야 합니다.
     */
    void* GetMutableData();
    /**
     * @brief 소유한 저장 값의 유무를 검사합니다.
     * @return 저장 주소가 nullptr이면 true입니다.
     */
    bool IsEmpty() const;
    /**
     * @brief 저장 값을 삭제하고 타입과 주소를 비웁니다.
     */
    void Reset();

private:
    /**
     * @brief 저장 값의 정확한 타입과 파괴 연산을 제공하는 정적 기술자입니다.
     */
    const FNativeType* mType{};
    /**
     * @brief 타입별 복사 함수로 확보한 소유 저장 공간입니다.
     */
    void* mData{};
};

template <typename T>
const FNativeType* GetNativeType() {
    static const FNativeType Type{[]() {
        if constexpr (std::is_same_v<T, bool>) {
            return EPropertyKind::Bool;
        } else if constexpr (std::is_same_v<T, Uint8>) {
            return EPropertyKind::Byte;
        } else if constexpr (std::is_integral_v<T>) {
            return EPropertyKind::Integer;
        } else if constexpr (std::is_floating_point_v<T>) {
            return EPropertyKind::Float;
        } else if constexpr (std::is_same_v<T, FString>) {
            return EPropertyKind::String;
        } else if constexpr (std::is_same_v<T, FName>) {
            return EPropertyKind::Name;
        } else if constexpr (std::is_same_v<T, FGuid>) {
            return EPropertyKind::Guid;
        } else if constexpr (std::is_same_v<T, FVector2>) {
            return EPropertyKind::Vector2;
        } else if constexpr (std::is_same_v<T, FVector3>) {
            return EPropertyKind::Vector3;
        } else if constexpr (std::is_same_v<T, FVector4>) {
            return EPropertyKind::Vector4;
        } else if constexpr (std::is_same_v<T, FTransform>) {
            return EPropertyKind::Transform;
        } else if constexpr (std::is_enum_v<T>) {
            return EPropertyKind::Enum;
        } else if constexpr (std::is_pointer_v<T> || requires(const T& Value) { Value.Get(); } || requires(const T& Value) { Value.get(); }) {
            return EPropertyKind::Object;
        } else if constexpr (requires(const T& Value) { Value.begin(); Value.end(); }) {
            return EPropertyKind::Array;
        } else {
            return EPropertyKind::Struct;
        }
    }(), sizeof(T), alignof(T), &CopyNativeValue<T>, &DestroyNativeValue<T>, &VisitNativeReferences<T>, &GetNativeStruct<T>, &GetNativeObjectClass<T>, &GetNativeElementType<T>, &ReadNativeObject<T>, &MakeNativeObjectValue<T>};

    return &Type;
}

template <typename T>
void* CopyNativeValue(const void* Value) {
    if constexpr (std::is_copy_constructible_v<T> && !requires { typename T::value_type; }) {
        return new T{*static_cast<const T*>(Value)};
    } else if constexpr (requires { typename T::value_type; }) {
        if constexpr (std::is_copy_constructible_v<typename T::value_type>) {
            return new T{*static_cast<const T*>(Value)};
        }
    }

    return nullptr;
}

template <typename T>
void DestroyNativeValue(void* Value) {
    delete static_cast<T*>(Value);
}

template <typename T>
void VisitNativeReferences(const void* Value, const std::function<void(UObject*)>& Visitor) {
    const T& NativeValue{*static_cast<const T*>(Value)};

    if constexpr (std::is_pointer_v<T> && std::is_convertible_v<T, UObject*>) {
        if (NativeValue != nullptr) {
            Visitor(NativeValue);
        }
    } else if constexpr (requires { NativeValue.Get(); }) {
        if constexpr (std::is_convertible_v<decltype(NativeValue.Get()), UObject*>) {
            if (UObject* Object{NativeValue.Get()}) {
                Visitor(Object);
            }
        }
    } else if constexpr (requires { NativeValue.get(); }) {
        if constexpr (std::is_convertible_v<decltype(NativeValue.get()), UObject*>) {
            if (UObject* Object{NativeValue.get()}) {
                Visitor(Object);
            }
        }
    } else if constexpr (!std::is_same_v<T, FString> && requires { NativeValue.begin(); NativeValue.end(); }) {
        for (const auto& Element : NativeValue) {
            VisitNativeReferences<std::remove_cvref_t<decltype(Element)>>(&Element, Visitor);
        }
    } else if constexpr (requires { T::StaticStruct(); }) {
        VisitScriptStructReferences(T::StaticStruct(), Value, Visitor);
    }
}

template <typename T>
UScriptStruct* GetNativeStruct() {
    if constexpr (requires { T::StaticStruct(); }) {
        return T::StaticStruct();
    } else {
        return nullptr;
    }
}

template <typename T>
UClass* GetNativeObjectClass() {
    if constexpr (std::is_pointer_v<T> && std::is_convertible_v<T, UObject*>) {
        return std::remove_pointer_t<T>::StaticClass();
    } else if constexpr (requires(const T& Value) { Value.Get(); }) {
        return GetNativeObjectClass<decltype(std::declval<const T&>().Get())>();
    } else if constexpr (requires(const T& Value) { Value.get(); }) {
        return GetNativeObjectClass<decltype(std::declval<const T&>().get())>();
    } else {
        return nullptr;
    }
}

template <typename T>
const FNativeType* GetNativeElementType() {
    if constexpr (!std::is_same_v<T, FString> && requires(const T& Value) { typename T::value_type; Value.begin(); Value.end(); }) {
        return GetNativeType<typename T::value_type>();
    } else {
        return nullptr;
    }
}

template <typename T>
FPropertyValue::FPropertyValue(const T& Value)
	: mType(GetNativeType<T>()),
	  mData(mType->mCopy(&Value)) {
    if (mData == nullptr) {
        mType = nullptr;
    }
}

template <typename T>
bool FPropertyValue::Is() const {
    return mType == GetNativeType<T>() && mData != nullptr;
}

template <typename T>
const T* FPropertyValue::Get() const {
    return Is<T>() ? static_cast<const T*>(mData) : nullptr;
}

template <typename T>
UObject* ReadNativeObject(const void* Value) {
    if constexpr (std::is_pointer_v<T> && std::is_convertible_v<T, UObject*>) {
        return *static_cast<const T*>(Value);
    } else {
        return nullptr;
    }
}

template <typename T>
FPropertyValue MakeNativeObjectValue(UObject* Object) {
    if constexpr (std::is_pointer_v<T> && std::is_convertible_v<T, UObject*>) {
        if (Object != nullptr && !IsNativeObjectCompatible(Object, std::remove_pointer_t<T>::StaticClass())) {
            return {};
        }

        return FPropertyValue{static_cast<T>(Object)};
    } else {
        return {};
    }
}
