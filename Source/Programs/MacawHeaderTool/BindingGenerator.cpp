/**
 * @file BindingGenerator.cpp
 * @brief 리플렉션 함수·열거형에서 네이티브 ABI와 C# 호출 래퍼를 생성합니다.
 */

#include "Core/Types.h"
#include "CodeGenerator.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <set>

/**
 * @brief 리플렉션·C# 바인딩의 소스 분석과 코드 생성 기능입니다.
 */
namespace HeaderTool {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {

        // 경계를 넘는 타입 규칙입니다. Source/Script/Private/ScriptAbi.h의 ToAbi, FromAbi와 맞아야 합니다.

        /**
         * @brief 네이티브 스칼라와 C# 타입 대응 표입니다.
         */
        const std::map<std::string, std::string> sScalarTypes{
            {"float", "float"},
            {"int", "int"},
            {"Core::Int32", "int"},
            {"Core::Index_t", "int"},
            {"Core::Uint32", "uint"},
            {"Core::Uint64", "ulong"},
        };

        /**
         * @brief 엔진 구조체에 대응하는 ABI·C# 타입 이름입니다.
         */
        struct FStructType {
            /**
             * @brief 경계 호출의 C++ ABI 타입 이름입니다.
             */
            std::string Abi;

            /**
             * @brief C#에 공개할 타입 이름입니다.
             */
            std::string Script;
        };

        /**
         * @brief 엔진 구조체와 ABI·C# 구조체 대응 표입니다.
         */
        const std::map<std::string, FStructType> sStructTypes{
            {"Core::FVector2", {"Script::Abi::FVector2", "Vector2"}},
            {"Core::FVector", {"Script::Abi::FVector3", "Vector3"}},
            {"Core::FQuat", {"Script::Abi::FQuat", "Quaternion"}},
            {"Core::FTransform", {"Script::Abi::FTransform", "Transform"}},
            {"Engine::FWorldHit", {"Script::Abi::FWorldHit", "RaycastHit"}},
            {"Script::FScriptValue", {"Script::FScriptValue", "ScriptValue"}},
        };

        /**
         * @brief 노출 타입을 C# 경계 표현별로 분류합니다.
         */
        enum class EBindingKind {
            /**
             * @brief 불리언 값입니다.
             */
            Bool,

            /**
             * @brief 지원하는 스칼라 수치 타입입니다.
             */
            Scalar,

            /**
             * @brief UTF-8 문자열 값입니다.
             */
            String,

            /**
             * @brief 구조체로 전달하는 바인딩 또는 리플렉션 구조체입니다.
             */
            Struct,

            /**
             * @brief 정수로 전달하는 열거형입니다.
             */
            Enum,

            /**
             * @brief 엔진 객체 핸들입니다.
             */
            Object,

            /**
             * @brief 엔진 객체 핸들 배열입니다.
             */
            ObjectArray
        };

        /**
         * @brief 네이티브 타입의 바인딩 종류와 C++·C# ABI·래퍼 표현입니다.
         */
        struct FBindingType {
            /**
             * @brief 바인딩 표현의 종류입니다.
             */
            EBindingKind Kind{};

            /**
             * @brief 경계 호출의 C++ ABI 타입 이름입니다.
             */
            std::string Abi;

            /**
             * @brief C#에 공개할 타입 이름입니다.
             */
            std::string Script;

            /**
             * @brief C# 함수 포인터 경계의 ABI 타입 이름입니다.
             */
            std::string ScriptAbi;

            /**
             * @brief 원래 네이티브 타입 이름입니다.
             */
            std::string Native;

            /**
             * @brief 객체 바인딩 대상의 클래스 정의입니다.
             */
            const FType* Class{};
        };

        /**
         * @brief 엔진 클래스에 대응하는 C# 이름과 기반 클래스 이름입니다.
         */
        struct FScriptClass {
            /**
             * @brief 생성할 C# 엔진 객체 래퍼의 클래스 이름입니다.
             */
            std::string Name;

            /**
             * @brief C# 래퍼의 기반 클래스 이름입니다.
             */
            std::string Super;
        };

        /**
         * @brief C# 래퍼의 접근·정적 속성과 생성할 소스 줄입니다.
         */
        struct FWrapper {
            /**
             * @brief C# 래퍼의 정적 클래스 여부입니다.
             */
            bool Static{};

            /**
             * @brief C# 래퍼를 internal 접근으로 제한할지 나타냅니다.
             */
            bool Internal{};

            /**
             * @brief 생성 파일에 기록할 소스 줄입니다.
             */
            std::vector<std::string> Lines;
        };

        /**
         * @brief 문자열 조각 사이에 구분자를 넣어 합칩니다.
         * @param[in] Parts 구분자로 합칠 문자열 조각입니다.
         * @param[in] Separator 문자열 사이에 넣을 구분자입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string Join(const std::vector<std::string>& Parts, const std::string& Separator) {
            std::string Result;

            for (Core::Index_t Index{}; Index < Parts.size(); ++Index) {
                Result += (Index == 0 ? "" : Separator) + Parts[Index];
            }

            return Result;
        }

        /**
         * @brief 타입 문자열 끝의 짧은 식별자를 추출합니다.
         * @param[in] Type 생성·조회·변환할 타입입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string ShortName(const std::string& Type) {
            std::size_t First{Type.size()};

            while (First > 0 && IsIdentifierCharacter(Type[First - 1])) {
                --First;
            }

            return Type.substr(First);
        }

        /**
         * @brief 문자열 시작에 지정한 접두어가 있으면 제거합니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         * @param[in] Prefix 제거하거나 비교할 접두어입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string StripPrefix(const std::string& Text, const std::string& Prefix) {
            return Text.starts_with(Prefix) ? Text.substr(Prefix.size()) : Text;
        }

        /**
         * @brief 엔진 클래스 이름의 A·U 접두어를 제거합니다.
         * @param[in] Name 조회·등록·출력할 대상 이름입니다.
         * @return 해석·변환한 문자열입니다.
         */
        std::string StripClassPrefix(const std::string& Name) {
            return !Name.empty() && (Name[0] == 'A' || Name[0] == 'U') ? Name.substr(1) : Name;
        }

        /**
         * @brief C# 엔진 객체 래퍼 계층의 시작 타입인지 검사합니다.
         * @param[in] Name 조회·등록·출력할 대상 이름입니다.
         * @return Actor·Component·World·CameraModifier 래퍼 시작 타입이면 true입니다.
         */
        bool IsWrapperRoot(const std::string& Name) {
            return Name == "AActor" || Name == "UActorComponent" || Name == "UWorld" || Name == "UCameraModifier";
        }

        /**
         * @brief 노출 함수·엔진 객체 계층에서 C++ 썽크와 C# 래퍼를 생성합니다.
         */
        class FBindingWriter {
        public:
            /**
             * @brief 파싱한 작업 공간을 C++·C# 바인딩 생성기에 연결합니다.
             * @param[in] Workspace 원본 헤더와 루트 경로를 가진 작업 공간입니다.
             */
            explicit FBindingWriter(const FWorkspace& Workspace)
                : mWorkspace{Workspace} {
            }

            /**
             * @brief 함수·열거형을 수집해 네이티브 썽크와 C# 래퍼를 생성합니다.
             */
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

                for (Core::Index_t Index{}; Index < Functions.size(); ++Index) {
                    WriteFunction(*Functions[Index], Index + 1);
                }

                WriteFiles(Functions.size());
            }

        private:
            /**
             * @brief Actor·Component·World·CameraModifier 계층에서 C# 래퍼 대상 클래스를 수집합니다.
             */
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

            /**
             * @brief 네이티브 클래스에 대응하는 C# 래퍼 이름을 반환합니다.
             * @param[in] Type 생성·조회·변환할 타입입니다.
             * @return 네이티브 클래스에 대응하는 C# 래퍼 이름입니다.
             */
            std::string ScriptNameOf(const FType* Type) const {
                const auto Found{mScriptClasses.find(Type)};

                return Found == mScriptClasses.end() ? std::string{} : Found->second.Name;
            }

            /**
             * @brief 짧은 타입 이름으로 파싱된 클래스 정의를 찾습니다.
             * @param[in] Name 조회·등록·출력할 대상 이름입니다.
             * @return 찾은 타입의 비소유 주소이며 없으면 nullptr입니다.
             */
            const FType* FindClass(const std::string& Name) const {
                const auto Found{mByName.find(Name)};

                return Found == mByName.end() ? nullptr : Found->second;
            }

            /**
             * @brief 바인딩에서 사용할 공개 헤더의 포함 경로를 수집합니다.
             * @param[in] File 처리할 원본 소스 파일 경로입니다.
             */
            void AddInclude(const std::string& File) {
                const std::size_t Public{File.find("/Public/")};

                if (Public != std::string::npos) {
                    mIncludes.insert(File.substr(Public + 8));
                }
            }

            /**
             * @brief 네이티브 타입을 C#·ABI 표현으로 해석하고 미지원 타입은 오류로 보고합니다.
             * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
             * @param[in] Function 바인딩을 생성할 함수 메타데이터입니다.
             * @return 네이티브·C#·ABI 타입과 바인딩 종류입니다.
             */
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

            /**
             * @brief 노출 함수의 네이티브 썽크·ABI 서명·C# 호출 래퍼를 생성합니다.
             * @param[in] Function 바인딩을 생성할 함수 메타데이터입니다.
             * @param[in] Index 처리할 원소 또는 토큰의 인덱스입니다.
             */
            void WriteFunction(const FFunction& Function, Core::Index_t Index) {
                const FType& Owner{*Function.Owner};
                const std::string Native{Owner.Namespace + "::" + Owner.Name};
                const FType* OwnerClass{FindClass(Owner.ShortName)};
                std::string ScriptClass{Owner.ShortName == "UObject" ? std::string{"EngineObject"} : (OwnerClass != nullptr ? ScriptNameOf(OwnerClass) : std::string{})};

                AddInclude(Function.File);

                if (ScriptClass.empty()) {
                    if (!Function.Static || !Owner.ShortName.starts_with("FScript")) {
                        throw FHeaderToolError{Function.File, 0, Function.Label + ": " + "Only static functions on FScript* classes or AActor, UActorComponent, UWorld and UCameraModifier classes can be bound"};
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

                for (Core::Index_t ParameterIndex{}; ParameterIndex < Function.Parameters.size(); ++ParameterIndex) {
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

            /**
             * @brief 바인딩 서명 해시·함수 테이블·C# 래퍼를 조합해 파일에 기록합니다.
             * @param[in] Count 처리할 원소 수입니다.
             */
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

            /**
             * @brief 생성 대상 헤더를 소유하는 작업 공간 참조입니다.
             */
            const FWorkspace& mWorkspace;

            /**
             * @brief 짧은 클래스 이름으로 파싱된 타입을 찾는 색인입니다.
             */
            std::map<std::string, const FType*> mByName;

            /**
             * @brief 소스에서 수집한 타입 순서입니다.
             */
            std::vector<const FType*> mOrder;

            /**
             * @brief C# 객체 래퍼 생성 대상 클래스입니다.
             */
            std::vector<const FType*> mClasses;

            /**
             * @brief 네이티브 클래스별 C# 이름·상속 정보입니다.
             */
            std::map<const FType*, FScriptClass> mScriptClasses;

            /**
             * @brief C#에 노출할 열거형 목록입니다.
             */
            std::vector<const FEnum*> mEnums;

            /**
             * @brief 정규화한 네이티브 이름별 열거형 색인입니다.
             */
            std::map<std::string, const FEnum*> mEnumsByNative;

            /**
             * @brief 썽크가 포함할 공개 헤더 경로 집합입니다.
             */
            std::set<std::string> mIncludes;

            /**
             * @brief 양쪽 바인딩의 일치를 검사할 ABI 서명입니다.
             */
            std::vector<std::string> mSignatures;

            /**
             * @brief 생성할 네이티브 경계 함수의 소스 줄입니다.
             */
            std::vector<std::string> mThunks;

            /**
             * @brief 바인딩 함수 주소 테이블에 기록할 소스 줄입니다.
             */
            std::vector<std::string> mEntries;

            /**
             * @brief C# 이름별 함수 래퍼입니다.
             */
            std::map<std::string, FWrapper> mWrappers;

            /**
             * @brief C# 래퍼를 출력할 이름 순서입니다.
             */
            std::vector<std::string> mWrapperOrder;
        };

    }

    /**
     * @brief Script 모듈의 네이티브 썽크와 Macaw.Engine의 C# 래퍼를 씁니다.
     */
    void WriteBindings(const FWorkspace& Workspace) {
        FBindingWriter{Workspace}.Write();
    }

}
