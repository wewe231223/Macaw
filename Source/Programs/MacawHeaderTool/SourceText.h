/**
 * @file SourceText.h
 * @brief 헤더 툴이 읽는 소스 텍스트, 마스크와 토큰입니다.
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace HeaderTool {

    /**
     * @brief 소스 위치와 함께 보고하는 헤더 툴 오류입니다.
     */
    struct FHeaderToolError {
        /**
         * @brief 작업 공간 기준 파일 경로입니다.
         */
        std::string Path;

        /**
         * @brief 1부터 시작하는 줄 번호입니다. 0이면 줄을 표시하지 않습니다.
         */
        int Line{};

        /**
         * @brief 오류 설명입니다.
         */
        std::string Message;
    };

    /**
     * @brief 토큰 종류입니다.
     */
    enum class ETokenKind {
        /**
         * @brief 식별자 토큰입니다.
         */
        Identifier,

        /**
         * @brief 숫자 리터럴 토큰입니다.
         */
        Number,

        /**
         * @brief 연산자·괄호 등 구두점 토큰입니다.
         */
        Punctuation
    };

    /**
     * @brief 마스크에서 읽은 토큰입니다. 주석, 문자열과 전처리 줄은 토큰이 되지 않습니다.
     */
    struct FToken {
        /**
         * @brief 토큰 종류입니다.
         */
        ETokenKind Kind{};

        /**
         * @brief 토큰 문자열입니다. "::"는 한 토큰입니다.
         */
        std::string Text;

        /**
         * @brief 원문 기준 시작 위치입니다.
         */
        std::size_t Offset{};
    };

    /**
     * @brief 읽어 들인 소스 파일과 파생 데이터입니다.
     */
    struct FSourceFile {
        /**
         * @brief "Source/"로 시작하는 작업 공간 기준 경로입니다.
         */
        std::string Path;

        /**
         * @brief Source 바로 아래 디렉터리 이름입니다.
         */
        std::string Module;

        /**
         * @brief 파일 원문입니다.
         */
        std::string Text;

        /**
         * @brief 주석, 문자열, 문자 리터럴과 전처리 줄을 공백으로 바꾼 원문과 같은 길이의 텍스트입니다.
         */
        std::string Mask;

        /**
         * @brief 마스크에서 읽은 토큰입니다.
         */
        std::vector<FToken> Tokens;

        /**
         * @brief 원문 위치의 줄 번호를 반환합니다.
         * @param[in] Offset 원문 기준 위치입니다.
         * @return 1부터 시작하는 줄 번호입니다.
         */
        int LineAt(std::size_t Offset) const;

        /**
         * @brief 토큰 위치를 담아 오류를 던집니다.
         * @param[in] TokenIndex 오류 위치의 토큰 번호입니다.
         * @param[in] Message 오류 설명입니다.
         */
        [[noreturn]] void Fail(std::size_t TokenIndex, const std::string& Message) const;

        /**
         * @brief 두 토큰 사이의 원문을 앞뒤 공백 없이 반환합니다.
         * @param[in] First 첫 토큰 번호입니다.
         * @param[in] Last 끝 토큰 번호입니다. 이 토큰은 포함하지 않습니다.
         * @return 원문 조각입니다.
         */
        std::string TextBetween(std::size_t First, std::size_t Last) const;

        /**
         * @brief 두 토큰 사이의 마스크를 앞뒤 공백 없이 반환합니다.
         * @param[in] First 첫 토큰 번호입니다.
         * @param[in] Last 끝 토큰 번호입니다. 이 토큰은 포함하지 않습니다.
         * @return 마스크 조각입니다.
         */
        std::string MaskBetween(std::size_t First, std::size_t Last) const;

        /**
         * @brief 여는 괄호 토큰과 짝이 맞는 닫는 괄호 토큰을 찾습니다.
         * @param[in] Opening 여는 괄호 토큰 번호입니다.
         * @return 닫는 괄호 토큰 번호입니다.
         */
        std::size_t FindClosing(std::size_t Opening) const;

        /**
         * @brief 토큰이 지정한 문자열인지 검사합니다.
         * @param[in] Index 검사할 토큰 번호입니다. 범위를 벗어나면 false입니다.
         * @param[in] Text 비교할 문자열입니다.
         * @return 같으면 true입니다.
         */
        bool Is(std::size_t Index, std::string_view Text) const;
    };

    /**
     * @brief 파일을 읽고 마스크와 토큰을 만듭니다.
     * @param[in] Root 작업 공간 루트입니다.
     * @param[in] Path "Source/"로 시작하는 작업 공간 기준 경로입니다.
     * @return 읽은 소스 파일입니다.
     */
    FSourceFile ReadSourceFile(const std::string& Root, const std::string& Path);

    /**
     * @brief 앞뒤 공백을 제거합니다.
     * @param[in] Text 원본 문자열입니다.
     * @return 공백을 제거한 문자열입니다.
     */
    std::string Trim(std::string_view Text);

    /**
     * @brief 식별자에 쓰일 수 있는 문자인지 검사합니다.
     * @param[in] Character 검사할 문자입니다.
     * @return 영문자, 숫자 또는 밑줄이면 true입니다.
     */
    bool IsIdentifierCharacter(char Character);

    /**
     * @brief 단어 경계에 있는 단어를 모두 지웁니다.
     * @param[in] Text 원본 문자열입니다.
     * @param[in] Word 지울 단어입니다.
     * @return 단어를 지운 문자열입니다.
     */
    std::string RemoveWord(std::string_view Text, std::string_view Word);

    /**
     * @brief 단어 경계에 있는 단어가 있는지 검사합니다.
     * @param[in] Text 검사할 문자열입니다.
     * @param[in] Word 찾을 단어입니다.
     * @return 있으면 true입니다.
     */
    bool ContainsWord(std::string_view Text, std::string_view Word);

    /**
     * @brief 꺾쇠와 소괄호 깊이가 0인 쉼표로 나눕니다. 마지막 조각은 공백만 있으면 버립니다.
     * @param[in] Text 나눌 문자열입니다.
     * @return 나눈 조각입니다. 공백은 제거하지 않습니다.
     */
    std::vector<std::string> SplitTopLevel(std::string_view Text);

}
