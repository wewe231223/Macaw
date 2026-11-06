#include "HeaderParser.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>

namespace HeaderTool {

    namespace {

        constexpr std::string_view sAnnotations[]{"UCLASS", "USTRUCT", "UENUM", "UPROPERTY", "UFUNCTION", "GENERATED_BODY"};
        constexpr std::string_view sLegacyAnnotations[]{"ReflectTypes", "ReflectClass", "ReflectRootClass", "ReflectStruct", "ReflectMembers", "ReflectProperty", "ReflectAccessor", "ReflectSetter", "ReflectReadOnlyProperty", "ReflectReadOnlyAccessor", "ReflectArray", "ReflectArrayAccessor", "ReflectFunction", "ReflectEnum", "ReflectScriptEnum"};
        constexpr std::string_view sPropertyFlags[]{"Edit", "Serialize", "ReadOnly", "Transient", "SerializeLate"};
        constexpr std::string_view sClassFlags[]{"Abstract", "NoFactory"};

        template <std::size_t Count>
        bool Contains(const std::string_view (&Values)[Count], std::string_view Value) {
            return std::find(std::begin(Values), std::end(Values), Value) != std::end(Values);
        }

        struct FNamespace {
            std::string Name;
            std::size_t Opening{};
            std::size_t Closing{};
        };

        struct FArgument {
            std::string Key;
            std::string Value;
        };

        struct FAnnotation {
            std::string Name;
            std::size_t Token{};
            std::size_t Closing{};
            std::vector<std::string> Arguments;
        };

        bool IsIdentifier(const FSourceFile& Source, std::size_t Index) {
            return Index < Source.Tokens.size() && Source.Tokens[Index].Kind == ETokenKind::Identifier;
        }

        // 괄호 안을 깊이 0의 쉼표로 나누어 원문 조각을 돌려줍니다.
        std::vector<std::string> ReadArguments(const FSourceFile& Source, std::size_t Opening, std::size_t Closing) {
            std::vector<std::string> Arguments;
            std::size_t First{Opening + 1};
            int Depth{};

            for (std::size_t Index{Opening + 1}; Index <= Closing; ++Index) {
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

        FArgument SplitSpecifier(const std::string& Text) {
            const std::size_t Equal{Text.find('=')};

            if (Equal == std::string::npos) {
                return {Trim(Text), {}};
            }

            return {Trim(std::string_view{Text}.substr(0, Equal)), Trim(std::string_view{Text}.substr(Equal + 1))};
        }

        std::string NormalizeType(std::string_view Text) {
            std::string Type{Trim(RemoveWord(Text, "const"))};

            if (!Type.empty() && Type.back() == '&') {
                Type.pop_back();
            }

            return Trim(Type);
        }

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

        std::string StripMeta(std::string Text) {
            std::size_t Position{};

            while ((Position = Text.find("UMETA", Position)) != std::string::npos) {
                std::size_t Last{Text.find('(', Position)};
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

        std::string Sanitize(std::string Text) {
            for (char& Character : Text) {
                if (!IsIdentifierCharacter(Character)) {
                    Character = '_';
                }
            }

            return Text;
        }

        std::string DerivedName(const std::string& Member) {
            if (Member.size() > 1 && Member[0] == 'm' && std::isupper(static_cast<unsigned char>(Member[1]))) {
                return Member.substr(1);
            }

            return Member;
        }

        class FHeaderParser {
        public:
            explicit FHeaderParser(FHeader& Header)
                : mHeader{Header}, mSource{Header.Source} {
            }

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
            static std::string Quote(const std::string& Text) {
                return "'" + Text + "'";
            }

            void ReadNamespaces() {
                for (std::size_t Index{}; Index < mSource.Tokens.size(); ++Index) {
                    if (!mSource.Is(Index, "namespace") || !IsIdentifier(mSource, Index + 1)) {
                        continue;
                    }

                    std::string Name{mSource.Tokens[Index + 1].Text};
                    std::size_t Opening{Index + 2};

                    while (mSource.Is(Opening, "::") && IsIdentifier(mSource, Opening + 1)) {
                        Name += "::" + mSource.Tokens[Opening + 1].Text;
                        Opening += 2;
                    }

                    if (mSource.Is(Opening, "{")) {
                        mNamespaces.push_back({Name, Opening, mSource.FindClosing(Opening)});
                    }
                }
            }

            std::string NamespaceAt(std::size_t Index) const {
                std::string Result;

                for (const FNamespace& Namespace : mNamespaces) {
                    if (Index > Namespace.Opening && Index < Namespace.Closing) {
                        Result += (Result.empty() ? "" : "::") + Namespace.Name;
                    }
                }

                return Result;
            }

            bool IsTemplateHead(std::size_t Keyword) const {
                if (Keyword == 0 || !mSource.Is(Keyword - 1, ">")) {
                    return false;
                }

                int Depth{};

                for (std::size_t Index{Keyword - 1}; Index > 0; --Index) {
                    if (mSource.Is(Index, ">")) {
                        ++Depth;
                    } else if (mSource.Is(Index, "<") && --Depth == 0) {
                        return mSource.Is(Index - 1, "template");
                    }
                }

                return false;
            }

            void ReadDefinitions() {
                const std::vector<FToken>& Tokens{mSource.Tokens};

                for (std::size_t Index{}; Index < Tokens.size(); ++Index) {
                    if ((!mSource.Is(Index, "class") && !mSource.Is(Index, "struct")) || (Index > 0 && mSource.Is(Index - 1, "enum"))) {
                        continue;
                    }

                    std::size_t NameIndex{Index + 1};

                    if (mSource.Is(NameIndex, "alignas") && mSource.Is(NameIndex + 1, "(")) {
                        NameIndex = mSource.FindClosing(NameIndex + 1) + 1;
                    }

                    if (!IsIdentifier(mSource, NameIndex)) {
                        continue;
                    }

                    std::size_t Opening{NameIndex + 1};

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

                    for (std::size_t Outer{}; Outer < mHeader.Types.size(); ++Outer) {
                        if (Opening > mHeader.Types[Outer].Opening && Opening < mHeader.Types[Outer].Closing) {
                            Type.Outer = static_cast<int>(Outer);
                        }
                    }

                    const FType* Outer{Type.Outer >= 0 ? &mHeader.Types[static_cast<std::size_t>(Type.Outer)] : nullptr};
                    Type.Name = Outer != nullptr ? Outer->Name + "::" + Type.ShortName : Type.ShortName;
                    Type.Template = IsTemplateHead(Index) || (Outer != nullptr && Outer->Template);
                    Type.Super = ReadSuper(NameIndex + 1, Opening);
                    mHeader.Types.push_back(std::move(Type));
                    mKeywords.push_back(Index);
                }
            }

            std::string ReadSuper(std::size_t Index, std::size_t Opening) const {
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

                std::size_t Last{Index};
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

            std::vector<FAnnotation> ReadAnnotations() const {
                std::vector<FAnnotation> Annotations;

                for (std::size_t Index{}; Index < mSource.Tokens.size(); ++Index) {
                    const FToken& Token{mSource.Tokens[Index]};

                    if (Token.Kind != ETokenKind::Identifier || !mSource.Is(Index + 1, "(")) {
                        continue;
                    }

                    if (Contains(sLegacyAnnotations, Token.Text)) {
                        mSource.Fail(Index, Token.Text + " is no longer supported; use UCLASS, USTRUCT, UENUM, UPROPERTY, UFUNCTION and GENERATED_BODY");
                    }

                    if (Contains(sAnnotations, Token.Text)) {
                        const std::size_t Closing{mSource.FindClosing(Index + 1)};
                        Annotations.push_back({Token.Text, Index, Closing, ReadArguments(mSource, Index + 1, Closing)});
                        Index = Closing;
                    }
                }

                return Annotations;
            }

            FType* OwnerAt(std::size_t Index) {
                FType* Owner{};

                for (FType& Type : mHeader.Types) {
                    if (Index > Type.Opening && Index < Type.Closing) {
                        Owner = &Type;
                    }
                }

                return Owner;
            }

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

            void ReadProperty(const FAnnotation& Annotation) {
                FType& Owner{ReflectedOwner(Annotation)};
                const std::size_t First{Annotation.Closing + 1};
                std::size_t Index{First};
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

            void ReadFunction(const FAnnotation& Annotation) {
                const FType* Owner{OwnerAt(Annotation.Token)};

                if (Owner == nullptr) {
                    mSource.Fail(Annotation.Token, "UFUNCTION() must be inside a class or struct");
                }

                if (Owner->Template) {
                    mSource.Fail(Annotation.Token, "Template types cannot use reflection");
                }

                const std::size_t First{Annotation.Closing + 1};
                std::size_t Opening{First};
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

                const std::size_t Closing{mSource.FindClosing(Opening)};
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

                std::size_t Index{Annotation.Closing + 1};

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
                std::size_t Opening{Index + 1};

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

                const std::size_t Closing{mSource.FindClosing(Opening)};
                std::set<std::string> Seen;
                std::size_t First{Opening + 1};
                int Depth{};

                for (std::size_t Entry{Opening + 1}; Entry <= Closing; ++Entry) {
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

                        for (std::size_t Meta{First}; Meta < Entry; ++Meta) {
                            if (mSource.Is(Meta, "UMETA") && mSource.Is(Meta + 1, "(")) {
                                const std::size_t MetaClosing{mSource.FindClosing(Meta + 1)};

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

            void NoteReflected(const FAnnotation& Annotation) {
                const int Line{mSource.LineAt(mSource.Tokens[Annotation.Token].Offset)};

                if (mFirstReflectedLine == 0 || Line < mFirstReflectedLine) {
                    mFirstReflectedLine = Line;
                }
            }

            FHeader& mHeader;
            const FSourceFile& mSource;
            std::vector<FNamespace> mNamespaces;
            std::vector<std::size_t> mKeywords;
            int mFirstReflectedLine{};
        };

    }

    std::unique_ptr<FHeader> ParseHeader(FSourceFile Source) {
        auto Header{std::make_unique<FHeader>()};
        Header->Source = std::move(Source);
        FHeaderParser{*Header}.Parse();

        return Header;
    }

    void CheckSourceFile(const FSourceFile& Source) {
        for (std::size_t Index{}; Index < Source.Tokens.size(); ++Index) {
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
