/**
 * @file CodeGenerator.cpp
 * @brief 리플렉션 생성 헤더와 모듈 등록 코드를 구성합니다.
 */

#include "CodeGenerator.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>

namespace HeaderTool {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {

        /**
         * @brief 문자열 조각 사이에 구분자를 넣어 합칩니다.
         * @param[in] Parts 구분자로 합칠 문자열 조각입니다.
         * @param[in] Separator 문자열 사이에 넣을 구분자입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string Join(const std::vector<std::string>& Parts, const std::string& Separator) {
            std::string Result;

            for (std::size_t Index{}; Index < Parts.size(); ++Index) {
                Result += (Index == 0 ? "" : Separator) + Parts[Index];
            }

            return Result;
        }

        // 생성 디렉터리는 항상 Intermediate/<이름>/Generated이므로 세 단계 위가 작업 공간 루트입니다.

        /**
         * @brief 생성 코드에서 원본 헤더를 포함할 경로를 계산합니다.
         * @param[in] File 처리할 원본 소스 파일 경로입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string IncludePath(const std::string& File) {
            const std::size_t Public{File.find("/Public/")};

            return Public != std::string::npos ? File.substr(Public + 8) : "../../../" + File;
        }

        /**
         * @brief 접근자·배열 여부·플래그에 맞는 프로퍼티 등록 호출문을 만듭니다.
         * @param[in] Property 등록문을 생성할 프로퍼티 정보입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string PropertyStatement(const FProperty& Property) {
            std::vector<std::string> Pointers{"&ReflectionOwner::" + Property.Member};

            if (!Property.Getter.empty() || !Property.Setter.empty()) {
                Pointers.push_back(Property.Getter.empty() ? "nullptr" : "&ReflectionOwner::" + Property.Getter);
                Pointers.push_back("&ReflectionOwner::" + Property.Setter);
            }

            std::vector<std::string> Flags;

            for (const std::string& Flag : Property.Flags) {
                Flags.push_back("Core::EPropertyFlags::" + Flag);
            }

            const std::string FlagText{Flags.empty() ? "Core::EPropertyFlags::None" : Join(Flags, " | ")};
            const std::string Function{Property.Array ? "RegisterArrayProperty" : "RegisterProperty"};

            return "        CoreUObject::" + Function + "<" + Join(Pointers, ", ") + ">(Information, " + Property.Name + ", " + FlagText + ");";
        }

        /**
         * @brief 등록할 타입 또는 열거형과 선언 헤더의 연결 정보입니다.
         */
        struct FEntry {
            /**
             * @brief 타입·열거형을 선언한 헤더의 비소유 주소입니다.
             */
            const FHeader* Header{};

            /**
             * @brief 등록할 타입 정의이며 열거형 항목이면 nullptr입니다.
             */
            const FType* Type{};

            /**
             * @brief 등록할 열거형 정의이며 타입 항목이면 nullptr입니다.
             */
            const FEnum* Enum{};
        };

        /**
         * @brief 등록 모듈에 속한 리플렉션 타입과 열거형을 모읍니다.
         * @param[in] Workspace 원본 헤더와 루트 경로를 가진 작업 공간입니다.
         * @param[in] Module 생성할 모듈 이름이며 비어 있으면 전체입니다.
         * @return 등록할 타입·열거형과 선언 헤더 목록입니다.
         */
        std::vector<FEntry> CollectEntries(const FWorkspace& Workspace, const std::string& Module) {
            std::vector<FEntry> Entries;

            for (const auto& Header : Workspace.Headers) {
                if (GetRegistrationModule(Header->Source.Module) != Module) {
                    continue;
                }

                for (const FType& Type : Header->Types) {
                    if (Type.Kind != ETypeKind::None) {
                        Entries.push_back({Header.get(), &Type, nullptr});
                    }
                }

                for (const FEnum& Enum : Header->Enums) {
                    if (Enum.Reflect) {
                        Entries.push_back({Header.get(), nullptr, &Enum});
                    }
                }
            }

            return Entries;
        }

        /**
         * @brief GENERATED_BODY에 들어갈 선언과 등록 함수를 조합합니다.
         * @param[in,out] Lines 생성한 소스 줄을 추가할 배열입니다.
         * @param[in] Header 파싱하거나 생성에 참조할 헤더입니다.
         * @param[in] Type 생성·조회·변환할 타입입니다.
         */
        void AppendBody(std::vector<std::string>& Lines, const FHeader& Header, const FType& Type) {
            std::vector<std::string> Body{"public:", "    using ReflectionOwner = " + Type.ShortName + ";"};

            if (!Type.Super.empty()) {
                Body.push_back("    using Super = " + Type.Super + ";");
            }

            if (Type.Kind == ETypeKind::Struct) {
                Body.push_back("    static CoreUObject::UScriptStruct* StaticStruct();");
            } else {
                Body.push_back("    static CoreUObject::UClass* StaticClass();");
                Body.push_back(Type.Kind == ETypeKind::RootClass ? "    virtual CoreUObject::UClass* GetClass() const;" : "    CoreUObject::UClass* GetClass() const override;");
            }

            Body.push_back("private:");
            Body.push_back("    template <typename T, typename TMetadata> friend void CoreUObject::BuildNativeReflection(TMetadata* Information);");
            Body.push_back("    static void RegisterReflectionMembers(CoreUObject::FReflectionBuilder& Information);");
            Body.push_back(Type.Keyword == "class" ? "private:" : "public:");

            Lines.push_back("");
            Lines.push_back("#define " + Header.FileId + "_" + std::to_string(Type.GeneratedBodyLine) + "_GENERATED_BODY \\");

            for (std::size_t Index{}; Index < Body.size(); ++Index) {
                Lines.push_back(Body[Index] + (Index + 1 < Body.size() ? " \\" : ""));
            }
        }

    }

    /**
     * @brief 모듈의 생성 디렉터리를 작업 공간 기준으로 반환합니다.
     */
    std::string GetGeneratedDirectory(const std::string& Module) {
        return "Intermediate/" + (Module == "App" ? std::string{"Macaw"} : Module) + "/Generated";
    }

    /**
     * @brief 헤더의 타입을 등록하는 모듈 이름을 반환합니다. Core 타입은 CoreUObject가 등록합니다.
     */
    std::string GetRegistrationModule(const std::string& SourceModule) {
        return SourceModule == "Core" ? "CoreUObject" : SourceModule;
    }

    /**
     * @brief 내용이 바뀐 경우에만 CRLF 줄바꿈으로 파일을 씁니다.
     */
    void WriteIfChanged(const std::string& Root, const std::string& Path, const std::vector<std::string>& Lines) {
        std::string Text;

        for (const std::string& Line : Lines) {
            for (const char Character : Line) {
                if (Character != '\r') {
                    Text += Character == '\n' ? std::string{"\r\n"} : std::string(1, Character);
                }
            }

            Text += "\r\n";
        }

        const std::filesystem::path File{std::filesystem::path{Root} / std::filesystem::path{Path}};
        std::ifstream Input{File, std::ios::binary};

        if (Input && std::string{std::istreambuf_iterator<char>{Input}, std::istreambuf_iterator<char>{}} == Text) {
            return;
        }

        Input.close();
        std::filesystem::create_directories(File.parent_path());

        std::ofstream Output{File, std::ios::binary | std::ios::trunc};

        Output << Text;

        if (!Output) {
            throw FHeaderToolError{Path, 0, "Cannot write generated file"};
        }
    }

    /**
     * @brief 모듈 디렉터리에 있는 헤더의 generated.h를 씁니다.
     */
    void WriteGeneratedHeaders(const FWorkspace& Workspace, const std::string& SourceModule) {
        // 모든 모듈의 Generated 폴더가 include 경로에 함께 오르므로 이름이 같으면 다른 모듈의 generated.h를 읽을 수 있습니다.
        std::map<std::string, std::string> AllNames;

        for (const auto& Header : Workspace.Headers) {
            if (!Header->NeedsGeneratedHeader) {
                continue;
            }

            const auto [Existing, Inserted]{AllNames.emplace(Header->GeneratedName, Header->Source.Path)};

            if (!Inserted) {
                throw FHeaderToolError{Header->Source.Path, 0, "Reflected header file names must be unique across all modules; " + Existing->second + " also generates " + Header->GeneratedName};
            }
        }

        std::set<std::string> Written;

        for (const auto& Header : Workspace.Headers) {
            if (Header->Source.Module != SourceModule || !Header->NeedsGeneratedHeader) {
                continue;
            }

            Written.insert(Header->GeneratedName);

            std::vector<std::string> Lines{"// MacawHeaderTool이 " + Header->Source.Path + "에서 생성한 파일입니다. 직접 수정하지 마세요.", "#pragma once", "", "#include \"Core/ReflectionMacros.h\""};
            const bool HasEnums{std::any_of(Header->Enums.begin(), Header->Enums.end(), [](const FEnum& Enum) { return Enum.Reflect; })};
            const bool HasTypes{std::any_of(Header->Types.begin(), Header->Types.end(), [](const FType& Type) { return Type.Kind != ETypeKind::None; })};

            if (HasEnums) {
                Lines.push_back("#include \"CoreUObject/Reflection/TNativeEnum.h\"");
            }

            if (HasTypes) {
                Lines.push_back("");
                Lines.push_back("#undef CURRENT_FILE_ID");
                Lines.push_back("#define CURRENT_FILE_ID " + Header->FileId);
            }

            for (const FType& Type : Header->Types) {
                if (Type.Kind != ETypeKind::None) {
                    AppendBody(Lines, *Header, Type);
                }
            }

            for (const FEnum& Enum : Header->Enums) {
                if (!Enum.Reflect) {
                    continue;
                }

                Lines.push_back("");
                Lines.push_back("namespace " + Enum.Namespace + " {");
                Lines.push_back(std::string{"    enum "} + (Enum.Scoped ? "class " : "") + Enum.Name + (Enum.Base.empty() ? "" : " : " + Enum.Base) + ";");
                Lines.push_back("    CoreUObject::UEnum* StaticEnum" + Enum.Name + "();");
                Lines.push_back("    CoreUObject::UEnum* GetNativeEnum(CoreUObject::TNativeEnum<" + Enum.Name + ">);");
                Lines.push_back("}");
            }

            WriteIfChanged(Workspace.Root, GetGeneratedDirectory(SourceModule) + "/" + Header->GeneratedName, Lines);
        }

        // 리플렉션을 지운 헤더나 삭제·이름 변경된 헤더의 generated.h는 남아 있으면 옛 선언이 계속 include될 수 있으므로 지웁니다.
        const std::filesystem::path Directory{std::filesystem::path{Workspace.Root} / GetGeneratedDirectory(SourceModule)};
        std::error_code Code;

        for (const auto& Entry : std::filesystem::directory_iterator{Directory, Code}) {
            const std::string Name{Entry.path().filename().string()};

            if (Entry.is_regular_file(Code) && Name.ends_with(".generated.h") && !Written.contains(Name)) {
                std::filesystem::remove(Entry.path(), Code);
            }
        }
    }

    /**
     * @brief 모듈의 리플렉션 등록 헤더와 소스를 씁니다.
     */
    void WriteModuleReflection(const FWorkspace& Workspace, const std::string& Module) {
        const std::vector<FEntry> Entries{CollectEntries(Workspace, Module)};
        const std::string Directory{GetGeneratedDirectory(Module)};
        std::vector<std::string> Header{"#pragma once", ""};
        std::vector<std::string> Source{"#include \"" + Module + "Reflection.h\"", "#include \"CoreUObject/Reflection/ReflectionMacros.h\""};
        std::set<std::string> Includes;

        for (const FEntry& Entry : Entries) {
            const std::string Include{IncludePath(Entry.Header->Source.Path)};

            if (Includes.insert(Include).second) {
                Header.push_back("#include \"" + Include + "\"");
            }

            if (Entry.Enum != nullptr) {
                std::vector<std::string> Values;

                for (const std::string& Value : Entry.Enum->Values) {
                    Values.push_back("ReflectEnumValue(" + Entry.Enum->Name + ", " + Value + ")");
                }

                Source.push_back("");
                Source.push_back("namespace " + Entry.Enum->Namespace + " {");
                Source.push_back("");
                Source.push_back("    ImplementEnumReflection(" + Entry.Enum->Name + ", " + Join(Values, ", ") + ")");
                Source.push_back("");
                Source.push_back("}");

                continue;
            }

            const FType& Type{*Entry.Type};
            const std::string Implementation{Type.Kind == ETypeKind::Struct ? "ImplementStructReflection" : (Type.Kind == ETypeKind::RootClass ? "ImplementRootReflection" : "ImplementReflection")};
            const std::string Flags{Type.ClassFlags.empty() ? "" : ", " + Join(Type.ClassFlags, ", ")};

            Source.push_back("");
            Source.push_back("namespace " + Type.Namespace + " {");
            Source.push_back("");
            Source.push_back("    " + Implementation + "(" + Type.Name + Flags + ")");
            Source.push_back("");
            Source.push_back("    void " + Type.Name + "::RegisterReflectionMembers(CoreUObject::FReflectionBuilder&" + (Type.Properties.empty() ? "" : " Information") + ") {");

            const std::vector<FType>& Types{Entry.Header->Types};
            const int TypeIndex{static_cast<int>(&Type - Types.data())};

            for (const FEntry& Child : Entries) {
                if (Child.Type != nullptr && Child.Header == Entry.Header && Child.Type->Outer == TypeIndex) {
                    Source.push_back("        " + Child.Type->ShortName + "::StaticStruct();");
                }
            }

            for (const FProperty& Property : Type.Properties) {
                Source.push_back(PropertyStatement(Property));
            }

            Source.push_back("    }");
            Source.push_back("");
            Source.push_back("}");
        }

        Header.push_back("");
        Header.push_back("namespace " + Module + "::ReflectionModules {");
        Header.push_back("    void Register" + Module + "Types();");
        Header.push_back("}");
        Source.push_back("");
        Source.push_back("void " + Module + "::ReflectionModules::Register" + Module + "Types() {");

        if (Module == "App") {
            std::set<std::string> Names;

            for (const auto& Candidate : Workspace.Headers) {
                const FHeader& Each{*Candidate};
                const bool Reflected{std::any_of(Each.Types.begin(), Each.Types.end(), [](const FType& Type) { return Type.Kind != ETypeKind::None; }) || std::any_of(Each.Enums.begin(), Each.Enums.end(), [](const FEnum& Enum) { return Enum.Reflect; })};

                if (Reflected) {
                    Names.insert(GetRegistrationModule(Each.Source.Module));
                }
            }

            for (const std::string& Name : Names) {
                if (Name != "App" && Name != "Game") {
                    Source.insert(Source.begin() + 2, "#include \"" + Name + "Reflection.h\"");
                    Source.push_back("    " + Name + "::ReflectionModules::Register" + Name + "Types();");
                }
            }
        }

        for (const FEntry& Entry : Entries) {
            if (Entry.Enum != nullptr) {
                Source.push_back("    " + Entry.Enum->Namespace + "::StaticEnum" + Entry.Enum->Name + "();");

                continue;
            }

            const int Outer{Entry.Type->Outer};

            if (Outer < 0 || Entry.Header->Types[static_cast<std::size_t>(Outer)].Kind == ETypeKind::None) {
                Source.push_back("    " + Entry.Type->Namespace + "::" + Entry.Type->Name + (Entry.Type->Kind == ETypeKind::Struct ? "::StaticStruct();" : "::StaticClass();"));
            }
        }

        Source.push_back("}");
        WriteIfChanged(Workspace.Root, Directory + "/" + Module + "Reflection.h", Header);
        WriteIfChanged(Workspace.Root, Directory + "/" + Module + "Reflection.cpp", Source);
    }

}
