/**
 * @file FName.h
 * @brief 이름 풀의 엔트리와 표시 이름, 비교 이름 및 숫자 접미사를 가진 이름 타입을 정의한다.
 */
#pragma once
#include "Core/STL.h"
#include "Core/Common.h"
#include <string_view>

// Max size of name, including the null terminator
/**
 * @brief 이름 풀의 저장 규격 상수를 선언합니다.
 */
enum {
    /** @brief 이름 풀에 저장할 기본 이름의 최대 버퍼 크기. 널 종료 문자를 포함하므로 문자열은 최대 1023바이트이다. */
    NameSize = 1024
};

/**
 * @brief 이름 풀의 엔트리를 가리키는 32비트 식별자.
 * @details 값은 풀의 메모리 배치와 등록 순서에 따라 결정되므로 실행 간 안정성을 보장하지 않는다. 0은 빈 이름에 사용하는 기본 식별자이다.
 */
struct FNameEntryId {
    /** @brief 값이 0인 기본 엔트리 식별자를 생성한다. */
    FNameEntryId();

    /**
     * @brief 엔트리 식별자의 내부 정수 값을 반환한다.
     * @return 현재 이름 풀에서 사용하는 32비트 식별자 값.
     * @note 이 값은 실행 간 영속 식별자로 사용할 수 없다.
     */
    Uint32 ToUnstableInt() const;

    /**
     * @brief 내부 정수 값에서 엔트리 식별자를 구성한다.
     * @param UnstableInt 현재 이름 풀에서 얻은 식별자 값.
     * @return 전달한 값을 보관하는 엔트리 식별자.
     * @details 해당 값이 실제 이름 풀 엔트리를 가리키는지는 검사하지 않는다.
     */
    static FNameEntryId FromUnstableInt(Uint32 UnstableInt);

    // operator
    /**
     * @brief 두 엔트리 식별자의 내부 값이 같은지 비교한다.
     * @param Rhs 비교할 엔트리 식별자.
     * @return 내부 값이 같으면 true, 다르면 false.
     */
    bool operator==(const FNameEntryId& Rhs) const;

    /**
     * @brief 두 엔트리 식별자의 내부 값이 다른지 비교한다.
     * @param Rhs 비교할 엔트리 식별자.
     * @return 내부 값이 다르면 true, 같으면 false.
     */
    bool operator!=(const FNameEntryId& Rhs) const;

private:
    /** @brief 이름 풀에서 사용하는 내부 엔트리 식별자 값. */
    Uint32 mValue{};
};

/** @brief 이름 엔트리의 문자 저장 형식과 문자열 길이를 비트 필드로 보관한다. */
struct FNameEntryHeader {
    /** @brief 와이드 문자 저장 여부를 나타내는 비트. 현재 이름 풀은 0으로 설정하여 char 문자열을 저장한다. */
    Uint16 mBIsWide : 1 {0};
    /** @brief 널 종료 문자를 제외한 저장 문자열의 바이트 수. */
    Uint16 mLen : 15 {0};
};

/**
 * @brief 이름 풀에 저장되는 문자열과 비교용 식별자를 보관한다.
 * @details 이름 풀은 엔트리 뒤에 문자열을 저장할 추가 공간을 할당한다. 엔트리는 복사와 이동을 허용하지 않는다.
 */
struct FNameEntry {
public:
    /** @brief 이름 풀 엔트리의 복사 생성을 금지한다. */
    FNameEntry(const FNameEntry&) = delete;
    /** @brief 이름 풀 엔트리의 이동 생성을 금지한다. */
    FNameEntry(FNameEntry&&) = delete;
    /** @brief 이름 풀 엔트리의 복사 할당을 금지한다. */
    FNameEntry& operator=(const FNameEntry&) = delete;
    /** @brief 이름 풀 엔트리의 이동 할당을 금지한다. */
    FNameEntry& operator=(FNameEntry&&) = delete;

    /**
     * @brief 저장된 이름 문자열의 길이를 조회한다.
     * @return 널 종료 문자를 제외한 문자열의 바이트 수.
     */
    Int32 GetNameLength() const;

    /**
     * @brief 대소문자를 구분하지 않는 이름 비교에 사용할 엔트리 식별자를 조회한다.
     * @return 저장된 비교용 엔트리 식별자.
     */
    FNameEntryId GetComparisonId() const;

    /**
     * @brief 엔트리의 비교용 식별자를 설정한다.
     * @param NewId 대소문자를 구분하지 않는 비교에 사용할 엔트리 식별자.
     */
    void SetComparisonId(FNameEntryId NewId);

    /**
     * @brief 이름 풀에 저장된 문자열의 시작 주소를 조회한다.
     * @return 이름 풀이 소유하는 널 종료 char 문자열의 읽기 전용 포인터.
     */
    const char* GetName() const;

private:
    /** @brief 저장 문자열의 형식과 길이를 보관하는 헤더. */
    FNameEntryHeader mHeader{};
    /** @brief 대소문자를 구분하지 않는 이름 비교에 사용하는 엔트리 식별자. */
    FNameEntryId mComparisonId{};
    /** @brief 문자열 저장 영역의 시작. 이름 풀에서 확보한 추가 공간에 널 종료 문자열을 저장한다. */
    Uint8 mNameData[1]{};
};

/**
 * @brief 표시용 이름과 비교용 이름을 공유 풀에 저장하고 숫자 접미사를 별도로 보관한다.
 * @details 문자열 표시는 입력의 대소문자를 보존하며 동등성 비교는 대소문자를 구분하지 않는 비교용 식별자와 숫자 값을 사용한다.
 * @details 내부 숫자 값은 0이면 접미사가 없고, 양수이면 문자열에 표시할 숫자보다 1 크다. 기본 생성된 이름은 None 상태이며 문자열로 변환하면 비어 있다.
 * @note 이름 풀에 저장할 기본 이름은 NameSize - 1바이트 이하여야 한다.
 */
class FName {
public:
    /** @brief 식별자와 숫자 값이 모두 0인 None 상태의 이름을 생성한다. */
    FName() = default;
    /**
     * @brief 문자열의 기본 이름과 숫자 접미사를 분리하여 이름 풀에 등록한다.
     * @param Str 생성할 이름 문자열. 빈 문자열이면 None 상태를 유지한다.
     * @details SplitNameAndNumber()로 마지막 숫자 접미사를 분리하고 기본 이름의 표시용 및 비교용 식별자를 저장한다.
     */
    FName(std::string_view Str);
    /**
     * @brief 널 종료 문자열에서 이름을 생성한다.
     * @param PStr 기본 이름과 숫자 접미사를 포함할 수 있는 문자열. nullptr 또는 빈 문자열이면 None 상태의 이름을 생성한다.
     */
    FName(const char* PStr);
    /**
     * @brief FString에서 기본 이름과 숫자 접미사를 분리하여 이름을 생성한다.
     * @param Str 생성할 이름 문자열. 빈 문자열이면 None 상태의 이름을 생성한다.
     */
    FName(FString Str);
    /**
     * @brief 기본 이름과 표시할 숫자를 명시하여 이름을 생성한다.
     * @param BaseName 이름 풀에 등록할 기본 이름. 빈 문자열이면 None 상태의 이름을 생성한다.
     * @param InNumber 접미사로 표시할 숫자. 0 이상이고 Int32의 최댓값 미만이면 적용하며, 그 외에는 접미사를 사용하지 않는다.
     * @details BaseName 자체의 숫자 접미사는 분리하지 않는다. 유효한 숫자는 내부에 InNumber + 1로 저장한다.
     */
    FName(std::string_view BaseName, Int32 InNumber);

    /**
     * @brief 비교용 엔트리 식별자를 먼저 비교하고 식별자가 같으면 내부 숫자 값을 비교한다.
     * @param Rhs 비교할 이름.
     * @return 현재 이름이 앞서면 -1, 같은 비교용 식별자와 숫자를 가지면 0, 뒤서면 1.
     * @details 정렬 순서는 문자열의 사전순이 아니라 이름 풀의 식별자 순서에 따라 결정되며 실행 간 안정성을 보장하지 않는다.
     */
    Int32 Compare(const FName& Rhs) const;
    /**
     * @brief 대소문자를 구분하지 않고 이름과 숫자 접미사의 동등성을 비교한다.
     * @param Rhs 비교할 이름.
     * @return 비교용 엔트리 식별자와 내부 숫자 값이 모두 같으면 true, 그 외에는 false.
     */
    bool operator==(const FName& Rhs) const;
    /**
     * @brief 현재 이름이 비교 대상보다 앞서는지 확인한다.
     * @param Rhs 비교할 이름.
     * @return Compare(Rhs)가 음수이면 true, 그 외에는 false.
     */
    bool operator<(const FName& Rhs) const;

    /**
     * @brief 이름이 빈 이름을 나타내는 None 상태인지 확인한다.
     * @return 비교용 엔트리 식별자의 내부 값이 0이면 true, 그 외에는 false.
     */
    bool IsNone() const;
    /**
     * @brief 기본 이름을 유지하면서 숫자 접미사를 바꾼 사본을 생성한다.
     * @param InNumber 새로 표시할 숫자. 0 이상이고 Int32의 최댓값 미만이면 적용하며, 그 외에는 접미사를 제거한다.
     * @return 표시용 및 비교용 식별자를 유지하고 숫자 값만 갱신한 이름. None 상태의 이름은 그대로 유지한다.
     */
    FName WithNumber(Int32 InNumber) const;
    /**
     * @brief 표시용 이름과 숫자 접미사를 문자열로 구성한다.
     * @return 숫자 접미사가 있으면 표시용 문자열에 밑줄과 표시 숫자를 붙인 값, 없으면 표시용 문자열. None 상태이면 빈 문자열.
     * @details 내부 숫자 값이 양수이면 그 값에서 1을 뺀 숫자를 접미사로 표시한다.
     */
    FString ToString() const;

    /**
     * @brief 입력의 대소문자를 보존하는 표시용 엔트리 식별자를 조회한다.
     * @return 저장된 표시용 엔트리 식별자. None 상태이면 내부 값이 0인 식별자.
     */
    FNameEntryId GetDisplayId() const;

    /**
     * @brief 대소문자를 구분하지 않는 이름 비교에 사용할 엔트리 식별자를 조회한다.
     * @return 저장된 비교용 엔트리 식별자.
     */
    FNameEntryId GetComparisonId() const;

    /**
     * @brief 숫자 접미사의 내부 저장 값을 조회한다.
     * @return 접미사가 없으면 0, 있으면 문자열에 표시할 숫자에 1을 더한 값.
     */
    Int32 GetNumber() const;

private:
    /** @brief 문자열의 대소문자를 보존하는 표시용 엔트리 식별자. */
    FNameEntryId mDisplayId{};
    /** @brief 대소문자를 구분하지 않는 이름 비교에 사용하는 엔트리 식별자. */
    FNameEntryId mComparisonId{};
    /** @brief 숫자 접미사의 내부 값. 0이면 접미사가 없고, 양수이면 표시할 숫자에 1을 더한 값이다. */
    Int32 mNumber{0};
};

/**
 * @brief 이름 문자열의 마지막 밑줄 뒤에 있는 유효한 숫자 접미사를 분리한다.
 * @param InString 분리할 이름 문자열.
 * @param[out] OutString 접미사를 제외한 기본 이름의 뷰. 분리할 수 없으면 전체 입력을 가리킨다.
 * @param[out] OutNumber 접미사 숫자에 1을 더한 내부 값. 분리할 수 없으면 0.
 * @details 기본 이름과 접미사가 모두 있어야 하며, 접미사는 0부터 Int32의 최댓값보다 1 작은 값까지의 십진수여야 한다. 두 자리 이상의 접미사는 0으로 시작할 수 없다.
 * @note OutString은 입력 문자열의 저장 영역을 참조하므로 사용하는 동안 원본 문자열을 유지해야 한다.
 */
void SplitNameAndNumber(std::string_view InString, std::string_view& OutString, Int32& OutNumber);

/** @brief FName의 비교용 식별자와 숫자 값을 사용하여 해시 기반 컨테이너의 키를 지원한다. */
template <>
struct std::hash<FName> {
public:
    /**
     * @brief 이름의 동등성 비교 기준에 맞는 해시 값을 계산한다.
     * @param Name 해시 값을 계산할 이름.
     * @return 비교용 엔트리 식별자와 내부 숫자 값을 결합한 해시.
     * @details 비교용 식별자가 실행 간 안정성을 보장하지 않으므로 이 해시도 영속 식별자로 사용할 수 없다.
     */
    std::size_t operator()(const FName& Name) const noexcept;
};
