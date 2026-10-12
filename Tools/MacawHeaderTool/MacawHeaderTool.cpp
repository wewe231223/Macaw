#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <set>
#include <vector>

struct FToken {
    std::string mText{};
    std::size_t mLine{};
    bool mConditional{};
};

struct FPropertyDefinition {
    std::string mMember{};
    std::map<std::string, std::string> mOptions{};
};

struct FParameterDefinition {
    std::string mType{};
    std::string mName{};
};

struct FFunctionDefinition {
    std::string mName{};
    std::string mReturnType{};
    std::vector<FParameterDefinition> mParameters{};
};

struct FTypeDefinition {
    std::string mName{};
    std::string mParent{};
    std::string mKind{};
    std::string mHeader{};
    std::map<std::string, std::string> mOptions{};
    std::vector<FPropertyDefinition> mProperties{};
    std::vector<FFunctionDefinition> mFunctions{};
    std::vector<std::string> mValues{};
};

std::string ReadFile(const std::filesystem::path& Path) {
    std::ifstream Stream{Path, std::ios::binary};

    if (!Stream) {
        throw std::runtime_error{"Cannot read " + Path.string()};
    }

    return {std::istreambuf_iterator<char>{Stream}, std::istreambuf_iterator<char>{}};
}

void WriteFile(const std::filesystem::path& Path, const std::string& Content) {
    std::string WindowsContent{};

    for (char Character : Content) {
        if (Character == '\n') {
            WindowsContent += '\r';
        }

        WindowsContent += Character;
    }

    if (std::filesystem::exists(Path) && ReadFile(Path) == WindowsContent) {
        return;
    }

    std::filesystem::create_directories(Path.parent_path());
    std::ofstream Stream{Path, std::ios::binary | std::ios::trunc};

    if (!Stream || !(Stream << WindowsContent)) {
        throw std::runtime_error{"Cannot write " + Path.string()};
    }
}

std::vector<FToken> Tokenize(const std::string& Source) {
    std::vector<FToken> Tokens{};
    std::size_t Line{1};
    std::size_t ConditionalDepth{};

    for (std::size_t Position{}; Position < Source.size();) {
        const char Character{Source[Position]};

        if (Character == '\n') {
            ++Line;
            ++Position;
        } else if (std::isspace(static_cast<unsigned char>(Character))) {
            ++Position;
        } else if (Character == '#' || Source.compare(Position, 2, "//") == 0) {
            const std::size_t Start{Position};

            while (Position < Source.size() && Source[Position] != '\n') {
                ++Position;
            }

            if (Character == '#') {
                std::istringstream Directive{Source.substr(Start + 1, Position - Start - 1)};
                std::string Name{};
                Directive >> Name;

                if (Name == "if" || Name == "ifdef" || Name == "ifndef") {
                    ++ConditionalDepth;
                } else if (Name == "endif" && ConditionalDepth != 0) {
                    --ConditionalDepth;
                }
            }
        } else if (Source.compare(Position, 2, "/*") == 0) {
            Position += 2;

            while (Position < Source.size() && Source.compare(Position, 2, "*/") != 0) {
                if (Source[Position++] == '\n') {
                    ++Line;
                }
            }

            Position = std::min(Position + 2, Source.size());
        } else if (Source.compare(Position, 2, "R\"") == 0) {
            const std::size_t Start{Position};
            const std::size_t Open{Source.find('(', Position + 2)};

            if (Open == std::string::npos || Open - Position > 18) {
                throw std::runtime_error{"Invalid raw string delimiter"};
            }

            const std::string Closing{")" + Source.substr(Position + 2, Open - Position - 2) + "\""};
            const std::size_t Close{Source.find(Closing, Open + 1)};

            if (Close == std::string::npos) {
                throw std::runtime_error{"Unterminated raw string literal"};
            }

            Position = Close + Closing.size();
            Tokens.push_back({Source.substr(Start, Position - Start), Line, ConditionalDepth != 0});
            Line += std::count(Source.begin() + Start, Source.begin() + Position, '\n');
        } else if (Character == '"' || Character == '\'') {
            const std::size_t Start{Position++};

            while (Position < Source.size() && Source[Position] != Character) {
                if (Source[Position] == '\\') {
                    ++Position;
                }

                ++Position;
            }

            if (Position >= Source.size()) {
                throw std::runtime_error{"Unterminated literal"};
            }

            Tokens.push_back({Source.substr(Start, ++Position - Start), Line, ConditionalDepth != 0});
        } else if (std::isalnum(static_cast<unsigned char>(Character)) || Character == '_') {
            const std::size_t Start{Position++};

            while (Position < Source.size() && (std::isalnum(static_cast<unsigned char>(Source[Position])) || Source[Position] == '_' || Source[Position] == '.')) {
                ++Position;
            }

            Tokens.push_back({Source.substr(Start, Position - Start), Line, ConditionalDepth != 0});
        } else {
            Tokens.push_back({std::string{Character}, Line, ConditionalDepth != 0});
            ++Position;
        }
    }

    return Tokens;
}

std::size_t FindClosing(const std::vector<FToken>& Tokens, std::size_t Position, const std::string& Open, const std::string& Close) {
    std::size_t Depth{};

    for (std::size_t Index{Position}; Index < Tokens.size(); ++Index) {
        if (Tokens[Index].mText == Open) {
            ++Depth;
        } else if (Tokens[Index].mText == Close && --Depth == 0) {
            return Index;
        }
    }

    throw std::runtime_error{"Unbalanced declaration"};
}

std::map<std::string, std::string> ParseOptions(const std::vector<FToken>& Tokens, std::size_t& Position) {
    if (Tokens.at(++Position).mText != "(") {
        throw std::runtime_error{"Reflection annotation requires parentheses"};
    }

    const std::size_t End{FindClosing(Tokens, Position, "(", ")")};
    std::map<std::string, std::string> Options{};

    while (++Position < End) {
        const std::string Key{Tokens[Position].mText};
        std::string Value{"true"};

        if (Position + 1 < End && Tokens[Position + 1].mText == "=") {
            Position += 2;
            Value.clear();

            while (Position < End && Tokens[Position].mText != ",") {
                Value += Tokens[Position++].mText;
            }
        } else {
            ++Position;
        }

        if (!Options.emplace(Key, Value).second) {
            throw std::runtime_error{"Duplicate reflection option " + Key};
        }

        if (Position < End && Tokens[Position].mText != ",") {
            throw std::runtime_error{"Invalid reflection option"};
        }
    }

    Position = End;
    return Options;
}

std::string Option(const std::map<std::string, std::string>& Options, const std::string& Key, const std::string& Default) {
    const auto Found{Options.find(Key)};

    return Found != Options.end() ? Found->second : Default;
}

void ValidateOptions(const std::map<std::string, std::string>& Options, const std::set<std::string>& Allowed, const std::set<std::string>& Flags) {
    for (const auto& [Key, Value] : Options) {
        if (!Allowed.contains(Key)) {
            throw std::runtime_error{"Unknown reflection option " + Key};
        }

        if (Value.empty() || (Flags.contains(Key) && Value != "true")) {
            throw std::runtime_error{"Invalid value for reflection option " + Key};
        }
    }
}

std::string NativeType(const std::vector<FToken>& Tokens, std::size_t Start, std::size_t End) {
    std::string Type{};

    for (std::size_t Index{Start}; Index < End; ++Index) {
        const std::string& Text{Tokens[Index].mText};

        if (Text != "const" && Text != "&" && Text != "virtual" && Text != "override" && Text != "static") {
            Type += Text;
        }
    }

    return Type;
}

FFunctionDefinition ParseFunction(const std::vector<FToken>& Tokens, std::size_t Start, std::size_t& End) {
    std::size_t Open{Start};

    while (Open < Tokens.size() && Tokens[Open].mText != "(" && Tokens[Open].mText != ";") {
        ++Open;
    }

    if (Open == Tokens.size() || Tokens[Open].mText != "(" || Open == Start) {
        throw std::runtime_error{"UFUNCTION must precede a member function declaration"};
    }

    FFunctionDefinition Function{};
    Function.mName = Tokens[Open - 1].mText;
    Function.mReturnType = NativeType(Tokens, Start, Open - 1);
    const std::size_t Close{FindClosing(Tokens, Open, "(", ")")};
    std::size_t ParameterStart{Open + 1};
    std::size_t Depth{};

    for (std::size_t Index{ParameterStart}; Index <= Close; ++Index) {
        const std::string& Text{Tokens[Index].mText};

        if (Text == "<") {
            ++Depth;
        } else if (Text == ">") {
            --Depth;
        }

        if (Index == Close || (Text == "," && Depth == 0)) {
            if (Index > ParameterStart) {
                std::size_t ParameterEnd{Index};

                for (std::size_t Part{ParameterStart}; Part < Index; ++Part) {
                    if (Tokens[Part].mText == "=") {
                        ParameterEnd = Part;
                        break;
                    }

                    if (Tokens[Part].mText == "&" && Tokens[ParameterStart].mText != "const") {
                        throw std::runtime_error{"Mutable reference parameters require an explicit invocation adapter"};
                    }
                }

                Function.mParameters.push_back({NativeType(Tokens, ParameterStart, ParameterEnd - 1), Tokens[ParameterEnd - 1].mText});
            }

            ParameterStart = Index + 1;
        }
    }

    End = Close;
    return Function;
}

std::vector<FTypeDefinition> ParseHeader(const std::string& Source, const std::string& Header) {
    const std::vector<FToken> Tokens{Tokenize(Source)};
    std::vector<FTypeDefinition> Types{};

    for (std::size_t Position{}; Position < Tokens.size(); ++Position) {
        const std::string Marker{Tokens[Position].mText};

        if (Marker != "UCLASS" && Marker != "USTRUCT" && Marker != "UENUM") {
            continue;
        }

        const std::size_t Line{Tokens[Position].mLine};

        try {
            if (Tokens[Position].mConditional) {
                throw std::runtime_error{"Reflection annotations inside conditional compilation are unsupported"};
            }

            FTypeDefinition Type{};
            Type.mKind = Marker;
            Type.mHeader = Header;
            Type.mOptions = ParseOptions(Tokens, Position);
            ValidateOptions(Type.mOptions, Marker == "UCLASS" ? std::set<std::string>{"Abstract", "NoFactory", "Factory"} : std::set<std::string>{}, {"Abstract", "NoFactory"});

            if (Type.mOptions.contains("Factory") && Type.mOptions.contains("NoFactory")) {
                throw std::runtime_error{"Factory and NoFactory cannot be combined"};
            }

            ++Position;

            if (Marker == "UENUM") {
                if (Tokens.at(Position++).mText != "enum" || Tokens.at(Position++).mText != "class") {
                    throw std::runtime_error{"UENUM requires enum class"};
                }
            } else if (Tokens.at(Position++).mText != (Marker == "UCLASS" ? "class" : "struct")) {
                throw std::runtime_error{"Reflection annotation does not match its declaration"};
            }

            Type.mName = Tokens.at(Position++).mText;

            while (Tokens.at(Position).mText != "{") {
                if (Tokens[Position].mText == "public" && Marker == "UCLASS") {
                    Type.mParent = Tokens.at(Position + 1).mText;
                } else if (Tokens[Position].mText == "," && Marker == "UCLASS") {
                    throw std::runtime_error{"Reflected classes support one UObject base"};
                }

                ++Position;
            }

            const std::size_t End{FindClosing(Tokens, Position, "{", "}")};

            if (Marker == "UENUM") {
                bool ExpectName{true};
                std::size_t Depth{};

                for (++Position; Position < End; ++Position) {
                    if (Tokens[Position].mConditional && (Tokens[Position].mText == "UPROPERTY" || Tokens[Position].mText == "UFUNCTION")) {
                        throw std::runtime_error{"Reflection annotations inside conditional compilation are unsupported"};
                    }

                    const std::string& Text{Tokens[Position].mText};

                    if (ExpectName) {
                        Type.mValues.push_back(Text);
                        ExpectName = false;
                    } else if (Text == "(" || Text == "{") {
                        ++Depth;
                    } else if (Text == ")" || Text == "}") {
                        --Depth;
                    } else if (Text == "," && Depth == 0) {
                        ExpectName = true;
                    }
                }
            } else {
                bool GeneratedTypesFound{};
                bool GeneratedBodyFound{};
                std::set<std::string> PropertyNames{};
                std::set<std::string> FunctionNames{};

                for (++Position; Position < End; ++Position) {
                    if (Tokens[Position].mText == "UPROPERTY") {
                        FPropertyDefinition Property{};
                        Property.mOptions = ParseOptions(Tokens, Position);
                        ValidateOptions(Property.mOptions, {"Edit", "Serialize", "ReadOnly", "Transient", "Reference", "Owned", "Color", "Name", "Getter", "Setter", "Category", "DisplayName", "Min", "Max", "Speed"}, {"Edit", "Serialize", "ReadOnly", "Transient", "Reference", "Owned", "Color"});

                        if (Property.mOptions.contains("Transient") && Property.mOptions.contains("Serialize")) {
                            throw std::runtime_error{"Transient and Serialize cannot be combined"};
                        }

                        while (++Position < End && Tokens[Position].mText != ";" && Tokens[Position].mText != "{" && Tokens[Position].mText != "=") {
                            Property.mMember = Tokens[Position].mText;
                        }

                        if (Property.mMember.size() < 2 || Property.mMember.front() != 'm' || !std::isupper(static_cast<unsigned char>(Property.mMember[1]))) {
                            throw std::runtime_error{"Reflected members must use the mPascalCase naming convention"};
                        }

                        if (!PropertyNames.insert(Option(Property.mOptions, "Name", "\"" + Property.mMember.substr(1) + "\"")).second) {
                            throw std::runtime_error{"Duplicate reflected property name"};
                        }

                        Type.mProperties.push_back(std::move(Property));
                    } else if (Tokens[Position].mText == "UFUNCTION") {
                        ValidateOptions(ParseOptions(Tokens, Position), {}, {});
                        FFunctionDefinition Function{ParseFunction(Tokens, Position + 1, Position)};

                        if (Marker != "UCLASS" || !FunctionNames.insert(Function.mName).second) {
                            throw std::runtime_error{"Reflected functions require a UObject class and a unique name"};
                        }

                        Type.mFunctions.push_back(std::move(Function));
                    } else if (Tokens[Position].mText == "GENERATED_TYPES") {
                        if (Tokens.at(Position + 1).mText != "(" || Tokens.at(Position + 2).mText != Type.mName || Tokens.at(Position + 3).mText != ")" || GeneratedTypesFound) {
                            throw std::runtime_error{"Generated types must match their reflected type"};
                        }

                        GeneratedTypesFound = true;
                    } else if (Tokens[Position].mText == "GENERATED_BODY") {
                        if (Tokens.at(Position + 1).mText != "(" || Tokens.at(Position + 2).mText != Type.mName || Tokens.at(Position + 3).mText != ")" || GeneratedBodyFound) {
                            throw std::runtime_error{"Generated body must match its reflected type"};
                        }

                        GeneratedBodyFound = true;
                    }
                }

                if (!GeneratedTypesFound || !GeneratedBodyFound) {
                    throw std::runtime_error{"Reflected types require generated types and body declarations"};
                }
            }

            Types.push_back(std::move(Type));
            Position = End;
        } catch (const std::exception& Error) {
            throw std::runtime_error{Header + "(" + std::to_string(Line) + "): error: " + Error.what()};
        }
    }

    return Types;
}

std::string PropertyFlags(const FPropertyDefinition& Property) {
    std::string Flags{"EPropertyFlags::None"};

    for (const std::string Key : {"Edit", "Serialize", "ReadOnly", "Transient", "Reference", "Owned", "Color"}) {
        if (Property.mOptions.contains(Key)) {
            Flags += " | EPropertyFlags::" + Key;
        }
    }

    if (Property.mOptions.contains("Transient") && Property.mOptions.contains("Serialize")) {
        throw std::runtime_error{"Transient and Serialize cannot be combined"};
    }

    return Flags;
}

void Generate(const std::filesystem::path& Root, const std::string& Module, const std::filesystem::path& Output) {
    std::vector<FTypeDefinition> Types{};
    std::vector<std::filesystem::path> Headers{};

    for (const auto& Entry : std::filesystem::recursive_directory_iterator{Root / Module / "Public"}) {
        if (Entry.is_regular_file() && Entry.path().extension() == ".h") {
            Headers.push_back(Entry.path());
        }
    }

    std::ranges::sort(Headers);
    std::map<std::string, std::string> GeneratedHeaders{};
    std::map<std::string, std::string> Names{};

    for (const auto& Header : Headers) {
        const std::string Relative{std::filesystem::relative(Header, Root / Module / "Public").generic_string()};
        std::vector<FTypeDefinition> Parsed{ParseHeader(ReadFile(Header), Relative)};

        for (FTypeDefinition& Type : Parsed) {
            if (!Names.emplace(Type.mName, Type.mHeader).second) {
                throw std::runtime_error{"Duplicate reflected type " + Type.mName};
            }

            std::string& Generated{GeneratedHeaders[Relative]};

            if (Generated.empty()) {
                Generated = "#pragma once\n\nclass UClass;\nclass UScriptStruct;\nclass UEnum;\n\n";
            }

            Generated += "struct FReflectionAccess" + Type.mName + ";\n";

            if (Type.mKind == "UENUM") {
                Generated += "UEnum* StaticEnum" + Type.mName + "();\n";
            } else {
                Generated += "#define " + Type.mName + "_GENERATED_TYPES using ReflectionOwner = " + Type.mName + ";";

                if (!Type.mParent.empty()) {
                    Generated += " using Super = " + Type.mParent + ";";
                }

                Generated += " friend struct FReflectionAccess" + Type.mName + ";\n";
                Generated += "#define " + Type.mName + "_GENERATED_BODY static " + (Type.mKind == "USTRUCT" ? "UScriptStruct* StaticStruct();\n" : "UClass* StaticClass();\n");
            }

            Types.push_back(std::move(Type));
        }
    }

    std::ostringstream Code{};
    Code << "#include \"pch.h\"\n#include \"CoreUObject/Reflection/FReflectionRegistry.h\"\n#include \"CoreUObject/Reflection/UFunction.h\"\n#include \"CoreUObject/Reflection/NewObject.h\"\n#include \"CoreUObject/Reflection/NativeSerialization.h\"\n\n";

    for (const auto& [Header, Content] : GeneratedHeaders) {
        Code << "#include \"" << Header << "\"\n";
        std::filesystem::path GeneratedPath{Header};
        GeneratedPath.replace_extension(".generated.h");
        WriteFile(Output / GeneratedPath, Content);
    }

    for (const FTypeDefinition& Type : Types) {
        const std::string Access{"FReflectionAccess" + Type.mName};
        const std::string Descriptor{Type.mKind == "UENUM" ? "UEnum" : Type.mKind == "USTRUCT" ? "UScriptStruct" : "UClass"};
        Code << "\nstruct " << Access << " {\npublic:\n    static void Build(" << Descriptor << "& Information);\n";

        for (const FPropertyDefinition& Property : Type.mProperties) {
            const std::string& Member{Property.mMember};
            Code << "    static FPropertyValue Read" << Member << "(const void* Container);\n    static bool Write" << Member << "(void* Container, const FPropertyValue& Value);\n    static void Serialize" << Member << "(void* Container, FArchive& Archive);\n    static void Visit" << Member << "(const void* Container, const std::function<void(UObject*)>& Visitor);\n";
        }

        for (const FFunctionDefinition& Function : Type.mFunctions) {
            Code << "    static bool Invoke" << Function.mName << "(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result);\n";
        }

        Code << "};\n\nvoid " << Access << "::Build(" << Descriptor << "& Information) {\n";

        for (const std::string& Value : Type.mValues) {
            Code << "    Information.AddValue(FName{\"" << Value << "\"}, static_cast<Int64>(" << Type.mName << "::" << Value << "));\n";
        }

        for (const FPropertyDefinition& Property : Type.mProperties) {
            const std::string& Member{Property.mMember};
            const std::string ValueType{"std::remove_cvref_t<decltype(std::declval<" + Type.mName + ">()." + Member + ")>"};
            const std::string Name{Option(Property.mOptions, "Name", "\"" + Member.substr(1) + "\"")};
            Code << "    {\n        std::unique_ptr<FProperty> Property{std::make_unique<FProperty>(FName{" << Name << "}, &Information, GetNativeType<" << ValueType << ">(), " << PropertyFlags(Property) << ", &Read" << Member << ", &Write" << Member << ", &Serialize" << Member << ", &Visit" << Member << ")};\n";
            Code << "        Property->SetEditorMetadata(FString{" << Option(Property.mOptions, "Category", "\"\"") << "}, FString{" << Option(Property.mOptions, "DisplayName", Name) << "}, " << Option(Property.mOptions, "Min", "-std::numeric_limits<float>::max()") << ", " << Option(Property.mOptions, "Max", "std::numeric_limits<float>::max()") << ", " << Option(Property.mOptions, "Speed", "0.1f") << ");\n        Information.AddProperty(std::move(Property));\n    }\n";
        }

        for (const FFunctionDefinition& Function : Type.mFunctions) {
            Code << "    {\n        std::unique_ptr<UFunction> Function{std::make_unique<UFunction>(FName{\"" << Function.mName << "\"}, &Information, &Invoke" << Function.mName << ")};\n";

            for (const FParameterDefinition& Parameter : Function.mParameters) {
                Code << "        Function->AddParameter(FName{\"" << Parameter.mName << "\"}, GetNativeType<" << Parameter.mType << ">());\n";
            }

            if (Function.mReturnType != "void") {
                Code << "        Function->SetReturnType(GetNativeType<" << Function.mReturnType << ">());\n";
            }

            Code << "        Information.AddFunction(std::move(Function));\n    }\n";
        }

        Code << "}\n";

        for (const FPropertyDefinition& Property : Type.mProperties) {
            const std::string& Member{Property.mMember};
            const std::string ValueType{"std::remove_cvref_t<decltype(std::declval<" + Type.mName + ">()." + Member + ")>"};
            const std::string Getter{Option(Property.mOptions, "Getter", "")};
            const std::string Setter{Option(Property.mOptions, "Setter", "")};
            const std::string Name{Option(Property.mOptions, "Name", "\"" + Member.substr(1) + "\"")};
            Code << "\nFPropertyValue " << Access << "::Read" << Member << "(const void* Container) {\n    const " << Type.mName << "& Object{*static_cast<const " << Type.mName << "*>(Container)};\n\n    return FPropertyValue{Object." << (Getter.empty() ? Member : Getter + "()") << "};\n}\n";
            Code << "\nbool " << Access << "::Write" << Member << "(void* Container, const FPropertyValue& Value) {\n";

            if (Property.mOptions.contains("ReadOnly")) {
                Code << "    return false;\n";
            } else {
                Code << "    using ValueType = " << ValueType << ";\n\n    const ValueType* Typed{Value.Get<ValueType>()};\n\n    if (Typed == nullptr) {\n        return false;\n    }\n\n    " << Type.mName << "& Object{*static_cast<" << Type.mName << "*>(Container)};\n";

                if (Setter.empty()) {
                    Code << "\n    Object." << Member << " = *Typed;\n\n    return true;\n";
                } else {
                    Code << "\n    return ApplyPropertySetter([&Object, Typed]() {\n        return Object." << Setter << "(*Typed);\n    });\n";
                }
            }

            Code << "}\n\nvoid " << Access << "::Serialize" << Member << "(void* Container, FArchive& Archive) {\n";

            if (Property.mOptions.contains("Serialize")) {
                Code << "    using ValueType = " << ValueType << ";\n\n    " << Type.mName << "& Object{*static_cast<" << Type.mName << "*>(Container)};\n    ValueType Value{Object." << Member << "};\n\n    SerializeNativeValue(Archive, " << Name << ", Value);\n\n    if (Archive.IsLoading()) {\n        Object." << Member << " = Value;\n    }\n";
            }

            Code << "}\n\nvoid " << Access << "::Visit" << Member << "(const void* Container, const std::function<void(UObject*)>& Visitor) {\n    const " << Type.mName << "& Object{*static_cast<const " << Type.mName << "*>(Container)};\n\n    VisitNativeReferences<" << ValueType << ">(&Object." << Member << ", Visitor);\n}\n";
        }

        for (const FFunctionDefinition& Function : Type.mFunctions) {
            std::string Call{"static_cast<" + Type.mName + "&>(Object)." + Function.mName + "("};

            for (std::size_t Index{}; Index < Function.mParameters.size(); ++Index) {
                if (Index != 0) {
                    Call += ", ";
                }

                Call += "*Arguments[" + std::to_string(Index) + "].Get<" + Function.mParameters[Index].mType + ">()";
            }

            Call += ")";
            Code << "\nbool " << Access << "::Invoke" << Function.mName << "(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result) {\n";

            if (Function.mReturnType == "void") {
                Code << "    " << Call << ";\n";
            } else {
                Code << "    Result = FPropertyValue{" << Call << "};\n";
            }

            Code << "\n    return true;\n}\n";
        }

        Code << "\n" << Descriptor << "* " << (Type.mKind == "UENUM" ? "StaticEnum" + Type.mName : Type.mName + "::" + (Type.mKind == "USTRUCT" ? "StaticStruct" : "StaticClass")) << "() {\n";

        if (Type.mKind == "UENUM") {
            Code << "    return FReflectionRegistry::DefineEnum(FName{\"" << Type.mName << "\"}, GetNativeType<" << Type.mName << ">(), &" << Access << "::Build);\n";
        } else if (Type.mKind == "USTRUCT") {
            Code << "    return FReflectionRegistry::DefineStruct(FName{\"" << Type.mName << "\"}, GetNativeType<" << Type.mName << ">(), &" << Access << "::Build);\n";
        } else {
            const std::string Parent{Type.mParent.empty() ? "nullptr" : "&Super::StaticClass"};
            std::string Flags{"EClassFlags::None"};

            for (const std::string Flag : {"Abstract", "NoFactory"}) {
                if (Type.mOptions.contains(Flag)) {
                    Flags += " | EClassFlags::" + Flag;
                }
            }

            const std::string Factory{Type.mOptions.contains("NoFactory") ? "nullptr" : Type.mOptions.contains("Factory") ? "&" + Type.mOptions.at("Factory") : "&CreateNativeInstance<" + Type.mName + ">"};
            Code << "    return FReflectionRegistry::DefineClass(FName{\"" << Type.mName << "\"}, " << Parent << ", sizeof(" << Type.mName << "), alignof(" << Type.mName << "), " << Flags << ", " << Factory << ", &" << Access << "::Build);\n";
        }

        Code << "}\n";
    }

    Code << "\nvoid ReflectionModules::Register" << Module << "() {\n";

    if (Module == "CoreUObject") {
        Code << "    UField::StaticClass();\n    UStruct::StaticClass();\n    UClass::StaticClass();\n    UScriptStruct::StaticClass();\n    UEnum::StaticClass();\n    UFunction::StaticClass();\n";
    }

    for (const FTypeDefinition& Type : Types) {
        Code << "    " << (Type.mKind == "UENUM" ? "StaticEnum" + Type.mName : Type.mName + "::" + (Type.mKind == "USTRUCT" ? "StaticStruct" : "StaticClass")) << "();\n";
    }

    Code << "}\n";
    WriteFile(Output / (Module + ".Reflection.gen.cpp"), Code.str());
}

int main(int ArgumentCount, char** Arguments) {
    try {
        if (ArgumentCount != 4) {
            throw std::runtime_error{"Usage: MacawHeaderTool Root Module Output"};
        }

        Generate(std::filesystem::path{Arguments[1]}, Arguments[2], std::filesystem::path{Arguments[3]});
        return 0;
    } catch (const std::exception& Error) {
        std::cerr << Error.what() << '\n';
        return 1;
    }
}
