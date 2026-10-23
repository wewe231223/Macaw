/**
 * @file FProperty.h
 * @brief 리플렉션 필드의 읽기·쓰기, 직렬화, 참조 방문과 에디터 메타데이터를 제공합니다.
 */
#pragma once

#include "CoreUObject/Reflection/FPropertyValue.h"
#include <limits>

class FArchive;
class UStruct;
class UEnum;

/**
 * @brief 프로퍼티의 편집, 직렬화와 참조 의미를 나타내는 비트 플래그입니다.
 */
enum class EPropertyFlags : Uint32 {
    /**
     * @brief 별도 프로퍼티 속성이 없는 기본값입니다.
     */
    None = 0,
    /**
     * @brief 에디터 편집 대상임을 나타냅니다.
     */
    Edit = 1,
    /**
     * @brief 구조체 프로퍼티 직렬화의 대상임을 나타냅니다.
     */
    Serialize = 2,
    /**
     * @brief 프로퍼티 쓰기를 허용하지 않습니다.
     */
    ReadOnly = 4,
    /**
     * @brief Serialize가 설정되어 있어도 구조체 직렬화에서 제외합니다.
     */
    Transient = 8,
    /**
     * @brief 리플렉션 함수의 인수 프로퍼티입니다.
     */
    Parameter = 16,
    /**
     * @brief 리플렉션 함수의 반환 값 프로퍼티입니다.
     */
    Return = 32,
    /**
     * @brief 객체 참조를 표현하는 프로퍼티임을 표시합니다.
     */
    Reference = 64,
    /**
     * @brief 소유 객체 또는 소유 컬렉션을 표현하는 프로퍼티임을 표시합니다.
     */
    Owned = 128,
    /**
     * @brief 벡터 값을 에디터에서 색상으로 편집하도록 표시합니다.
     */
    Color = 256,
    /**
     * @brief 에셋 선택을 사용하는 프로퍼티임을 표시합니다.
     */
    Asset = 512,
    /**
     * @brief 구조체 멤버를 별도 이름 범위 없이 현재 범위에 직렬화하도록 표시합니다.
     */
    Inline = 1024,
    /**
     * @brief 에디터 참조 선택 후보를 소유 액터의 컴포넌트로 제한하도록 표시합니다.
     */
    OwnerOnly = 2048
};

/**
 * @brief 프로퍼티 플래그를 비트 OR로 결합합니다.
 * @param Left 첫 번째 플래그입니다.
 * @param Right 두 번째 플래그입니다.
 * @return 결합한 플래그입니다.
 */
EPropertyFlags operator|(EPropertyFlags Left, EPropertyFlags Right);

/**
 * @brief 선언 이름과 소유 구조체를 갖는 경량 필드 메타데이터의 기반 클래스입니다.
 */
class FField {
public:
    /**
     * @brief 필드 이름과 소유 구조체를 설정합니다.
     * @param Name 필드 이름입니다.
     * @param Owner 필드의 비소유 구조체 포인터입니다.
     */
    FField(FName Name, UStruct* Owner);
    /**
     * @brief 파생 필드를 기반 포인터로 해제할 수 있게 합니다.
     */
    virtual ~FField() = default;

public:
    /**
     * @brief 선언된 필드 이름을 반환합니다.
     * @return 필드 이름입니다.
     */
    FName GetName() const;
    /**
     * @brief 필드를 선언한 구조체를 반환합니다.
     * @return 비소유 메타데이터 포인터입니다.
     */
    UStruct* GetOwner() const;

private:
    /**
     * @brief 필드의 선언 이름입니다.
     */
    FName mFieldName{};
    /**
     * @brief 필드를 선언한 구조체의 비소유 포인터입니다.
     */
    UStruct* mOwner{};
};

/**
 * @brief 네이티브 값의 타입 정보와 읽기·쓰기 콜백을 연결하는 프로퍼티 메타데이터입니다.
 * @note 에디터 범위와 선택 필터는 메타데이터이며 WriteContainer가 자동으로 적용하지 않습니다.
 */
class FProperty : public FField {
public:
    /**
     * @brief 소유 저장 공간에서 프로퍼티 값의 복사본을 읽는 함수 타입입니다.
     */
    using FReader = FPropertyValue (*)(const void*);
    /**
     * @brief 소유 저장 공간에 값을 쓰고 성공 여부를 반환하는 함수 타입입니다.
     */
    using FWriter = bool (*)(void*, const FPropertyValue&);
    /**
     * @brief 저장 공간과 프로퍼티 메타데이터로 직렬화를 수행하는 함수 타입입니다.
     */
    using FSerializer = void (*)(void*, FArchive&, const FProperty&);
    /**
     * @brief 객체별 동적 수치 범위를 읽는 함수 타입입니다.
     */
    using FBoundGetter = double (*)(const void*);
    /**
     * @brief 소유 저장 공간과 객체 후보로 선택 허용 여부를 판단하는 함수 타입입니다.
     */
    using FObjectFilter = bool (*)(const void*, UObject*);
    /**
     * @brief 소유 저장 공간으로 편집 허용 조건을 계산하는 함수 타입입니다.
     */
    using FEditCondition = bool (*)(const void*);
    /**
     * @brief 객체별 이름 선택 목록을 반환하는 함수 타입입니다.
     */
    using FOptionsGetter = TArray<FName> (*)(const void*);
    /**
     * @brief 저장 공간의 객체 참조를 방문 콜백으로 전달하는 함수 타입입니다.
     */
    using FReferenceVisitor = void (*)(const void*, const std::function<void(UObject*)>&);

public:
    /**
     * @brief 타입, 정책 플래그와 프로퍼티 연산 콜백을 설정합니다.
     * @param Name 프로퍼티 이름입니다. 표시 이름과 직렬화 이름의 초기값으로도 사용합니다.
     * @param Owner 프로퍼티를 선언한 구조체입니다.
     * @param Type 정확한 네이티브 값 타입입니다.
     * @param Flags 프로퍼티 정책 플래그입니다.
     * @param Reader 값 읽기 함수입니다.
     * @param Writer 값 쓰기 함수입니다.
     * @param Serializer 직렬화 함수입니다.
     * @param ReferenceVisitor 객체 참조 방문 함수입니다.
     */
    FProperty(FName Name, UStruct* Owner, const FNativeType* Type, EPropertyFlags Flags, FReader Reader = nullptr, FWriter Writer = nullptr, FSerializer Serializer = nullptr, FReferenceVisitor ReferenceVisitor = nullptr);
    /**
     * @brief 프로퍼티 메타데이터를 해제합니다.
     */
    ~FProperty() override = default;

public:
    /**
     * @brief 정확한 네이티브 값 타입 정보를 반환합니다.
     * @return 등록된 타입 포인터입니다.
     */
    const FNativeType* GetValueType() const;
    /**
     * @brief 프로퍼티 값의 종류를 반환합니다.
     * @return 등록된 종류입니다. 타입 정보가 없으면 Unknown입니다.
     */
    EPropertyKind GetKind() const;
    /**
     * @brief 네이티브 타입에 연결된 구조체 메타데이터를 반환합니다.
     * @return 구조체 정보입니다. 지원하지 않으면 nullptr입니다.
     */
    UScriptStruct* GetStruct() const;
    /**
     * @brief 네이티브 타입에 대응하는 등록 열거형을 찾습니다.
     * @return 열거형 메타데이터입니다. 없으면 nullptr입니다.
     */
    UEnum* GetEnum() const;
    /**
     * @brief 객체 참조 타입에 연결된 클래스 정보를 반환합니다.
     * @return 객체 클래스입니다. 지원하지 않으면 nullptr입니다.
     */
    UClass* GetObjectClass() const;
    /**
     * @brief 컨테이너 원소의 타입 정보를 반환합니다.
     * @return 원소 타입입니다. 해당 정보가 없으면 nullptr입니다.
     */
    const FNativeType* GetElementType() const;
    /**
     * @brief 지정한 플래그 중 하나라도 설정되어 있는지 검사합니다.
     * @param Flags 검사할 비트 마스크입니다.
     * @return 겹치는 비트가 있으면 true입니다.
     */
    bool HasAnyFlags(EPropertyFlags Flags) const;
    /**
     * @brief 쓰기 함수와 읽기 전용 정책을 검사합니다.
     * @return 쓰기 함수가 있고 ReadOnly가 아니면 true입니다.
     */
    bool CanWrite() const;
    /**
     * @brief 객체의 클래스 상속 관계를 검사한 뒤 프로퍼티 값을 읽습니다.
     * @param Object 읽을 객체입니다.
     * @return 읽은 값의 복사본입니다. 소유 타입 불일치 또는 읽기 불가이면 빈 값입니다.
     */
    FPropertyValue GetValue(const UObject& Object) const;
    /**
     * @brief 객체 타입과 정확한 값 타입을 확인하고 프로퍼티에 씁니다.
     * @param Object 변경할 객체입니다.
     * @param Value 적용할 값입니다.
     * @return 클래스 검사와 WriteContainer가 성공하면 true입니다.
     */
    bool SetValue(UObject& Object, const FPropertyValue& Value) const;
    /**
     * @brief 직접 저장 공간에서 읽기 콜백으로 값의 복사본을 얻습니다.
     * @param Container 소유 타입의 저장 공간입니다. nullptr를 허용합니다.
     * @return 읽은 값입니다. null 저장 공간이나 읽기 함수 부재이면 빈 값입니다.
     * @note null이 아닌 저장 공간의 실제 타입은 검사하지 않습니다.
     */
    FPropertyValue ReadContainer(const void* Container) const;
    /**
     * @brief 저장 공간, 쓰기 정책과 정확한 타입을 확인하고 쓰기 콜백을 호출합니다.
     * @param Container 소유 타입의 저장 공간입니다.
     * @param Value 적용할 값입니다.
     * @return 조건 검사와 쓰기 콜백이 성공하면 true입니다.
     */
    bool WriteContainer(void* Container, const FPropertyValue& Value) const;
    /**
     * @brief 직렬화 콜백을 교체합니다.
     * @param Serializer 적용할 함수입니다. nullptr이면 직렬화 콜백을 해제합니다.
     */
    void SetSerializer(FSerializer Serializer);
    /**
     * @brief 저장 공간과 직렬화 콜백이 있으면 직렬화를 수행합니다.
     * @param Container 소유 타입의 저장 공간입니다.
     * @param Archive 사용할 아카이브입니다.
     * @note 이 함수 자체는 Serialize와 Transient 플래그를 검사하지 않습니다.
     */
    void SerializeContainer(void* Container, FArchive& Archive) const;
    /**
     * @brief 저장 공간과 참조 방문 콜백이 있으면 객체 참조를 방문합니다.
     * @param Container 소유 타입의 저장 공간입니다.
     * @param Visitor 발견한 객체를 처리할 콜백입니다.
     */
    void VisitReferences(const void* Container, const std::function<void(UObject*)>& Visitor) const;
    /**
     * @brief 대상의 실제 클래스 기본 객체에서 값을 읽어 대상 프로퍼티에 씁니다.
     * @param Object 복원할 객체입니다. 클래스 정보가 있어야 합니다.
     * @return 기본 객체 조회와 값 쓰기가 성공하면 true입니다.
     */
    bool ResetToDefault(UObject& Object) const;
    /**
     * @brief 에디터 표시와 수치 편집 메타데이터를 설정합니다.
     * @param Category 프로퍼티 분류 이름입니다.
     * @param DisplayName 표시 이름입니다.
     * @param Minimum 정적 수치 하한입니다.
     * @param Maximum 정적 수치 상한입니다.
     * @param Speed 수치 편집 속도입니다.
     */
    void SetEditorMetadata(FString Category, FString DisplayName, double Minimum, double Maximum, float Speed);
    /**
     * @brief 에디터 분류 이름을 반환합니다.
     * @return 내부 문자열의 상수 참조입니다.
     */
    const FString& GetCategory() const;
    /**
     * @brief 에디터 표시 이름을 반환합니다.
     * @return 내부 문자열의 상수 참조입니다.
     */
    const FString& GetDisplayName() const;
    /**
     * @brief 객체별 동적 수치 범위 콜백을 설정합니다.
     * @param Minimum 동적 하한 함수입니다. nullptr이면 정적 하한을 사용합니다.
     * @param Maximum 동적 상한 함수입니다. nullptr이면 정적 상한을 사용합니다.
     */
    void SetDynamicBounds(FBoundGetter Minimum, FBoundGetter Maximum);
    /**
     * @brief 객체별 편집 허용 조건을 설정합니다.
     * @param Condition 조건 함수입니다. nullptr이면 추가 조건이 없습니다.
     */
    void SetEditCondition(FEditCondition Condition);
    /**
     * @brief 편집 플래그, 쓰기 가능 여부와 객체별 조건을 검사합니다.
     * @param Container 조건을 계산할 소유 저장 공간입니다.
     * @return Edit, 쓰기 가능, 유효한 저장 공간과 편집 조건이 모두 충족되면 true입니다.
     */
    bool CanEdit(const void* Container) const;
    /**
     * @brief 이름 선택 목록 콜백을 설정합니다.
     * @param Getter 목록 조회 함수입니다.
     */
    void SetOptionsGetter(FOptionsGetter Getter);
    /**
     * @brief 이름 선택 목록 콜백의 유무를 반환합니다.
     * @return 콜백이 설정되어 있으면 true입니다.
     */
    bool HasOptions() const;
    /**
     * @brief 소유 저장 공간에 대한 이름 선택 목록을 조회합니다.
     * @param Container 목록 조회에 사용할 저장 공간입니다.
     * @return 선택 목록입니다. 저장 공간이나 콜백이 없으면 빈 배열입니다.
     */
    TArray<FName> GetOptions(const void* Container) const;
    /**
     * @brief 에셋 선택기에 사용하는 클래스 메타데이터를 설정합니다.
     * @param AssetClass 선택할 에셋 클래스입니다.
     */
    void SetAssetClass(UClass* AssetClass);
    /**
     * @brief 에셋 선택기에 설정한 클래스를 반환합니다.
     * @return 비소유 에셋 클래스 포인터입니다.
     */
    UClass* GetAssetClass() const;
    /**
     * @brief 값을 읽은 뒤 네이티브 타입의 객체 읽기 어댑터로 해석합니다.
     * @param Container 소유 타입의 저장 공간입니다.
     * @return 해석한 객체입니다. 빈 값 또는 객체 읽기 미지원이면 nullptr입니다.
     */
    UObject* ReadObject(const void* Container) const;
    /**
     * @brief 네이티브 타입의 객체 값 생성 어댑터를 호출합니다.
     * @param Object 값으로 만들 객체입니다. nullptr를 허용합니다.
     * @return 생성한 타입 소거 값입니다. 지원하지 않거나 타입이 호환되지 않으면 빈 값입니다.
     */
    FPropertyValue MakeObjectValue(UObject* Object) const;
    /**
     * @brief 객체 후보의 선택 허용 필터를 설정합니다.
     * @param Filter 필터 함수입니다. nullptr이면 추가 필터가 없습니다.
     */
    void SetObjectFilter(FObjectFilter Filter);
    /**
     * @brief 저장 공간, 객체 클래스 호환성과 추가 필터를 검사합니다.
     * @param Container 필터 계산에 사용할 소유 저장 공간입니다.
     * @param Object 선택할 객체 후보입니다. nullptr는 참조 해제 후보입니다.
     * @return 타입 조건과 필터가 허용하면 true입니다.
     */
    bool CanSelectObject(const void* Container, UObject* Object) const;
    /**
     * @brief 아카이브에 사용할 프로퍼티 이름을 교체합니다.
     * @param Name 직렬화 필드 이름입니다.
     */
    void SetSerializationName(FString Name);
    /**
     * @brief 직렬화 필드 이름을 반환합니다.
     * @return 내부 문자열의 상수 참조입니다.
     */
    const FString& GetSerializationName() const;
    /**
     * @brief 구조체 직렬화에서 프로퍼티를 묶을 범위 이름을 설정합니다.
     * @param Scope 객체 범위 이름입니다. 빈 문자열이면 현재 범위를 사용합니다.
     */
    void SetSerializationScope(FString Scope);
    /**
     * @brief 직렬화 범위 이름을 반환합니다.
     * @return 내부 문자열의 상수 참조입니다.
     */
    const FString& GetSerializationScope() const;
    /**
     * @brief 정적 하한과 객체별 동적 하한으로 유효 편집 하한을 계산합니다.
     * @param Container 동적 범위를 조회할 저장 공간입니다. nullptr이면 정적 하한을 사용합니다.
     * @return 유한한 동적 값과 정적 하한 중 큰 값입니다. 동적 값이 유한하지 않으면 정적 하한입니다.
     */
    double GetMinimum(const void* Container = nullptr) const;
    /**
     * @brief 정적 상한과 객체별 동적 상한으로 유효 편집 상한을 계산합니다.
     * @param Container 동적 범위를 조회할 저장 공간입니다. nullptr이면 정적 상한을 사용합니다.
     * @return 유한한 동적 값과 정적 상한 중 작은 값입니다. 동적 값이 유한하지 않으면 정적 상한입니다.
     */
    double GetMaximum(const void* Container = nullptr) const;
    /**
     * @brief 수치 편집 속도를 반환합니다.
     * @return 설정된 편집 속도입니다.
     */
    float GetSpeed() const;

private:
    /**
     * @brief 정확한 프로퍼티 값 타입 정보입니다.
     */
    const FNativeType* mValueType{};
    /**
     * @brief 프로퍼티 정책 비트입니다.
     */
    EPropertyFlags mFlags{};
    /**
     * @brief 값 읽기 콜백입니다.
     */
    FReader mReader{};
    /**
     * @brief 값 쓰기 콜백입니다.
     */
    FWriter mWriter{};
    /**
     * @brief 직렬화 콜백입니다.
     */
    FSerializer mSerializer{};
    /**
     * @brief 객체 참조 방문 콜백입니다.
     */
    FReferenceVisitor mReferenceVisitor{};
    /**
     * @brief 에디터 분류 이름입니다.
     */
    FString mCategory{};
    /**
     * @brief 에디터 표시 이름입니다.
     */
    FString mDisplayName{};
    /**
     * @brief 정적 수치 편집 하한입니다.
     */
    double mMinimum{-std::numeric_limits<double>::max()};
    /**
     * @brief 정적 수치 편집 상한입니다.
     */
    double mMaximum{std::numeric_limits<double>::max()};
    /**
     * @brief 객체별 동적 하한 콜백입니다.
     */
    FBoundGetter mMinimumGetter{};
    /**
     * @brief 객체별 동적 상한 콜백입니다.
     */
    FBoundGetter mMaximumGetter{};
    /**
     * @brief 객체별 편집 허용 콜백입니다.
     */
    FEditCondition mEditCondition{};
    /**
     * @brief 객체 후보 선택 필터입니다.
     */
    FObjectFilter mObjectFilter{};
    /**
     * @brief 이름 선택 목록 콜백입니다.
     */
    FOptionsGetter mOptionsGetter{};
    /**
     * @brief 에셋 선택 클래스의 비소유 포인터입니다.
     */
    UClass* mAssetClass{};
    /**
     * @brief 직렬화 필드 이름입니다.
     */
    FString mSerializationName{};
    /**
     * @brief 직렬화 객체 범위 이름입니다.
     */
    FString mSerializationScope{};
    /**
     * @brief 수치 편집 속도입니다.
     */
    float mSpeed{0.1f};
};
