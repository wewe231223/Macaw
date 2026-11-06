#include "SourceText.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace HeaderTool {

    namespace {

        void Blank(std::string& Mask, std::size_t First, std::size_t Last) {
            for (std::size_t Index{First}; Index < Last && Index < Mask.size(); ++Index) {
                if (Mask[Index] != '\r' && Mask[Index] != '\n') {
                    Mask[Index] = ' ';
                }
            }
        }

        std::size_t LineEnd(const std::string& Text, std::size_t Position) {
            const std::size_t End{Text.find('\n', Position)};

            return End == std::string::npos ? Text.size() : End;
        }

        // 전처리 줄은 역슬래시로 이어진 줄까지 지웁니다.
        std::size_t DirectiveEnd(const std::string& Text, std::size_t Position) {
            std::size_t End{LineEnd(Text, Position)};

            while (End < Text.size()) {
                std::size_t Last{End};

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

        std::string MaskSource(const std::string& Text, const std::string& Path) {
            std::string Mask{Text};
            std::size_t Position{};

            while (Position < Text.size()) {
                const char Character{Text[Position]};
                const char Next{Position + 1 < Text.size() ? Text[Position + 1] : '\0'};
                std::size_t Last{Position + 1};

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
                    const std::size_t Opening{Text.find('(', Position + 2)};

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

        std::vector<FToken> Tokenize(const std::string& Mask) {
            std::vector<FToken> Tokens;
            std::size_t Position{};

            while (Position < Mask.size()) {
                const char Character{Mask[Position]};

                if (std::isspace(static_cast<unsigned char>(Character))) {
                    ++Position;
                    continue;
                }

                std::size_t Last{Position + 1};
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

    int FSourceFile::LineAt(std::size_t Offset) const {
        return 1 + static_cast<int>(std::count(Text.begin(), Text.begin() + static_cast<std::ptrdiff_t>(std::min(Offset, Text.size())), '\n'));
    }

    void FSourceFile::Fail(std::size_t TokenIndex, const std::string& Message) const {
        throw FHeaderToolError{Path, TokenIndex < Tokens.size() ? LineAt(Tokens[TokenIndex].Offset) : 0, Message};
    }

    std::string FSourceFile::TextBetween(std::size_t First, std::size_t Last) const {
        const std::size_t Begin{First < Tokens.size() ? Tokens[First].Offset : Text.size()};
        const std::size_t End{Last < Tokens.size() ? Tokens[Last].Offset : Text.size()};

        return End > Begin ? Trim(std::string_view{Text}.substr(Begin, End - Begin)) : std::string{};
    }

    std::string FSourceFile::MaskBetween(std::size_t First, std::size_t Last) const {
        const std::size_t Begin{First < Tokens.size() ? Tokens[First].Offset : Mask.size()};
        const std::size_t End{Last < Tokens.size() ? Tokens[Last].Offset : Mask.size()};

        return End > Begin ? Trim(std::string_view{Mask}.substr(Begin, End - Begin)) : std::string{};
    }

    std::size_t FSourceFile::FindClosing(std::size_t Opening) const {
        const std::string& Open{Tokens[Opening].Text};
        const std::string Close{Open == "(" ? ")" : (Open == "{" ? "}" : (Open == "[" ? "]" : ">"))};
        int Depth{};

        for (std::size_t Index{Opening}; Index < Tokens.size(); ++Index) {
            if (Tokens[Index].Text == Open) {
                ++Depth;
            } else if (Tokens[Index].Text == Close && --Depth == 0) {
                return Index;
            }
        }

        Fail(Opening, "Unterminated '" + Open + "'");
    }

    bool FSourceFile::Is(std::size_t Index, std::string_view Value) const {
        return Index < Tokens.size() && Tokens[Index].Text == Value;
    }

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

    std::string Trim(std::string_view Text) {
        std::size_t First{};
        std::size_t Last{Text.size()};

        while (First < Last && std::isspace(static_cast<unsigned char>(Text[First]))) {
            ++First;
        }

        while (Last > First && std::isspace(static_cast<unsigned char>(Text[Last - 1]))) {
            --Last;
        }

        return std::string{Text.substr(First, Last - First)};
    }

    bool IsIdentifierCharacter(char Character) {
        return std::isalnum(static_cast<unsigned char>(Character)) || Character == '_';
    }

    namespace {

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

    bool ContainsWord(std::string_view Text, std::string_view Word) {
        return FindWord(Text, Word, 0) != std::string_view::npos;
    }

    std::vector<std::string> SplitTopLevel(std::string_view Text) {
        std::vector<std::string> Parts;
        int Depth{};
        std::size_t First{};

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
