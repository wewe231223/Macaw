#include "CodeGenerator.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <set>

namespace HeaderTool {

    namespace {

        // 경계를 넘는 타입 규칙입니다. Source/Script/Private/ScriptAbi.h의 ToAbi, FromAbi와 맞아야 합니다.
        const std::map<std::string, std::string> sScalarTypes{
            {"float", "float"},
            {"int", "int"},
            {"Core::Int32", "int"},
            {"Core::Uint32", "uint"},
            {"Core::Uint64", "ulong"},
        };

        struct FStructType {
            std::string Abi;
            std::string Script;
        };

        const std::map<std::string, FStructType> sStructTypes{
            {"Core::FVector2", {"Script::Abi::FVector2", "Vector2"}},
            {"Core::FVector", {"Script::Abi::FVector3", "Vector3"}},
            {"Core::FQuat", {"Script::Abi::FQuat", "Quaternion"}},
            {"Core::FTransform", {"Script::Abi::FTransform", "Transform"}},
            {"Engine::FWorldHit", {"Script::Abi::FWorldHit", "RaycastHit"}},
            {"Script::FScriptValue", {"Script::FScriptValue", "ScriptValue"}},
        };

        enum class EBindingKind {
            Bool,
            Scalar,
            String,
            Struct,
            Enum,
            Object,
            ObjectArray
        };

        struct FBindingType {
            EBindingKind Kind{};
            std::string Abi;
            std::string Script;
            std::string ScriptAbi;
            std::string Native;
            const FType* Class{};
        };

        struct FScriptClass {
            std::string Name;
            std::string Super;
        };

        struct FWrapper {
            bool Static{};
            bool Internal{};
            std::vector<std::string> Lines;
        };

        std::string Join(const std::vector<std::string>& Parts, const std::string& Separator) {
            std::string Result;

            for (std::size_t Index{}; Index < Parts.size(); ++Index) {
                Result += (Index == 0 ? "" : Separator) + Parts[Index];
            }

            return Result;
        }

        std::string ShortName(const std::string& Type) {
            std::size_t First{Type.size()};

            while (First > 0 && IsIdentifierCharacter(Type[First - 1])) {
                --First;
            }

            return Type.substr(First);
        }

        std::string StripPrefix(const std::string& Text, const std::string& Prefix) {
            return Text.starts_with(Prefix) ? Text.substr(Prefix.size()) : Text;
        }

        std::string StripClassPrefix(const std::string& Name) {
            return !Name.empty() && (Name[0] == 'A' || Name[0] == 'U') ? Name.substr(1) : Name;
        }

        bool IsWrapperRoot(const std::string& Name) {
            return Name == "AActor" || Name == "UActorComponent" || Name == "UWorld";
        }

        class FBindingWriter {
        public:
            explicit FBindingWriter(const FWorkspace& Workspace)
                : mWorkspace{Workspace} {
            }

            void Write() {
                CollectWrapperClasses();
                std::vector<const FFunction*> Functions;

                for (const auto& Header : mWorkspace.Headers) {
                    for (const FFunction& Function : Header->Functions) {
                        Functions.push_back(&Function);
                    }

                    for (const FEnum& Enum : Header->Enums) {
                        if (Enum.Script) {
                            mEnums.push_back(&Enum);
                            mEnumsByNative[Enum.Namespace + "::" + Enum.Name] = &Enum;
                        }
                    }
                }

                std::stable_sort(Functions.begin(), Functions.end(), [](const FFunction* Left, const FFunction* Right) {
                    return Left->Owner->ShortName + Left->Name + std::to_string(Left->Parameters.size()) < Right->Owner->ShortName + Right->Name + std::to_string(Right->Parameters.size());
                });

                for (std::size_t Index{}; Index < Functions.size(); ++Index) {
                    WriteFunction(*Functions[Index], Index + 1);
                }

                WriteFiles(Functions.size());
            }

        private:
            void CollectWrapperClasses() {
                for (const auto& Header : mWorkspace.Headers) {
                    for (const FType& Type : Header->Types) {
                        if (Type.Kind == ETypeKind::Class || Type.Kind == ETypeKind::RootClass) {
                            mByName[Type.ShortName] = &Type;
                            mOrder.push_back(&Type);
                        }
                    }
                }

                std::function<bool(const FType&)> IsWrapped{[&](const FType& Type) {
                    if (IsWrapperRoot(Type.ShortName)) {
                        return true;
                    }

                    const auto Super{Type.Super.empty() ? mByName.end() : mByName.find(ShortName(Type.Super))};

                    return Super != mByName.end() && IsWrapped(*Super->second);
                }};

                for (const FType* Type : mOrder) {
                    if (mByName[Type->ShortName] == Type && Type->SourceModule == "Engine" && IsWrapped(*Type)) {
                        mClasses.push_back(Type);
                        mScriptClasses[Type] = {StripClassPrefix(Type->ShortName), IsWrapperRoot(Type->ShortName) ? "EngineObject" : StripClassPrefix(ShortName(Type->Super))};
                    }
                }
            }

            std::string ScriptNameOf(const FType* Type) const {
                const auto Found{mScriptClasses.find(Type)};

                return Found == mScriptClasses.end() ? std::string{} : Found->second.Name;
            }

            const FType* FindClass(const std::string& Name) const {
                const auto Found{mByName.find(Name)};

                return Found == mByName.end() ? nullptr : Found->second;
            }

            void AddInclude(const std::string& File) {
                const std::size_t Public{File.find("/Public/")};

                if (Public != std::string::npos) {
                    mIncludes.insert(File.substr(Public + 8));
                }
            }

            FBindingType Resolve(const std::string& Text, const FFunction& Function) const {
                const auto Scalar{sScalarTypes.find(Text)};
                const auto Struct{sStructTypes.find(Text)};
                const auto Enum{mEnumsByNative.find(Text)};

                if (Text == "bool") {
                    return {EBindingKind::Bool, "Core::Uint8", "bool", "byte"};
                }

                if (Scalar != sScalarTypes.end()) {
                    return {EBindingKind::Scalar, Text, Scalar->second, Scalar->second};
                }

                if (Text == "Core::FString") {
                    return {EBindingKind::String, "const char*", "string", "byte*"};
                }

                if (Struct != sStructTypes.end()) {
                    return {EBindingKind::Struct, Struct->second.Abi, Struct->second.Script, Struct->second.Script};
                }

                if (Enum != mEnumsByNative.end()) {
                    return {EBindingKind::Enum, "Core::Int32", StripPrefix(Enum->second->Name, "E"), "int", Text};
                }

                std::string Element;
                const bool Array{Text.starts_with("Core::TArray<") && Text.ends_with(">")};

                if (Array) {
                    Element = Text.substr(13, Text.size() - 14);
                    const std::size_t Star{Element.rfind('*')};

                    if (Star == std::string::npos || !Trim(std::string_view{Element}.substr(Star + 1)).empty()) {
                        throw FHeaderToolError{Function.File, 0, Function.Label + ": Unsupported type " + Text};
                    }

                    Element = Trim(std::string_view{Element}.substr(0, Star));
                } else if (Text.ends_with("*")) {
                    Element = Trim(std::string_view{Text}.substr(0, Text.size() - 1));
                }

                const FType* Object{FindClass(ShortName(Element))};
                const std::string Script{Object == nullptr ? std::string{} : (Object->ShortName == "UObject" ? std::string{"EngineObject"} : ScriptNameOf(Object))};

                if (Element.empty() || Script.empty()) {
                    throw FHeaderToolError{Function.File, 0, Function.Label + ": Unsupported type " + Text};
                }

                return {Array ? EBindingKind::ObjectArray : EBindingKind::Object, "CoreUObject::FObjectHandle", Script, "ObjectHandle", Element, Object};
            }

            void WriteFunction(const FFunction& Function, std::size_t Index) {
                const FType& Owner{*Function.Owner};
                const std::string Native{Owner.Namespace + "::" + Owner.Name};
                const FType* OwnerClass{FindClass(Owner.ShortName)};
                std::string ScriptClass{Owner.ShortName == "UObject" ? std::string{"EngineObject"} : (OwnerClass != nullptr ? ScriptNameOf(OwnerClass) : std::string{})};
                AddInclude(Function.File);

                if (ScriptClass.empty()) {
                    if (!Function.Static || !Owner.ShortName.starts_with("FScript")) {
                        throw FHeaderToolError{Function.File, 0, Function.Label + ": " + "Only static functions on FScript* classes or AActor, UActorComponent and UWorld classes can be bound"};
                    }

                    ScriptClass = StripPrefix(Owner.ShortName, "FScript");
                }

                std::vector<std::string> ThunkParameters;
                std::vector<std::string> Setup;
                std::vector<std::string> Arguments;
                std::vector<std::string> ScriptParameters;
                std::vector<std::string> ScriptAbiTypes;
                std::vector<std::string> ScriptArguments;
                std::vector<std::string> ScriptFixed;
                std::vector<std::string> ScriptLocals;
                std::string Signature{Native + "::" + Function.Name + "("};

                if (!Function.Static) {
                    ThunkParameters.push_back("CoreUObject::FObjectHandle SelfHandle");
                    ScriptAbiTypes.push_back("ObjectHandle");
                    ScriptArguments.push_back("Handle");
                    Setup.push_back("        auto* Self{Script::Abi::Resolve<" + Native + ">(SelfHandle)};");
                    Setup.push_back("        if (Self == nullptr) { return 0; }");
                }

                for (std::size_t ParameterIndex{}; ParameterIndex < Function.Parameters.size(); ++ParameterIndex) {
                    const FParameter& Parameter{Function.Parameters[ParameterIndex]};
                    const FBindingType Type{Resolve(Parameter.Type, Function)};
                    const std::string Argument{"Arg" + std::to_string(ParameterIndex + 1)};
                    const std::string Pass{"Pass" + std::to_string(ParameterIndex + 1)};
                    Signature += Parameter.Type + ",";

                    if (Type.Kind == EBindingKind::ObjectArray) {
                        throw FHeaderToolError{Function.File, 0, Function.Label + ": " + "Arrays can only be returned"};
                    }

                    if (Type.Kind == EBindingKind::Struct) {
                        ThunkParameters.push_back("const " + Type.Abi + "* " + Argument);
                        Arguments.push_back("Script::Abi::FromAbi(*" + Argument + ")");
                        ScriptAbiTypes.push_back(Type.ScriptAbi + "*");
                        ScriptLocals.push_back("        " + Type.Script + " " + Pass + " = " + Parameter.Name + ";");
                        ScriptArguments.push_back("&" + Pass);
                    } else {
                        ThunkParameters.push_back(Type.Abi + " " + Argument);
                        ScriptAbiTypes.push_back(Type.ScriptAbi);

                        if (Type.Kind == EBindingKind::Bool) {
                            Arguments.push_back(Argument + " != 0");
                            ScriptArguments.push_back("(byte)(" + Parameter.Name + " ? 1 : 0)");
                        } else if (Type.Kind == EBindingKind::String) {
                            Arguments.push_back("Core::FString{" + Argument + "}");
                            ScriptFixed.push_back("        fixed (byte* " + Pass + " = Bindings.Utf8(" + Parameter.Name + "))");
                            ScriptArguments.push_back(Pass);
                        } else if (Type.Kind == EBindingKind::Enum) {
                            Arguments.push_back("static_cast<" + Type.Native + ">(" + Argument + ")");
                            AddInclude(mEnumsByNative.at(Type.Native)->File);
                            ScriptArguments.push_back("(int)" + Parameter.Name);
                        } else if (Type.Kind == EBindingKind::Object) {
                            AddInclude(Type.Class->File);
                            Setup.push_back("        " + Type.Native + "* " + Pass + "{};");
                            Setup.push_back("        if (!Script::Abi::ResolveArgument(" + Argument + ", " + Pass + ")) { return 0; }");
                            Arguments.push_back(Pass);
                            ScriptArguments.push_back("Bindings.HandleOf(" + Parameter.Name + ")");
                        } else {
                            Arguments.push_back(Argument);
                            ScriptArguments.push_back(Parameter.Name);
                        }
                    }

                    ScriptParameters.push_back(Type.Script + (Type.Kind == EBindingKind::Object ? "? " : " ") + Parameter.Name);
                }

                const std::string Call{(Function.Static ? Native + "::" : std::string{"Self->"}) + Function.Name + "(" + Join(Arguments, ", ") + ")"};
                std::vector<std::string> Body;
                std::string ScriptReturnType{"void"};
                std::vector<std::string> ScriptReturn;
                Signature += ")->" + Function.Return;

                if (Function.Return == "void") {
                    Body.push_back("        " + Call + ";");
                } else {
                    const FBindingType Type{Resolve(Function.Return, Function)};
                    ScriptReturnType = Type.Script;

                    if (Type.Kind == EBindingKind::ObjectArray) {
                        AddInclude(Type.Class->File);
                        ThunkParameters.push_back("const CoreUObject::FObjectHandle** Result");
                        ThunkParameters.push_back("Core::Int32* ResultCount");
                        Body.push_back("        *Result = Script::Abi::StoreObjects(" + Call + ", *ResultCount);");
                        ScriptAbiTypes.push_back("ObjectHandle**");
                        ScriptAbiTypes.push_back("int*");
                        ScriptArguments.push_back("&Ret");
                        ScriptArguments.push_back("&RetCount");
                        ScriptLocals.push_back("        ObjectHandle* Ret = null;");
                        ScriptLocals.push_back("        int RetCount = 0;");
                        ScriptReturn.push_back("        return Bindings.WrapArray<" + Type.Script + ">(Ret, RetCount);");
                        ScriptReturnType = Type.Script + "[]";
                    } else {
                        std::string Assign;

                        switch (Type.Kind) {
                        case EBindingKind::Bool:
                            Assign = "static_cast<Core::Uint8>((" + Call + ") ? 1 : 0)";
                            break;
                        case EBindingKind::String:
                            Assign = "Script::Abi::StoreString(" + Call + ")";
                            break;
                        case EBindingKind::Struct:
                            Assign = "Script::Abi::ToAbi(" + Call + ")";
                            break;
                        case EBindingKind::Enum:
                            Assign = "static_cast<Core::Int32>(" + Call + ")";
                            break;
                        case EBindingKind::Object:
                            Assign = "Script::Abi::ToHandle(" + Call + ")";
                            break;
                        default:
                            Assign = Call;
                            break;
                        }

                        ThunkParameters.push_back(Type.Abi + "* Result");
                        Body.push_back("        *Result = " + Assign + ";");
                        ScriptAbiTypes.push_back(Type.ScriptAbi + "*");
                        ScriptArguments.push_back("&Ret");
                        ScriptLocals.push_back("        " + Type.ScriptAbi + " Ret = default;");

                        if (Type.Kind == EBindingKind::Object) {
                            AddInclude(Type.Class->File);
                            ScriptReturnType = Type.Script + "?";
                            ScriptReturn.push_back("        return (" + Type.Script + "?)Bindings.Wrap(Ret);");
                        } else if (Type.Kind == EBindingKind::Bool) {
                            ScriptReturn.push_back("        return Ret != 0;");
                        } else if (Type.Kind == EBindingKind::String) {
                            ScriptReturn.push_back("        return Marshal.PtrToStringUTF8((nint)Ret)!;");
                        } else if (Type.Kind == EBindingKind::Enum) {
                            ScriptReturn.push_back("        return (" + Type.Script + ")Ret;");
                        } else {
                            ScriptReturn.push_back("        return Ret;");
                        }
                    }
                }

                mSignatures.push_back(Signature);
                mEntries.push_back("        reinterpret_cast<void*>(&Thunk" + std::to_string(Index) + "),");
                mThunks.push_back("    int __stdcall Thunk" + std::to_string(Index) + "(" + Join(ThunkParameters, ", ") + ") {");
                mThunks.insert(mThunks.end(), Setup.begin(), Setup.end());
                mThunks.insert(mThunks.end(), Body.begin(), Body.end());
                mThunks.push_back("        return 1;");
                mThunks.push_back("    }");
                mThunks.push_back("");

                if (mWrappers.find(ScriptClass) == mWrappers.end()) {
                    mWrappers[ScriptClass] = {Function.Static && OwnerClass == nullptr, Function.Internal, {}};
                    mWrapperOrder.push_back(ScriptClass);
                }

                std::vector<std::string>& Lines{mWrappers[ScriptClass].Lines};
                ScriptAbiTypes.push_back("int");
                Lines.push_back(std::string{"    "} + (Function.Internal ? "internal " : "public ") + (Function.Static ? "static " : "") + ScriptReturnType + " " + Function.Name + "(" + Join(ScriptParameters, ", ") + ") {");
                Lines.insert(Lines.end(), ScriptLocals.begin(), ScriptLocals.end());
                Lines.insert(Lines.end(), ScriptFixed.begin(), ScriptFixed.end());

                const std::string Invoke{"((delegate* unmanaged<" + Join(ScriptAbiTypes, ", ") + ">)Bindings.sTable[" + std::to_string(Index - 1) + "])(" + Join(ScriptArguments, ", ") + ")"};

                if (Function.Static) {
                    Lines.push_back("        " + Invoke + ";");
                } else {
                    Lines.push_back("        if (" + Invoke + " == 0) { throw Bindings.Gone(\"" + ScriptClass + "." + Function.Name + "\"); }");
                }

                Lines.insert(Lines.end(), ScriptReturn.begin(), ScriptReturn.end());
                Lines.push_back("    }");
                Lines.push_back("");
            }

            void WriteFiles(std::size_t Count) {
                // 엔진과 Macaw.Engine이 서로 다른 생성 결과로 빌드되면 시작할 때 거부합니다.
                std::uint32_t Hash{2166136261u};

                for (const std::string& Signature : mSignatures) {
                    for (const char Character : Signature) {
                        Hash = (Hash ^ static_cast<unsigned char>(Character)) * 16777619u;
                    }
                }

                std::vector<std::string> Source{"#include \"ScriptAbi.h\""};

                for (const std::string& Include : mIncludes) {
                    Source.push_back("#include \"" + Include + "\"");
                }

                Source.push_back("");
                Source.push_back("namespace {");
                Source.push_back("");
                Source.insert(Source.end(), mThunks.begin(), mThunks.end());
                Source.push_back("    void* const sThunks[]{");
                Source.insert(Source.end(), mEntries.begin(), mEntries.end());
                Source.push_back("        nullptr");
                Source.push_back("    };");
                Source.push_back("");
                Source.push_back("}");
                Source.push_back("");
                Source.push_back("Script::Abi::FBindingTable Script::Abi::GetBindingTable() {");
                Source.push_back("    return {sThunks, " + std::to_string(Count) + ", " + std::to_string(Hash) + "};");
                Source.push_back("}");

                std::vector<std::string> Script{"// <auto-generated/>", "#nullable enable", "using System.Numerics;", "using System.Runtime.InteropServices;", "", "namespace Macaw;", "", "internal static unsafe partial class Bindings {"};
                Script.push_back("    public const int Count = " + std::to_string(Count) + ";");
                Script.push_back("    public const uint Hash = " + std::to_string(Hash) + ";");
                Script.push_back("");
                Script.push_back("    private static readonly Dictionary<string, Func<ObjectHandle, EngineObject>> sFactories = new() {");

                for (const FType* Class : mClasses) {
                    Script.push_back("        [\"" + Class->ShortName + "\"] = Handle => new " + mScriptClasses.at(Class).Name + "(Handle),");
                }

                Script.push_back("    };");
                Script.push_back("");
                Script.push_back("    private static readonly Dictionary<Type, string> sNativeNames = new() {");

                for (const FType* Class : mClasses) {
                    Script.push_back("        [typeof(" + mScriptClasses.at(Class).Name + ")] = \"" + Class->ShortName + "\",");
                }

                Script.push_back("    };");
                Script.push_back("}");

                for (const FEnum* Enum : mEnums) {
                    Script.push_back("");
                    Script.push_back("public enum " + StripPrefix(Enum->Name, "E") + " {");

                    for (const std::string& Value : Enum->ScriptValues) {
                        Script.push_back("    " + Value + ",");
                    }

                    Script.push_back("}");
                }

                for (const FType* Class : mClasses) {
                    const FScriptClass& ScriptClass{mScriptClasses.at(Class)};
                    Script.push_back("");
                    Script.push_back("public unsafe partial class " + ScriptClass.Name + " : " + ScriptClass.Super + " {");
                    Script.push_back("    internal " + ScriptClass.Name + "(ObjectHandle Handle) : base(Handle) { }");

                    const auto Wrapper{mWrappers.find(ScriptClass.Name)};

                    if (Wrapper != mWrappers.end()) {
                        Script.push_back("");
                        Script.insert(Script.end(), Wrapper->second.Lines.begin(), Wrapper->second.Lines.end());
                        Script.pop_back();
                        mWrappers.erase(Wrapper);
                    }

                    Script.push_back("}");
                }

                for (const std::string& Name : mWrapperOrder) {
                    const auto Wrapper{mWrappers.find(Name)};

                    if (Wrapper == mWrappers.end()) {
                        continue;
                    }

                    Script.push_back("");

                    if (Wrapper->second.Static) {
                        Script.push_back(std::string{Wrapper->second.Internal ? "internal" : "public"} + " static unsafe partial class " + Name + " {");
                    } else {
                        Script.push_back("public abstract unsafe partial class " + Name + " {");
                    }

                    Script.insert(Script.end(), Wrapper->second.Lines.begin(), Wrapper->second.Lines.end());
                    Script.pop_back();
                    Script.push_back("}");
                }

                const std::string Directory{GetGeneratedDirectory("Script")};
                WriteIfChanged(mWorkspace.Root, Directory + "/ScriptBindings.cpp", Source);
                WriteIfChanged(mWorkspace.Root, Directory + "/ScriptBindings.g.cs", Script);
            }

            const FWorkspace& mWorkspace;
            std::map<std::string, const FType*> mByName;
            std::vector<const FType*> mOrder;
            std::vector<const FType*> mClasses;
            std::map<const FType*, FScriptClass> mScriptClasses;
            std::vector<const FEnum*> mEnums;
            std::map<std::string, const FEnum*> mEnumsByNative;
            std::set<std::string> mIncludes;
            std::vector<std::string> mSignatures;
            std::vector<std::string> mThunks;
            std::vector<std::string> mEntries;
            std::map<std::string, FWrapper> mWrappers;
            std::vector<std::string> mWrapperOrder;
        };

    }

    void WriteBindings(const FWorkspace& Workspace) {
        FBindingWriter{Workspace}.Write();
    }

}
