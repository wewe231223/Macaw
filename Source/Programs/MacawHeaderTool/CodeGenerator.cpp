#include "CodeGenerator.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>

namespace HeaderTool {

    namespace {

        std::string Join(const std::vector<std::string>& Parts, const std::string& Separator) {
            std::string Result;

            for (std::size_t Index{}; Index < Parts.size(); ++Index) {
                Result += (Index == 0 ? "" : Separator) + Parts[Index];
            }

            return Result;
        }

        // 생성 디렉터리는 항상 Intermediate/<이름>/Generated이므로 세 단계 위가 작업 공간 루트입니다.
        std::string IncludePath(const std::string& File) {
            const std::size_t Public{File.find("/Public/")};

            return Public != std::string::npos ? File.substr(Public + 8) : "../../../" + File;
        }

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

        struct FEntry {
            const FHeader* Header{};
            const FType* Type{};
            const FEnum* Enum{};
        };

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

    std::string GetGeneratedDirectory(const std::string& Module) {
        return "Intermediate/" + (Module == "App" ? std::string{"Macaw"} : Module) + "/Generated";
    }

    std::string GetRegistrationModule(const std::string& SourceModule) {
        return SourceModule == "Core" ? "CoreUObject" : SourceModule;
    }

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

    void WriteGeneratedHeaders(const FWorkspace& Workspace, const std::string& SourceModule) {
        std::map<std::string, std::string> Written;

        for (const auto& Header : Workspace.Headers) {
            if (Header->Source.Module != SourceModule || !Header->NeedsGeneratedHeader) {
                continue;
            }

            const auto [Existing, Inserted]{Written.emplace(Header->GeneratedName, Header->Source.Path)};

            if (!Inserted) {
                throw FHeaderToolError{Header->Source.Path, 0, "Reflected headers in one module need distinct file names; " + Existing->second + " also generates " + Header->GeneratedName};
            }

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
    }

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
