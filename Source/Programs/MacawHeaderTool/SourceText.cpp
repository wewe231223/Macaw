/**
 * @file SourceText.cpp
 * @brief 원문 위치를 보존하는 마스크·토큰과 헤더 툴 진단을 구현합니다.
 */

#include "Core/Types.h"
#include "SourceText.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace HeaderTool {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {

        /**
         * @brief 줄바꿈을 유지하면서 원문 범위를 공백으로 가립니다.
         * @param[in,out] Mask 원문과 길이가 같은 마스킹 문자열입니다.
         * @param[in] First 처리 구간의 시작 위치입니다.
         * @param[in] Last 처리 구간의 끝 위치이며 포함하지 않습니다.
         */
        void Blank(std::string& Mask, Core::Index_t First, Core::Index_t Last) {
            for (Core::Index_t Index{First}; Index < Last && Index < Mask.size(); ++Index) {
                if (Mask[Index] != '\r' && Mask[Index] != '\n') {
                    Mask[Index] = ' ';
                }
            }
        }

        /**
         * @brief 지정 위치가 속한 줄의 끝을 찾습니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @param[in] Position 문자열 탐색 시작 위치입니다.
         * @return 다음 줄바꿈 또는 문자열 끝 위치입니다.
         */
        std::size_t LineEnd(const std::string& Text, std::size_t Position) {
            const std::size_t End{Text.find('\n', Position)};

            return End == std::string::npos ? Text.size() : End;
        }

        // 전처리 줄은 역슬래시로 이어진 줄까지 지웁니다.

        /**
         * @brief 줄 연결 역슬래시를 고려해 전처리 지시문 끝을 찾습니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @param[in] Position 문자열 탐색 시작 위치입니다.
         * @return 줄 연결을 포함한 전처리 지시문 끝 위치입니다.
         */
        std::size_t DirectiveEnd(const std::string& Text, std::size_t Position) {
            std::size_t End{LineEnd(Text, Position)};

            while (End < Text.size()) {
                Core::Index_t Last{static_cast<Core::Index_t>(End)};

                while (Last > Position && (Text[Last - 1] == '\r' || Text[Last - 1] == ' ' || Text[Last - 1] == '\t')) {
                    --Last;
                }

                if (Last == Position || Text[Last - 1] != '\\') {
                    break;
                }

                End = LineEnd(Text, End + 1);
            }

            return End;
        }

        /**
         * @brief 원문 위치를 유지하면서 주석·문자열·문자·전처리 구간을 공백으로 가립니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @param[in] Path 작업에 사용할 파일 또는 프로젝트 상대 에셋 경로입니다.
         * @return 원문과 길이·줄바꿈 위치가 같은 마스크입니다.
         */
        std::string MaskSource(const std::string& Text, const std::string& Path) {
            std::string Mask{Text};
            std::size_t Position{};

            while (Position < Text.size()) {
                const char Character{Text[Position]};
                const char Next{Position + 1 < Text.size() ? Text[Position + 1] : '\0'};
                Core::Index_t Last{static_cast<Core::Index_t>(Position + 1)};

                if (Character == '/' && Next == '/') {
                    Last = LineEnd(Text, Position);
                } else if (Character == '#') {
                    Last = DirectiveEnd(Text, Position);
                } else if (Character == '/' && Next == '*') {
                    const std::size_t Finish{Text.find("*/", Position + 2)};

                    if (Finish == std::string::npos) {
                        throw FHeaderToolError{Path, 0, "Unterminated comment"};
                    }

                    Last = Finish + 2;
                } else if (Character == 'R' && Next == '"') {
                    const Core::Index_t Opening{static_cast<Core::Index_t>(Text.find('(', Position + 2))};

                    if (Opening == std::string::npos) {
                        throw FHeaderToolError{Path, 0, "Invalid raw string"};
                    }

                    const std::string Closing{")" + Text.substr(Position + 2, Opening - Position - 2) + "\""};
                    const std::size_t Finish{Text.find(Closing, Opening + 1)};

                    if (Finish == std::string::npos) {
                        throw FHeaderToolError{Path, 0, "Unterminated raw string"};
                    }

                    Last = Finish + Closing.size();
                } else if (Character == '"' || (Character == '\'' && (Position == 0 || !std::isdigit(static_cast<unsigned char>(Text[Position - 1]))))) {
                    Last = Position + 1;

                    while (Last < Text.size() && Text[Last] != Character) {
                        Last += Text[Last] == '\\' ? 2 : 1;
                    }

                    if (Last >= Text.size()) {
                        throw FHeaderToolError{Path, 0, "Unterminated string"};
                    }

                    ++Last;
                } else {
                    ++Position;

                    continue;
                }

                Blank(Mask, Position, Last);

                Position = Last;
            }

            return Mask;
        }

        /**
         * @brief 마스크에서 식별자·숫자·구두점 토큰과 원문 위치를 수집합니다.
         * @param[in,out] Mask 원문과 길이가 같은 마스킹 문자열입니다.
         * @return 원문 위치를 가진 토큰 목록입니다.
         */
        std::vector<FToken> Tokenize(const std::string& Mask) {
            std::vector<FToken> Tokens;
            std::size_t Position{};

            while (Position < Mask.size()) {
                const char Character{Mask[Position]};

                if (std::isspace(static_cast<unsigned char>(Character))) {
                    ++Position;

                    continue;
                }

                Core::Index_t Last{static_cast<Core::Index_t>(Position + 1)};
                ETokenKind Kind{ETokenKind::Punctuation};

                if (std::isalpha(static_cast<unsigned char>(Character)) || Character == '_') {
                    Kind = ETokenKind::Identifier;

                    while (Last < Mask.size() && IsIdentifierCharacter(Mask[Last])) {
                        ++Last;
                    }
                } else if (std::isdigit(static_cast<unsigned char>(Character))) {
                    Kind = ETokenKind::Number;

                    while (Last < Mask.size() && (IsIdentifierCharacter(Mask[Last]) || Mask[Last] == '.' || Mask[Last] == '\'')) {
                        ++Last;
                    }
                } else if (Character == ':' && Last < Mask.size() && Mask[Last] == ':') {
                    ++Last;
                }

                Tokens.push_back({Kind, Mask.substr(Position, Last - Position), Position});

                Position = Last;
            }

            return Tokens;
        }

    }

    /**
     * @brief 원문 위치의 줄 번호를 반환합니다.
     */
    int FSourceFile::LineAt(std::size_t Offset) const {
        return 1 + static_cast<int>(std::count(Text.begin(), Text.begin() + static_cast<std::ptrdiff_t>(std::min(Offset, Text.size())), '\n'));
    }

    /**
     * @brief 토큰 위치를 담아 오류를 던집니다.
     */
    void FSourceFile::Fail(Core::Index_t TokenIndex, const std::string& Message) const {
        throw FHeaderToolError{Path, TokenIndex >= 0 && TokenIndex < Tokens.size() ? LineAt(Tokens[TokenIndex].Offset) : 0, Message};
    }

    /**
     * @brief 두 토큰 사이의 원문을 앞뒤 공백 없이 반환합니다.
     */
    std::string FSourceFile::TextBetween(Core::Index_t First, Core::Index_t Last) const {
        const std::size_t Begin{First >= 0 && First < Tokens.size() ? Tokens[First].Offset : Text.size()};
        const std::size_t End{Last >= 0 && Last < Tokens.size() ? Tokens[Last].Offset : Text.size()};

        return End > Begin ? Trim(std::string_view{Text}.substr(Begin, End - Begin)) : std::string{};
    }

    /**
     * @brief 두 토큰 사이의 마스크를 앞뒤 공백 없이 반환합니다.
     */
    std::string FSourceFile::MaskBetween(Core::Index_t First, Core::Index_t Last) const {
        const std::size_t Begin{First >= 0 && First < Tokens.size() ? Tokens[First].Offset : Mask.size()};
        const std::size_t End{Last >= 0 && Last < Tokens.size() ? Tokens[Last].Offset : Mask.size()};

        return End > Begin ? Trim(std::string_view{Mask}.substr(Begin, End - Begin)) : std::string{};
    }

    /**
     * @brief 여는 괄호 토큰과 짝이 맞는 닫는 괄호 토큰을 찾습니다.
     */
    Core::Index_t FSourceFile::FindClosing(Core::Index_t Opening) const {
        const std::string& Open{Tokens[Opening].Text};
        const std::string Close{Open == "(" ? ")" : (Open == "{" ? "}" : (Open == "[" ? "]" : ">"))};
        int Depth{};

        for (Core::Index_t Index{Opening}; Index < Tokens.size(); ++Index) {
            if (Tokens[Index].Text == Open) {
                ++Depth;
            } else if (Tokens[Index].Text == Close && --Depth == 0) {
                return Index;
            }
        }

        Fail(Opening, "Unterminated '" + Open + "'");
    }

    /**
     * @brief 토큰이 지정한 문자열인지 검사합니다.
     */
    bool FSourceFile::Is(Core::Index_t Index, std::string_view Value) const {
        return Index >= 0 && Index < Tokens.size() && Tokens[Index].Text == Value;
    }

    /**
     * @brief 파일을 읽고 마스크와 토큰을 만듭니다.
     */
    FSourceFile ReadSourceFile(const std::string& Root, const std::string& Path) {
        std::ifstream Stream{std::filesystem::path{Root} / std::filesystem::path{Path}, std::ios::binary};

        if (!Stream) {
            throw FHeaderToolError{Path, 0, "Cannot read file"};
        }

        FSourceFile File;
        File.Path = Path;
        File.Text.assign(std::istreambuf_iterator<char>{Stream}, std::istreambuf_iterator<char>{});

        const std::size_t Slash{Path.find('/', 7)};

        File.Module = Path.substr(7, Slash == std::string::npos ? std::string::npos : Slash - 7);
        File.Mask = MaskSource(File.Text, Path);
        File.Tokens = Tokenize(File.Mask);

        return File;
    }

    /**
     * @brief 앞뒤 공백을 제거합니다.
     */
    std::string Trim(std::string_view Text) {
        Core::Index_t First{};
        Core::Index_t Last{static_cast<Core::Index_t>(Text.size())};

        while (First < Last && std::isspace(static_cast<unsigned char>(Text[First]))) {
            ++First;
        }

        while (Last > First && std::isspace(static_cast<unsigned char>(Text[Last - 1]))) {
            --Last;
        }

        return std::string{Text.substr(First, Last - First)};
    }

    /**
     * @brief 식별자에 쓰일 수 있는 문자인지 검사합니다.
     */
    bool IsIdentifierCharacter(char Character) {
        return std::isalnum(static_cast<unsigned char>(Character)) || Character == '_';
    }

    namespace {

        /**
         * @brief 지정 위치부터 단어 경계가 맞는 식별자를 찾습니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @param[in] Word 단어 경계와 함께 찾을 식별자입니다.
         * @param[in] Position 문자열 탐색 시작 위치입니다.
         * @return 단어 시작 위치이며 없으면 std::string_view::npos입니다.
         */
        std::size_t FindWord(std::string_view Text, std::string_view Word, std::size_t Position) {
            while ((Position = Text.find(Word, Position)) != std::string_view::npos) {
                const bool Before{Position > 0 && IsIdentifierCharacter(Text[Position - 1])};
                const bool After{Position + Word.size() < Text.size() && IsIdentifierCharacter(Text[Position + Word.size()])};

                if (!Before && !After) {
                    return Position;
                }

                ++Position;
            }

            return std::string_view::npos;
        }

    }

    /**
     * @brief 단어 경계에 있는 단어를 모두 지웁니다.
     */
    std::string RemoveWord(std::string_view Text, std::string_view Word) {
        std::string Result;
        std::size_t Position{};
        std::size_t Found{};

        while ((Found = FindWord(Text, Word, Position)) != std::string_view::npos) {
            Result.append(Text.substr(Position, Found - Position));

            Position = Found + Word.size();
        }

        Result.append(Text.substr(Position));

        return Result;
    }

    /**
     * @brief 단어 경계에 있는 단어가 있는지 검사합니다.
     */
    bool ContainsWord(std::string_view Text, std::string_view Word) {
        return FindWord(Text, Word, 0) != std::string_view::npos;
    }

    /**
     * @brief 꺾쇠와 소괄호 깊이가 0인 쉼표로 나눕니다. 마지막 조각은 공백만 있으면 버립니다.
     */
    std::vector<std::string> SplitTopLevel(std::string_view Text) {
        std::vector<std::string> Parts;
        int Depth{};
        Core::Index_t First{};

        for (std::size_t Position{}; Position < Text.size(); ++Position) {
            const char Character{Text[Position]};

            Depth += (Character == '<' || Character == '(') ? 1 : ((Character == '>' || Character == ')') ? -1 : 0);

            if (Character == ',' && Depth == 0) {
                Parts.emplace_back(Text.substr(First, Position - First));

                First = Position + 1;
            }
        }

        if (!Trim(Text.substr(First)).empty()) {
            Parts.emplace_back(Text.substr(First));
        }

        return Parts;
    }

}
