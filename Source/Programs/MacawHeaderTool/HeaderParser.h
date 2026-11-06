/**
 * @file HeaderParser.h
 * @brief UCLASS, USTRUCT, UENUM, UPROPERTY, UFUNCTION과 GENERATED_BODY를 읽는 헤더 파서입니다.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "SourceText.h"

namespace HeaderTool {

    /**
     * @brief 리플렉션 타입 종류입니다.
     */
    enum class ETypeKind {
        /**
         * @brief 해당 값이나 대상을 지정하지 않은 상태입니다.
         */
        None,

        /**
         * @brief 구조체로 전달하는 바인딩 또는 리플렉션 구조체입니다.
         */
        Struct,

        /**
         * @brief UObject 파생 리플렉션 클래스입니다.
         */
        Class,

        /**
         * @brief 기반 클래스가 없는 최상위 리플렉션 클래스입니다.
         */
        RootClass
    };

    /**
     * @brief UPROPERTY로 표시한 멤버입니다.
     */
    struct FProperty {
        /**
         * @brief 멤버 변수 이름입니다.
         */
        std::string Member;

        /**
         * @brief 따옴표를 포함한 리플렉션 이름 리터럴입니다.
         */
        std::string Name;

        /**
         * @brief getter 멤버 함수 이름입니다. 없으면 비어 있습니다.
         */
        std::string Getter;

        /**
         * @brief setter 멤버 함수 이름입니다. 없으면 비어 있습니다.
         */
        std::string Setter;

        /**
         * @brief 적은 순서의 EPropertyFlags 값 이름입니다.
         */
        std::vector<std::string> Flags;

        /**
         * @brief 멤버 타입이 TArray이면 true입니다.
         */
        bool Array{};
    };

    /**
     * @brief 헤더에 정의된 class 또는 struct입니다. 리플렉션하지 않는 타입도 포함합니다.
     */
    struct FType {
        /**
         * @brief 바깥 타입을 포함한 이름입니다. 네임스페이스는 포함하지 않습니다.
         */
        std::string Name;

        /**
         * @brief 타입 자신의 이름입니다.
         */
        std::string ShortName;

        /**
         * @brief "::"로 이은 네임스페이스입니다.
         */
        std::string Namespace;

        /**
         * @brief "class" 또는 "struct"입니다.
         */
        std::string Keyword;

        /**
         * @brief 첫 번째 부모 타입을 적힌 그대로 담습니다. 없으면 비어 있습니다.
         */
        std::string Super;

        /**
         * @brief 리플렉션 종류입니다. None이면 리플렉션하지 않습니다.
         */
        ETypeKind Kind{};

        /**
         * @brief EClassFlags 값 이름입니다.
         */
        std::vector<std::string> ClassFlags;

        /**
         * @brief 선언 순서의 프로퍼티입니다.
         */
        std::vector<FProperty> Properties;

        /**
         * @brief 여는 중괄호 토큰 번호입니다.
         */
        std::size_t Opening{};

        /**
         * @brief 닫는 중괄호 토큰 번호입니다.
         */
        std::size_t Closing{};

        /**
         * @brief 바로 바깥 타입의 번호입니다. 없으면 -1입니다.
         */
        int Outer{-1};

        /**
         * @brief 템플릿이거나 템플릿 안에 있으면 true입니다.
         */
        bool Template{};

        /**
         * @brief GENERATED_BODY()가 있는 줄 번호입니다. 없으면 0입니다.
         */
        int GeneratedBodyLine{};

        /**
         * @brief 정의된 파일 경로입니다.
         */
        std::string File;

        /**
         * @brief 정의된 파일의 Source 아래 디렉터리 이름입니다.
         */
        std::string SourceModule;

        /**
         * @brief C# 래퍼 클래스 이름입니다. 래퍼가 없으면 비어 있습니다.
         */
        std::string ScriptName;

        /**
         * @brief C# 래퍼의 부모 클래스 이름입니다.
         */
        std::string ScriptSuper;
    };

    /**
     * @brief UENUM으로 표시한 열거형입니다.
     */
    struct FEnum {
        /**
         * @brief 열거형 이름입니다.
         */
        std::string Name;

        /**
         * @brief "::"로 이은 네임스페이스입니다.
         */
        std::string Namespace;

        /**
         * @brief 바탕 타입입니다. 없으면 비어 있습니다.
         */
        std::string Base;

        /**
         * @brief enum class이면 true입니다.
         */
        bool Scoped{};

        /**
         * @brief 네이티브 리플렉션을 만들면 true입니다.
         */
        bool Reflect{};

        /**
         * @brief C# 열거형을 만들면 true입니다.
         */
        bool Script{};

        /**
         * @brief 리플렉션할 값 이름입니다. UMETA(Hidden) 값은 제외합니다.
         */
        std::vector<std::string> Values;

        /**
         * @brief C#에 그대로 적을 값 항목입니다. 초기화식을 포함합니다.
         */
        std::vector<std::string> ScriptValues;

        /**
         * @brief 정의된 파일 경로입니다.
         */
        std::string File;
    };

    /**
     * @brief UFUNCTION 매개변수입니다.
     */
    struct FParameter {
        /**
         * @brief const와 참조를 지운 타입입니다.
         */
        std::string Type;

        /**
         * @brief 매개변수 이름입니다.
         */
        std::string Name;
    };

    /**
     * @brief UFUNCTION으로 표시한 멤버 함수입니다.
     */
    struct FFunction {
        /**
         * @brief 함수 이름입니다.
         */
        std::string Name;

        /**
         * @brief 함수를 가진 타입입니다. 소유 헤더가 살아 있는 동안 유효합니다.
         */
        const FType* Owner{};

        /**
         * @brief 선언된 파일 경로입니다.
         */
        std::string File;

        /**
         * @brief 정적 함수이면 true입니다.
         */
        bool Static{};

        /**
         * @brief C# 쪽을 internal로 만들면 true입니다.
         */
        bool Internal{};

        /**
         * @brief const와 참조를 지운 반환 타입입니다.
         */
        std::string Return;

        /**
         * @brief 매개변수입니다.
         */
        std::vector<FParameter> Parameters;

        /**
         * @brief 오류 메시지에 쓸 위치 설명입니다.
         */
        std::string Label;
    };

    /**
     * @brief 파싱한 헤더 하나입니다.
     */
    struct FHeader {
        /**
         * @brief 소스 파일입니다.
         */
        FSourceFile Source;

        /**
         * @brief 정의 순서의 모든 타입입니다.
         */
        std::vector<FType> Types;

        /**
         * @brief 선언 순서의 UENUM입니다.
         */
        std::vector<FEnum> Enums;

        /**
         * @brief 선언 순서의 UFUNCTION입니다.
         */
        std::vector<FFunction> Functions;

        /**
         * @brief generated.h가 필요하면 true입니다.
         */
        bool NeedsGeneratedHeader{};

        /**
         * @brief generated.h 파일 이름입니다.
         */
        std::string GeneratedName;

        /**
         * @brief generated.h가 정의하는 CURRENT_FILE_ID 값입니다.
         */
        std::string FileId;
    };

    /**
     * @brief 헤더를 파싱합니다.
     * @param[in] Source 읽은 헤더입니다.
     * @return 파싱한 헤더입니다. 타입 주소가 바뀌지 않도록 힙에 둡니다.
     */
    std::unique_ptr<FHeader> ParseHeader(FSourceFile Source);

    /**
     * @brief 소스 파일에 리플렉션 어노테이션이 없는지 검사합니다.
     * @param[in] Source 검사할 .cpp 파일입니다.
     */
    void CheckSourceFile(const FSourceFile& Source);

}
