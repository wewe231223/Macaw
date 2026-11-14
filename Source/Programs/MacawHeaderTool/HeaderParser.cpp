/**
 * @file HeaderParser.cpp
 * @brief 리플렉션 표시를 파싱하고 선언·포함 순서 규칙을 검사합니다.
 */

#include "Core/Types.h"
#include "HeaderParser.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>

namespace HeaderTool {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {

        /**
         * @brief 허용하는 리플렉션 표시 이름입니다.
         */
        constexpr std::string_view sAnnotations[]{"UCLASS", "USTRUCT", "UENUM", "UPROPERTY", "UFUNCTION", "GENERATED_BODY"};

        /**
         * @brief 교체해야 하는 이전 리플렉션 매크로 이름입니다.
         */
        constexpr std::string_view sLegacyAnnotations[]{"ReflectTypes", "ReflectClass", "ReflectRootClass", "ReflectStruct", "ReflectMembers", "ReflectProperty", "ReflectAccessor", "ReflectSetter", "ReflectReadOnlyProperty", "ReflectReadOnlyAccessor", "ReflectArray", "ReflectArrayAccessor", "ReflectFunction", "ReflectEnum", "ReflectScriptEnum"};

        /**
         * @brief 허용하는 프로퍼티 플래그 이름입니다.
         */
        constexpr std::string_view sPropertyFlags[]{"Edit", "Serialize", "ReadOnly", "Transient", "SerializeLate"};

        /**
         * @brief 허용하는 클래스 플래그 이름입니다.
         */
        constexpr std::string_view sClassFlags[]{"Abstract", "NoFactory"};

        /**
         * @brief 고정 문자열 배열에 지정한 값이 있는지 검사합니다.
         * @tparam Count 고정 배열의 원소 수입니다.
         * @param[in] Values 검사하거나 ABI에 저장할 배열입니다.
         * @param[in] Value 검사·변환·저장할 값입니다.
         * @return 배열에 일치하는 문자열이 있으면 true입니다.
         */
        template <std::size_t Count>
        bool Contains(const std::string_view (&Values)[Count], std::string_view Value) {
            return std::find(std::begin(Values), std::end(Values), Value) != std::end(Values);
        }

        /**
         * @brief 네임스페이스 이름과 본문 토큰 범위입니다.
         */
        struct FNamespace {
            /**
             * @brief 본문 범위를 기록한 네임스페이스의 이름입니다.
             */
            std::string Name;

            /**
             * @brief 네임스페이스 본문의 여는 중괄호 토큰 위치입니다.
             */
            Core::Index_t Opening{};

            /**
             * @brief 본문 또는 인자의 닫는 토큰 위치입니다.
             */
            Core::Index_t Closing{};
        };

        /**
         * @brief 리플렉션 지정자의 이름과 선택적 값입니다.
         */
        struct FArgument {
            /**
             * @brief 리플렉션 지정자 이름입니다.
             */
            std::string Key;

            /**
             * @brief 등호 뒤의 원문 지정자 값입니다.
             */
            std::string Value;
        };

        /**
         * @brief 리플렉션 표시의 이름·토큰 범위·원문 인자입니다.
         */
        struct FAnnotation {
            /**
             * @brief UCLASS·UPROPERTY 등 리플렉션 표시의 이름입니다.
             */
            std::string Name;

            /**
             * @brief 표시 이름의 토큰 위치입니다.
             */
            Core::Index_t Token{};

            /**
             * @brief 본문 또는 인자의 닫는 토큰 위치입니다.
             */
            Core::Index_t Closing{};

            /**
             * @brief 표시의 괄호 안에서 분리한 원문 인자입니다.
             */
            std::vector<std::string> Arguments;
        };

        /**
         * @brief 범위 안의 토큰이 식별자인지 검사합니다.
         * @param[in] Source 원문·마스크·토큰을 제공하는 소스입니다.
         * @param[in] Index 처리할 원소 또는 토큰의 인덱스입니다.
         * @return 범위 안의 토큰이 식별자이면 true입니다.
         */
        bool IsIdentifier(const FSourceFile& Source, Core::Index_t Index) {
            return Index < Source.Tokens.size() && Source.Tokens[Index].Kind == ETokenKind::Identifier;
        }

        // 괄호 안을 깊이 0의 쉼표로 나누어 원문 조각을 돌려줍니다.

        /**
         * @brief 괄호 내부를 중첩 깊이 0의 쉼표로 나누어 원문 인자를 수집합니다.
         * @param[in] Source 원문·마스크·토큰을 제공하는 소스입니다.
         * @param[in] Opening 여는 괄호 토큰 위치입니다.
         * @param[in] Closing 닫는 괄호 토큰 위치입니다.
         * @return 공백을 정리한 원문 인자 목록입니다.
         */
        std::vector<std::string> ReadArguments(const FSourceFile& Source, Core::Index_t Opening, Core::Index_t Closing) {
            std::vector<std::string> Arguments;
            Core::Index_t First{static_cast<Core::Index_t>(Opening + 1)};
            int Depth{};

            for (Core::Index_t Index{static_cast<Core::Index_t>(Opening + 1)}; Index <= Closing; ++Index) {
                const std::string& Text{Source.Tokens[Index].Text};

                if (Text == "(" || Text == "{" || Text == "[") {
                    ++Depth;
                } else if ((Text == ")" || Text == "}" || Text == "]") && Index != Closing) {
                    --Depth;
                }

                if (Index == Closing || (Text == "," && Depth == 0)) {
                    std::string Argument{Source.TextBetween(First, Index)};

                    if (!Argument.empty()) {
                        Arguments.push_back(std::move(Argument));
                    }

                    First = Index + 1;
                }
            }

            return Arguments;
        }

        /**
         * @brief 리플렉션 지정자를 첫 등호 기준으로 이름과 값으로 나눕니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @return 지정자 이름과 선택적인 값입니다.
         */
        FArgument SplitSpecifier(const std::string& Text) {
            const std::size_t Equal{Text.find('=')};

            if (Equal == std::string::npos) {
                return {Trim(Text), {}};
            }

            return {Trim(std::string_view{Text}.substr(0, Equal)), Trim(std::string_view{Text}.substr(Equal + 1))};
        }

        /**
         * @brief 타입의 const·마지막 참조 기호를 제거하고 공백을 정리합니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string NormalizeType(std::string_view Text) {
            std::string Type{Trim(RemoveWord(Text, "const"))};

            if (!Type.empty() && Type.back() == '&') {
                Type.pop_back();
            }

            return Trim(Type);
        }

        /**
         * @brief 텍스트에서 블록·줄 주석을 제거합니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string StripComments(std::string Text) {
            std::size_t Position{};

            while ((Position = Text.find("/*", Position)) != std::string::npos) {
                const std::size_t Finish{Text.find("*/", Position + 2)};

                Text.erase(Position, Finish == std::string::npos ? std::string::npos : Finish + 2 - Position);
            }

            Position = 0;

            while ((Position = Text.find("//", Position)) != std::string::npos) {
                Text.erase(Position, Text.find('\n', Position) - Position);
            }

            return Text;
        }

        /**
         * @brief 열거형 값에 붙은 UMETA 표시와 인자를 제거합니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string StripMeta(std::string Text) {
            std::size_t Position{};

            while ((Position = Text.find("UMETA", Position)) != std::string::npos) {
                Core::Index_t Last{static_cast<Core::Index_t>(Text.find('(', Position))};
                int Depth{};

                while (Last < Text.size()) {
                    Depth += Text[Last] == '(' ? 1 : (Text[Last] == ')' ? -1 : 0);
                    ++Last;

                    if (Depth == 0) {
                        break;
                    }
                }

                Text.erase(Position, Last - Position);
            }

            return Trim(Text);
        }

        /**
         * @brief 식별자에 사용할 수 없는 문자를 밑줄로 바꿉니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string Sanitize(std::string Text) {
            for (char& Character : Text) {
                if (!IsIdentifierCharacter(Character)) {
                    Character = '_';
                }
            }

            return Text;
        }

        /**
         * @brief 필드의 m 접두어를 제거해 기본 프로퍼티 이름을 구합니다.
         * @param[in] Member 프로퍼티 이름을 유도할 필드 이름입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string DerivedName(const std::string& Member) {
            if (Member.size() > 1 && Member[0] == 'm' && std::isupper(static_cast<unsigned char>(Member[1]))) {
                return Member.substr(1);
            }

            return Member;
        }

        /**
         * @brief 헤더의 리플렉션 표시를 읽어 타입·프로퍼티·함수·열거형 정보를 구성합니다.
         */
        class FHeaderParser {
        public:
            /**
             * @brief 파싱할 헤더와 원본 소스·토큰을 연결합니다.
             * @param[in] Header 파싱하거나 생성에 참조할 헤더입니다.
             */
            explicit FHeaderParser(FHeader& Header)
                : mHeader{Header}, mSource{Header.Source} {
            }

            /**
             * @brief 타입과 리플렉션 표시를 읽고 generated.h 포함 규칙을 검증합니다.
             */
            void Parse() {
                ReadNamespaces();
                ReadDefinitions();

                for (const FAnnotation& Annotation : ReadAnnotations()) {
                    if (Annotation.Name == "UCLASS" || Annotation.Name == "USTRUCT") {
                        ReadType(Annotation);
                    } else if (Annotation.Name == "GENERATED_BODY") {
                        ReadGeneratedBody(Annotation);
                    } else if (Annotation.Name == "UPROPERTY") {
                        ReadProperty(Annotation);
                    } else if (Annotation.Name == "UFUNCTION") {
                        ReadFunction(Annotation);
                    } else {
                        ReadEnum(Annotation);
                    }
                }

                for (const FType& Type : mHeader.Types) {
                    if (Type.Kind != ETypeKind::None) {
                        if (Type.GeneratedBodyLine == 0) {
                            mSource.Fail(Type.Opening, Quote(Type.Name) + " has no GENERATED_BODY()");
                        }

                        mHeader.NeedsGeneratedHeader = true;
                    }
                }

                for (const FEnum& Enum : mHeader.Enums) {
                    mHeader.NeedsGeneratedHeader = mHeader.NeedsGeneratedHeader || Enum.Reflect;
                }

                const std::string Stem{std::filesystem::path{mSource.Path}.stem().string()};

                mHeader.GeneratedName = Stem + ".generated.h";
                mHeader.FileId = "MACAW_" + Sanitize(mSource.Module) + "_" + Sanitize(Stem) + "_h";

                CheckGeneratedInclude();
            }

        private:
            /**
             * @brief 진단에 넣을 이름을 작은따옴표로 감쌉니다.
             * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
             * @return 해석·변환한 문자열입니다.
             */
            static std::string Quote(const std::string& Text) {
                return "'" + Text + "'";
            }

            /**
             * @brief 이름이 있는 네임스페이스와 토큰 범위를 수집합니다.
             */
            void ReadNamespaces() {
                for (Core::Index_t Index{}; Index < mSource.Tokens.size(); ++Index) {
                    if (!mSource.Is(Index, "namespace") || !IsIdentifier(mSource, Index + 1)) {
                        continue;
                    }

                    std::string Name{mSource.Tokens[Index + 1].Text};
                    Core::Index_t Opening{static_cast<Core::Index_t>(Index + 2)};

                    while (mSource.Is(Opening, "::") && IsIdentifier(mSource, Opening + 1)) {
                        Name += "::" + mSource.Tokens[Opening + 1].Text;
                        Opening += 2;
                    }

                    if (mSource.Is(Opening, "{")) {
                        mNamespaces.push_back({Name, Opening, mSource.FindClosing(Opening)});
                    }
                }
            }

            /**
             * @brief 토큰 위치를 포함하는 중첩 네임스페이스 이름을 조합합니다.
             * @param[in] Index 처리할 원소 또는 토큰의 인덱스입니다.
             * @return 해석·변환한 문자열입니다.
             */
            std::string NamespaceAt(Core::Index_t Index) const {
                std::string Result;

                for (const FNamespace& Namespace : mNamespaces) {
                    if (Index > Namespace.Opening && Index < Namespace.Closing) {
                        Result += (Result.empty() ? "" : "::") + Namespace.Name;
                    }
                }

                return Result;
            }

            /**
             * @brief 타입 키워드 앞에 템플릿 매개변수 목록이 있는지 검사합니다.
             * @param[in] Keyword 템플릿 여부를 검사할 타입 키워드 위치입니다.
             * @return 템플릿 매개변수 목록이 앞에 있으면 true입니다.
             */
            bool IsTemplateHead(Core::Index_t Keyword) const {
                if (Keyword == 0 || !mSource.Is(Keyword - 1, ">")) {
                    return false;
                }

                int Depth{};

                for (Core::Index_t Index{static_cast<Core::Index_t>(Keyword - 1)}; Index > 0; --Index) {
                    if (mSource.Is(Index, ">")) {
                        ++Depth;
                    } else if (mSource.Is(Index, "<") && --Depth == 0) {
                        return mSource.Is(Index - 1, "template");
                    }
                }

                return false;
            }

            /**
             * @brief 클래스·구조체 정의와 본문 범위를 수집합니다.
             */
            void ReadDefinitions() {
                const std::vector<FToken>& Tokens{mSource.Tokens};

                for (Core::Index_t Index{}; Index < Tokens.size(); ++Index) {
                    if ((!mSource.Is(Index, "class") && !mSource.Is(Index, "struct")) || (Index > 0 && mSource.Is(Index - 1, "enum"))) {
                        continue;
                    }

                    Core::Index_t NameIndex{static_cast<Core::Index_t>(Index + 1)};

                    if (mSource.Is(NameIndex, "alignas") && mSource.Is(NameIndex + 1, "(")) {
                        NameIndex = mSource.FindClosing(NameIndex + 1) + 1;
                    }

                    if (!IsIdentifier(mSource, NameIndex)) {
                        continue;
                    }

                    Core::Index_t Opening{static_cast<Core::Index_t>(NameIndex + 1)};

                    while (Opening < Tokens.size() && !mSource.Is(Opening, "{") && !mSource.Is(Opening, ";")) {
                        ++Opening;
                    }

                    if (!mSource.Is(Opening, "{")) {
                        continue;
                    }

                    FType Type;
                    Type.ShortName = Tokens[NameIndex].Text;
                    Type.Keyword = Tokens[Index].Text;
                    Type.Namespace = NamespaceAt(Index);
                    Type.Opening = Opening;
                    Type.Closing = mSource.FindClosing(Opening);
                    Type.File = mSource.Path;
                    Type.SourceModule = mSource.Module;

                    for (Core::Index_t Outer{}; Outer < mHeader.Types.size(); ++Outer) {
                        if (Opening > mHeader.Types[Outer].Opening && Opening < mHeader.Types[Outer].Closing) {
                            Type.Outer = static_cast<Core::Int32>(Outer);
                        }
                    }

                    const FType* Outer{Type.Outer >= 0 ? &mHeader.Types[static_cast<Core::Index_t>(Type.Outer)] : nullptr};

                    Type.Name = Outer != nullptr ? Outer->Name + "::" + Type.ShortName : Type.ShortName;
                    Type.Template = IsTemplateHead(Index) || (Outer != nullptr && Outer->Template);
                    Type.Super = ReadSuper(NameIndex + 1, Opening);

                    mHeader.Types.push_back(std::move(Type));
                    mKeywords.push_back(Index);
                }
            }

            /**
             * @brief 타입 선언에서 지원하는 단일 기반 클래스 이름을 읽습니다.
             * @param[in] Index 처리할 원소 또는 토큰의 인덱스입니다.
             * @param[in] Opening 여는 괄호 토큰 위치입니다.
             * @return 정규화한 기반 클래스 이름이며 상속이 없으면 빈 문자열입니다.
             */
            std::string ReadSuper(Core::Index_t Index, Core::Index_t Opening) const {
                if (mSource.Is(Index, "final")) {
                    ++Index;
                }

                if (!mSource.Is(Index, ":")) {
                    return {};
                }

                ++Index;

                while (mSource.Is(Index, "public") || mSource.Is(Index, "protected") || mSource.Is(Index, "private") || mSource.Is(Index, "virtual")) {
                    ++Index;
                }

                Core::Index_t Last{Index};
                int Depth{};

                for (; Last < Opening; ++Last) {
                    const std::string& Text{mSource.Tokens[Last].Text};

                    Depth += (Text == "<" || Text == "(") ? 1 : ((Text == ">" || Text == ")") ? -1 : 0);

                    if (Text == "," && Depth == 0) {
                        break;
                    }
                }

                return mSource.TextBetween(Index, Last);
            }

            /**
             * @brief 리플렉션 표시와 인자를 수집하고 이전 매크로 사용을 거부합니다.
             * @return 검증한 리플렉션 표시 목록입니다.
             */
            std::vector<FAnnotation> ReadAnnotations() const {
                std::vector<FAnnotation> Annotations;

                for (Core::Index_t Index{}; Index < mSource.Tokens.size(); ++Index) {
                    const FToken& Token{mSource.Tokens[Index]};

                    if (Token.Kind != ETokenKind::Identifier || !mSource.Is(Index + 1, "(")) {
                        continue;
                    }

                    if (Contains(sLegacyAnnotations, Token.Text)) {
                        mSource.Fail(Index, Token.Text + " is no longer supported; use UCLASS, USTRUCT, UENUM, UPROPERTY, UFUNCTION and GENERATED_BODY");
                    }

                    if (Contains(sAnnotations, Token.Text)) {
                        const Core::Index_t Closing{static_cast<Core::Index_t>(mSource.FindClosing(Index + 1))};

                        Annotations.push_back({Token.Text, Index, Closing, ReadArguments(mSource, Index + 1, Closing)});

                        Index = Closing;
                    }
                }

                return Annotations;
            }

            /**
             * @brief 토큰 위치를 본문에 포함하는 가장 안쪽 타입을 찾습니다.
             * @param[in] Index 처리할 원소 또는 토큰의 인덱스입니다.
             * @return 위치를 포함하는 가장 안쪽 타입이며 없으면 nullptr입니다.
             */
            FType* OwnerAt(Core::Index_t Index) {
                FType* Owner{};

                for (FType& Type : mHeader.Types) {
                    if (Index > Type.Opening && Index < Type.Closing) {
                        Owner = &Type;
                    }
                }

                return Owner;
            }

            /**
             * @brief 표시가 속한 타입을 찾고 리플렉션 대상으로 선언되었는지 검사합니다.
             * @param[in] Annotation 해석할 리플렉션 표시·토큰 범위입니다.
             * @return 리플렉션 표시를 소유한 타입 참조입니다.
             */
            FType& ReflectedOwner(const FAnnotation& Annotation) {
                FType* Owner{OwnerAt(Annotation.Token)};

                if (Owner == nullptr || Owner->Kind == ETypeKind::None) {
                    mSource.Fail(Annotation.Token, Annotation.Name + " must be inside a UCLASS() or USTRUCT() type");
                }

                if (Owner->Template) {
                    mSource.Fail(Annotation.Token, "Template types cannot use reflection");
                }

                return *Owner;
            }

            /**
             * @brief 리플렉션 표시를 타입 정의에 연결하고 종류와 플래그를 기록합니다.
             * @param[in] Annotation 해석할 리플렉션 표시·토큰 범위입니다.
             */
            void ReadType(const FAnnotation& Annotation) {
                const auto Found{std::find(mKeywords.begin(), mKeywords.end(), Annotation.Closing + 1)};

                if (Found == mKeywords.end()) {
                    mSource.Fail(Annotation.Token, Annotation.Name + "() must be followed by a class or struct definition");
                }

                FType& Type{mHeader.Types[static_cast<std::size_t>(Found - mKeywords.begin())]};

                if (Type.Template) {
                    mSource.Fail(Annotation.Token, "Template types cannot use reflection");
                }

                NoteReflected(Annotation);

                if (Annotation.Name == "USTRUCT") {
                    if (!Annotation.Arguments.empty()) {
                        mSource.Fail(Annotation.Token, "USTRUCT() takes no specifiers");
                    }

                    Type.Kind = ETypeKind::Struct;

                    return;
                }

                Type.Kind = Type.Super.empty() ? ETypeKind::RootClass : ETypeKind::Class;

                for (const std::string& Argument : Annotation.Arguments) {
                    if (!Contains(sClassFlags, Argument)) {
                        mSource.Fail(Annotation.Token, "Unknown UCLASS specifier " + Quote(Argument));
                    }

                    Type.ClassFlags.push_back("CoreUObject::EClassFlags::" + Argument);
                }
            }

            /**
             * @brief GENERATED_BODY 위치를 소유 타입에 기록하고 중복을 검사합니다.
             * @param[in] Annotation 해석할 리플렉션 표시·토큰 범위입니다.
             */
            void ReadGeneratedBody(const FAnnotation& Annotation) {
                FType& Owner{ReflectedOwner(Annotation)};

                if (!Annotation.Arguments.empty()) {
                    mSource.Fail(Annotation.Token, "GENERATED_BODY() takes no arguments");
                }

                if (Owner.GeneratedBodyLine != 0) {
                    mSource.Fail(Annotation.Token, Quote(Owner.Name) + " has GENERATED_BODY() twice");
                }

                Owner.GeneratedBodyLine = mSource.LineAt(mSource.Tokens[Annotation.Token].Offset);
            }

            /**
             * @brief 필드와 지정자를 읽어 접근자·직렬화·배열 등록 정보를 구성합니다.
             * @param[in] Annotation 해석할 리플렉션 표시·토큰 범위입니다.
             */
            void ReadProperty(const FAnnotation& Annotation) {
                FType& Owner{ReflectedOwner(Annotation)};
                const Core::Index_t First{static_cast<Core::Index_t>(Annotation.Closing + 1)};
                Core::Index_t Index{First};
                int Angles{};

                for (; Index < mSource.Tokens.size(); ++Index) {
                    const std::string& Text{mSource.Tokens[Index].Text};

                    if (Text == "<") {
                        ++Angles;
                    } else if (Text == ">") {
                        --Angles;
                    } else if (Angles == 0 && (Text == "{" || Text == "=" || Text == "[" || Text == ";")) {
                        break;
                    } else if ((Angles == 0 && Text == "(") || Text == "}" || Contains(sAnnotations, Text)) {
                        mSource.Fail(Annotation.Token, "UPROPERTY() must be followed by a member variable declaration");
                    }
                }

                if (Index == First || !IsIdentifier(mSource, Index - 1)) {
                    mSource.Fail(Annotation.Token, "UPROPERTY() must be followed by a member variable declaration");
                }

                FProperty Property;
                Property.Member = mSource.Tokens[Index - 1].Text;

                const std::string Type{mSource.TextBetween(First, Index - 1)};

                if (Type.empty() || ContainsWord(Type, "static")) {
                    mSource.Fail(Annotation.Token, "UPROPERTY() requires a non-static member variable");
                }

                Property.Array = Type.starts_with("Core::TArray<") || Type.starts_with("TArray<");

                for (const std::string& Argument : Annotation.Arguments) {
                    const FArgument Specifier{SplitSpecifier(Argument)};

                    if (Specifier.Key == "Getter" && !Specifier.Value.empty()) {
                        Property.Getter = Specifier.Value;
                    } else if (Specifier.Key == "Setter" && !Specifier.Value.empty()) {
                        Property.Setter = Specifier.Value;
                    } else if (Specifier.Key == "Name" && Specifier.Value.size() > 1 && Specifier.Value.front() == '"' && Specifier.Value.back() == '"') {
                        Property.Name = Specifier.Value;
                    } else if (Specifier.Key == "Array" && Specifier.Value.empty()) {
                        Property.Array = true;
                    } else if (Specifier.Value.empty() && Contains(sPropertyFlags, Specifier.Key)) {
                        Property.Flags.push_back(Specifier.Key);
                    } else {
                        mSource.Fail(Annotation.Token, "Unknown UPROPERTY specifier " + Quote(Argument));
                    }
                }

                if (!Property.Getter.empty() && Property.Setter.empty()) {
                    mSource.Fail(Annotation.Token, "UPROPERTY Getter requires a Setter");
                }

                if (Property.Array && Property.Getter.empty() && !Property.Setter.empty()) {
                    mSource.Fail(Annotation.Token, "Array UPROPERTY Setter requires a Getter");
                }

                if (Property.Name.empty()) {
                    Property.Name = "\"" + DerivedName(Property.Member) + "\"";
                }

                for (const FProperty& Existing : Owner.Properties) {
                    if (Existing.Member == Property.Member) {
                        mSource.Fail(Annotation.Token, "Duplicate reflected member " + Quote(Property.Member));
                    }
                }

                Owner.Properties.push_back(std::move(Property));
            }

            /**
             * @brief 노출 함수의 타입·매개변수·지정자를 읽고 바인딩 제약을 검사합니다.
             * @param[in] Annotation 해석할 리플렉션 표시·토큰 범위입니다.
             */
            void ReadFunction(const FAnnotation& Annotation) {
                const FType* Owner{OwnerAt(Annotation.Token)};

                if (Owner == nullptr) {
                    mSource.Fail(Annotation.Token, "UFUNCTION() must be inside a class or struct");
                }

                if (Owner->Template) {
                    mSource.Fail(Annotation.Token, "Template types cannot use reflection");
                }

                const Core::Index_t First{static_cast<Core::Index_t>(Annotation.Closing + 1)};
                Core::Index_t Opening{First};
                int Angles{};

                for (; Opening < mSource.Tokens.size(); ++Opening) {
                    const std::string& Text{mSource.Tokens[Opening].Text};

                    Angles += Text == "<" ? 1 : (Text == ">" ? -1 : 0);

                    if (Angles == 0 && Text == "(") {
                        break;
                    }

                    if (Text == ";" || Text == "{" || Text == "}" || Contains(sAnnotations, Text)) {
                        mSource.Fail(Annotation.Token, "UFUNCTION() must be followed by a member function declaration");
                    }
                }

                if (Opening == First || !IsIdentifier(mSource, Opening - 1) || mSource.Is(Opening - 1, "operator")) {
                    mSource.Fail(Annotation.Token, "UFUNCTION() must be followed by a member function declaration");
                }

                FFunction Function;
                Function.Name = mSource.Tokens[Opening - 1].Text;
                Function.Owner = Owner;
                Function.File = mSource.Path;
                Function.Label = "UFUNCTION " + Function.Name;

                for (const std::string& Argument : Annotation.Arguments) {
                    if (Argument != "Internal") {
                        mSource.Fail(Annotation.Token, "Unknown UFUNCTION specifier " + Quote(Argument));
                    }

                    Function.Internal = true;
                }

                std::string Return{mSource.MaskBetween(First, Opening - 1)};

                Function.Static = ContainsWord(Return, "static");

                for (std::string_view Keyword : {"static", "virtual", "inline", "explicit"}) {
                    Return = RemoveWord(Return, Keyword);
                }

                Function.Return = NormalizeType(Return);

                const Core::Index_t Closing{static_cast<Core::Index_t>(mSource.FindClosing(Opening))};
                const std::size_t Begin{mSource.Tokens[Opening].Offset + 1};
                const std::string Parameters{mSource.Mask.substr(Begin, mSource.Tokens[Closing].Offset - Begin)};

                for (const std::string& Part : SplitTopLevel(Parameters)) {
                    const std::string Parameter{Trim(std::string_view{Part}.substr(0, Part.find('=')))};
                    std::size_t NameStart{Parameter.size()};

                    while (NameStart > 0 && IsIdentifierCharacter(Parameter[NameStart - 1])) {
                        --NameStart;
                    }

                    std::size_t TypeEnd{NameStart};

                    while (TypeEnd > 0 && std::isspace(static_cast<unsigned char>(Parameter[TypeEnd - 1]))) {
                        --TypeEnd;
                    }

                    if (NameStart == Parameter.size() || std::isdigit(static_cast<unsigned char>(Parameter[NameStart])) || TypeEnd == NameStart || TypeEnd == 0) {
                        mSource.Fail(Annotation.Token, Function.Label + ": Unnamed parameter " + Trim(Part));
                    }

                    Function.Parameters.push_back({NormalizeType(std::string_view{Parameter}.substr(0, TypeEnd)), Parameter.substr(NameStart)});
                }

                mHeader.Functions.push_back(std::move(Function));
            }

            /**
             * @brief 열거형 표시와 값을 읽어 네이티브·C# 노출 정보를 구성합니다.
             * @param[in] Annotation 해석할 리플렉션 표시·토큰 범위입니다.
             */
            void ReadEnum(const FAnnotation& Annotation) {
                if (OwnerAt(Annotation.Token) != nullptr) {
                    mSource.Fail(Annotation.Token, "UENUM() must be at namespace scope");
                }

                FEnum Enum;
                Enum.File = mSource.Path;
                Enum.Namespace = NamespaceAt(Annotation.Token);

                if (Annotation.Arguments.empty()) {
                    Enum.Reflect = true;
                } else if (Annotation.Arguments.size() == 1 && Annotation.Arguments[0] == "Script") {
                    Enum.Reflect = true;
                    Enum.Script = true;
                } else if (Annotation.Arguments.size() == 1 && Annotation.Arguments[0] == "ScriptOnly") {
                    Enum.Script = true;
                } else {
                    mSource.Fail(Annotation.Token, "UENUM() accepts only Script or ScriptOnly");
                }

                if (Enum.Reflect) {
                    NoteReflected(Annotation);
                }

                Core::Index_t Index{static_cast<Core::Index_t>(Annotation.Closing + 1)};

                if (!mSource.Is(Index, "enum")) {
                    mSource.Fail(Annotation.Token, "UENUM() must be followed by an enum definition");
                }

                ++Index;
                Enum.Scoped = mSource.Is(Index, "class") || mSource.Is(Index, "struct");
                Index += Enum.Scoped ? 1 : 0;

                if (!IsIdentifier(mSource, Index)) {
                    mSource.Fail(Annotation.Token, "UENUM() requires a named enum");
                }

                Enum.Name = mSource.Tokens[Index].Text;

                Core::Index_t Opening{static_cast<Core::Index_t>(Index + 1)};

                if (mSource.Is(Opening, ":")) {
                    while (Opening < mSource.Tokens.size() && !mSource.Is(Opening, "{") && !mSource.Is(Opening, ";")) {
                        ++Opening;
                    }

                    Enum.Base = mSource.TextBetween(Index + 2, Opening);
                }

                if (!mSource.Is(Opening, "{")) {
                    mSource.Fail(Annotation.Token, "UENUM() must be followed by an enum definition");
                }

                if (Enum.Reflect && !Enum.Scoped && Enum.Base.empty()) {
                    mSource.Fail(Annotation.Token, "Reflected unscoped enums need an underlying type so they can be forward declared");
                }

                const Core::Index_t Closing{static_cast<Core::Index_t>(mSource.FindClosing(Opening))};
                std::set<std::string> Seen;
                Core::Index_t First{static_cast<Core::Index_t>(Opening + 1)};
                int Depth{};

                for (Core::Index_t Entry{static_cast<Core::Index_t>(Opening + 1)}; Entry <= Closing; ++Entry) {
                    const std::string& Text{mSource.Tokens[Entry].Text};

                    Depth += Text == "(" ? 1 : (Text == ")" ? -1 : 0);

                    if (Entry != Closing && (Text != "," || Depth != 0)) {
                        continue;
                    }

                    if (Entry > First) {
                        if (!IsIdentifier(mSource, First)) {
                            mSource.Fail(First, "Invalid enum value");
                        }

                        bool Hidden{};

                        for (Core::Index_t Meta{First}; Meta < Entry; ++Meta) {
                            if (mSource.Is(Meta, "UMETA") && mSource.Is(Meta + 1, "(")) {
                                const Core::Index_t MetaClosing{static_cast<Core::Index_t>(mSource.FindClosing(Meta + 1))};

                                for (const std::string& Argument : ReadArguments(mSource, Meta + 1, MetaClosing)) {
                                    if (Argument != "Hidden") {
                                        mSource.Fail(Meta, "UMETA() accepts only Hidden");
                                    }

                                    Hidden = true;
                                }
                            }
                        }

                        const std::string& Value{mSource.Tokens[First].Text};

                        if (!Seen.insert(Value).second) {
                            mSource.Fail(First, "Duplicate enum value " + Quote(Value));
                        }

                        if (!Hidden) {
                            Enum.Values.push_back(Value);
                        }
                    }

                    First = Entry + 1;
                }

                const std::size_t Begin{mSource.Tokens[Opening].Offset + 1};
                const std::string Body{StripComments(mSource.Text.substr(Begin, mSource.Tokens[Closing].Offset - Begin))};

                for (const std::string& Part : SplitTopLevel(Body)) {
                    Enum.ScriptValues.push_back(StripMeta(Trim(Part)));
                }

                mHeader.Enums.push_back(std::move(Enum));
            }

            // 뒤에 오는 헤더의 generated.h가 CURRENT_FILE_ID를 덮어쓰므로 첫 리플렉션 타입 앞의 마지막 #include여야 합니다.

            /**
             * @brief 첫 리플렉션 타입 앞의 마지막 include가 자기 generated.h인지 검증합니다.
             */
            void CheckGeneratedInclude() const {
                std::string Last;
                int LastLine{};
                std::size_t Position{};
                int Line{1};

                while (Position < mSource.Text.size()) {
                    std::size_t End{mSource.Text.find('\n', Position)};

                    End = End == std::string::npos ? mSource.Text.size() : End;

                    const std::string Text{Trim(std::string_view{mSource.Text}.substr(Position, End - Position))};
                    const std::string Directive{Text.starts_with("#") ? Trim(std::string_view{Text}.substr(1)) : std::string{}};
                    const std::string Target{Directive.starts_with("include") ? Trim(std::string_view{Directive}.substr(7)) : std::string{}};

                    if (Target.size() > 2) {
                        const std::string Name{std::filesystem::path{Target.substr(1, Target.size() - 2)}.filename().string()};

                        if (!mHeader.NeedsGeneratedHeader && Name.ends_with(".generated.h")) {
                            throw FHeaderToolError{mSource.Path, Line, Name + " is included but nothing in this header is reflected"};
                        }

                        if (mFirstReflectedLine == 0 || Line < mFirstReflectedLine) {
                            Last = Name;
                            LastLine = Line;
                        }
                    }

                    Position = End + 1;
                    ++Line;
                }

                if (mHeader.NeedsGeneratedHeader && Last != mHeader.GeneratedName) {
                    throw FHeaderToolError{mSource.Path, LastLine, "The last #include before the first reflected type must be \"" + mHeader.GeneratedName + "\""};
                }
            }

            /**
             * @brief 첫 리플렉션 표시의 줄 번호를 기록합니다.
             * @param[in] Annotation 해석할 리플렉션 표시·토큰 범위입니다.
             */
            void NoteReflected(const FAnnotation& Annotation) {
                const int Line{mSource.LineAt(mSource.Tokens[Annotation.Token].Offset)};

                if (mFirstReflectedLine == 0 || Line < mFirstReflectedLine) {
                    mFirstReflectedLine = Line;
                }
            }

            /**
             * @brief 파싱 결과를 기록할 헤더 참조입니다.
             */
            FHeader& mHeader;

            /**
             * @brief 파싱할 원문·마스크·토큰의 참조입니다.
             */
            const FSourceFile& mSource;

            /**
             * @brief 네임스페이스 이름·토큰 범위 목록입니다.
             */
            std::vector<FNamespace> mNamespaces;

            /**
             * @brief 클래스·구조체 시작 키워드의 토큰 위치입니다.
             */
            std::vector<Core::Index_t> mKeywords;

            /**
             * @brief 첫 리플렉션 표시의 줄 번호입니다.
             */
            int mFirstReflectedLine{};
        };

    }

    /**
     * @brief 헤더를 파싱합니다.
     */
    std::unique_ptr<FHeader> ParseHeader(FSourceFile Source) {
        auto Header{std::make_unique<FHeader>()};
        Header->Source = std::move(Source);

        FHeaderParser{*Header}.Parse();

        return Header;
    }

    /**
     * @brief 소스 파일에 리플렉션 어노테이션이 없는지 검사합니다.
     */
    void CheckSourceFile(const FSourceFile& Source) {
        for (Core::Index_t Index{}; Index < Source.Tokens.size(); ++Index) {
            const FToken& Token{Source.Tokens[Index]};

            if (Token.Kind != ETokenKind::Identifier || !Source.Is(Index + 1, "(")) {
                continue;
            }

            if (Contains(sAnnotations, Token.Text) || Contains(sLegacyAnnotations, Token.Text)) {
                Source.Fail(Index, "Move reflection annotations into headers: " + Token.Text);
            }

            if (Token.Text.starts_with("Implement") && Token.Text.ends_with("Reflection")) {
                Source.Fail(Index, "Reflection implementations must be generated from header annotations");
            }
        }
    }

}
