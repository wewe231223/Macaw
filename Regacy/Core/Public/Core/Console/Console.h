/**
 * @file Console.h
 * @brief 출력 핸들별 콘솔 메시지 이력, 로그 추가 및 대기 로그 반영 인터페이스를 정의한다.
 */

#pragma once
#include "Core/CoreMinimal.h"

/**
 * @brief 콘솔 메시지의 로그 수준을 구분한다.
 */
enum class ELogLevel {
    /**
     * @brief 일반 정보 메시지 수준.
     */
    Log,
    /**
     * @brief 경고 메시지 수준.
     */
    Warning,
    /**
     * @brief 오류 메시지 수준. 이력에 기록할 때 ErrorHandler에도 경고 수준으로 보고한다.
     */
    Error,
    /**
     * @brief 치명적 오류로 분류하는 로그 수준. 콘솔은 이 수준만으로 프로세스를 종료하지 않는다.
     */
    Fatal
};

/**
 * @brief 콘솔 메시지가 발생한 기능 영역을 구분한다.
 */
enum class ELogCategory {
    /**
     * @brief 엔진의 기반 기능에 대한 로그 분류.
     */
    Core,
    /**
     * @brief 렌더링 기능에 대한 로그 분류.
     */
    Render,
    /**
     * @brief 물리 기능에 대한 로그 분류.
     */
    Physics,
    /**
     * @brief 그 밖의 기능에 대한 로그 분류.
     */
    Etc
};

/**
 * @brief 기록 시간, 로그 수준, 분류 및 본문을 보관하는 콘솔 메시지.
 */
struct FConsoleMessage {
    /**
     * @brief HH:MM:SS 형식의 로컬 기록 시각. Print() 또는 AddLog()에서 설정한다.
     */
    FString mTime{};

    /**
     * @brief 메시지의 로그 수준. 기본값은 일반 정보 수준이다.
     */
    ELogLevel mLevel{ELogLevel::Log};

    /**
     * @brief 메시지의 기능 영역 분류. 기본값은 Etc이다.
     */
    ELogCategory mCategory{ELogCategory::Etc};

    /**
     * @brief 콘솔에 표시할 메시지 본문.
     */
    FString mText{};
};

/**
 * @brief 출력별 로그 저장소를 조회하는 인덱스 기반 핸들.
 */
struct FConsoleOutputHandle {
    /**
     * @brief 출력 저장소의 인덱스. UINT32_MAX는 미지정 핸들을 나타낸다.
     */
    std::uint32_t mIndex{UINT32_MAX};

    /**
     * @brief 출력 인덱스가 지정되어 있는지 확인한다.
     * @return mIndex가 UINT32_MAX가 아니면 true, 미지정 값이면 false.
     * @details 해당 인덱스의 출력 저장소가 실제로 존재하는지는 검사하지 않는다.
     */
    bool IsValid() const;
};

/**
 * @brief 출력 핸들별로 메모리 내 로그 이력과 대기 로그를 관리한다.
 * @details 현재 기본 출력 저장소 두 개는 각각 최근 1000개의 메시지를 순환 이력으로 보관한다. 용량을 넘기면 가장 오래된 메시지를 덮어쓴다.
 * @note 대기 로그 추가와 버퍼 교환은 뮤텍스로 보호하지만 이력 접근과 Flush() 호출의 동기화는 호출자가 관리한다.
 */
namespace Console {
    /**
     * @brief 일반 로그용 기본 출력 저장소의 핸들. 인덱스는 0이다.
     */
    inline constexpr FConsoleOutputHandle STDOutHandle{0};

    /**
     * @brief 오류 로그용 기본 출력 저장소의 핸들. 인덱스는 1이다.
     */
    inline constexpr FConsoleOutputHandle STDErrorHandle{1};

    /**
     * @brief 콘솔 메시지에 현재 시각을 설정하여 출력 이력에 즉시 기록한다.
     * @param Handle 기록할 출력 저장소의 핸들.
     * @param Message 기록할 메시지. 기존 시간 필드는 현재 로컬 시각으로 교체한다.
     * @details 대응하는 출력 저장소가 없으면 기록하지 않는다. 이력이 가득 차면 가장 오래된 메시지를 덮어쓴다.
     */
    void Print(FConsoleOutputHandle Handle, FConsoleMessage Message);

    /**
     * @brief 지정한 출력의 표시 이력을 비운다.
     * @param Handle 이력을 비울 출력 핸들.
     * @details 대응하는 저장소가 없으면 아무 작업도 하지 않는다. 메시지 수와 이력 시작 위치만 초기화하므로 대기 로그는 유지되며 이후 Flush()로 반영된다.
     */
    void Clear(FConsoleOutputHandle Handle);

    /**
     * @brief printf 형식의 로그를 생성하여 출력별 대기 버퍼에 추가한다.
     * @param Handle 로그를 받을 출력 핸들.
     * @param Level 메시지의 로그 수준.
     * @param Categor 메시지의 기능 영역 분류.
     * @param Format 가변 인수를 적용할 널 종료 printf 형식 문자열.
     * @details 가변 인수는 Format에 지정한 형식과 일치해야 한다. 형식 적용에 실패하거나 출력 저장소가 없으면 로그를 추가하지 않는다.
     * @details 시간은 로그를 추가할 때 설정하며 이력에는 Flush() 호출 후 반영된다. 대기 버퍼에 추가하는 작업은 뮤텍스로 보호한다.
     */
    void AddLog(FConsoleOutputHandle Handle, ELogLevel Level, ELogCategory Categor, const char* Format, ...);

    /**
     * @brief 지정한 출력의 대기 로그를 표시 이력에 반영한다.
     * @param Handle 대기 로그를 반영할 출력 핸들.
     * @details 뮤텍스 안에서 읽기 및 쓰기 버퍼를 교환한 뒤 읽기 버퍼의 메시지를 이력에 기록하고 읽기 버퍼를 비운다. 메시지의 기존 기록 시각은 유지한다.
     * @details 대응하는 출력 저장소가 없으면 아무 작업도 하지 않는다.
     */
    void Flush(FConsoleOutputHandle Handle);

    /**
     * @brief 지정한 출력의 표시 이력에 있는 메시지 수를 조회한다.
     * @param Handle 조회할 출력 핸들.
     * @return 이력 메시지 수. 대응하는 출력 저장소가 없으면 0.
     * @details 아직 Flush()하지 않은 대기 로그는 포함하지 않는다.
     */
    std::size_t GetMessageCount(FConsoleOutputHandle Handle);

    /**
     * @brief 지정한 출력 이력에서 오래된 순서로 메시지를 조회한다.
     * @param Handle 조회할 출력 핸들.
     * @param Index 가장 오래된 메시지를 0으로 하는 이력 인덱스.
     * @return 이력 저장소가 소유하는 메시지의 읽기 전용 참조.
     * @pre Handle에 대응하는 출력 저장소가 존재하고 Index가 GetMessageCount(Handle) 미만이어야 한다.
     * @details 핸들과 인덱스의 범위를 직접 검사하지 않는다. 반환한 메시지의 내용은 이후 Print() 또는 Flush()가 순환 이력을 덮어쓰면 변경될 수 있다.
     */
    const FConsoleMessage& GetMessageAt(FConsoleOutputHandle Handle, std::size_t Index);
}
