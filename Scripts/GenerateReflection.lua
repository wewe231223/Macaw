local Reflection = {}

local PropertyArguments = { ReflectProperty = 3, ReflectAccessor = 5, ReflectSetter = 4, ReflectReadOnlyProperty = 3, ReflectReadOnlyAccessor = 4, ReflectArray = 3, ReflectArrayAccessor = 5 }

function Reflection.GetGeneratedDirectory(Module)
    return "Intermediate/" .. (Module == "App" and "Macaw" or Module) .. "/Generated"
end

local function MaskSource(Text)
    local Result = {}
    local Position = 1

    while Position <= #Text do
        local Character = Text:sub(Position, Position)
        local Pair = Text:sub(Position, Position + 1)
        local Last = Position

        if Pair == "//" or Character == "#" then
            Last = (Text:find("\n", Position, true) or (#Text + 1)) - 1
        elseif Pair == "/*" then
            local Finish = assert(Text:find("*/", Position + 2, true), "Unterminated reflection source comment")
            Last = Finish + 1
        elseif Pair == 'R"' then
            local Opening = assert(Text:find("(", Position + 2, true), "Invalid reflection source raw string")
            local Delimiter = Text:sub(Position + 2, Opening - 1)
            local Closing = ")" .. Delimiter .. '"'
            local Finish = assert(Text:find(Closing, Opening + 1, true), "Unterminated reflection source raw string")
            Last = Finish + #Closing - 1
        elseif Character == '"' or Character == "'" then
            Last = Position + 1

            while Last <= #Text and Text:sub(Last, Last) ~= Character do
                Last = Last + (Text:sub(Last, Last) == "\\" and 2 or 1)
            end

            assert(Last <= #Text, "Unterminated reflection source string")
        end

        Result[#Result + 1] = Last == Position and Character or Text:sub(Position, Last):gsub("[^\r\n]", " ")
        Position = Last + 1
    end

    return table.concat(Result)
end

local function ReadCalls(Text, File)
    local Mask = MaskSource(Text)
    local Calls = {}
    local Position = 1

    while Position <= #Mask do
        local Start, Finish, Name = Mask:find("%f[%a_]([%a_][%w_]*)%s*%(", Position)

        if not Start then
            break
        end

        Position = Finish + 1

        if Name:match("^Reflect") then
            local Arguments = {}
            local First = Finish + 1
            local Depth = 1
            local Braces = 0
            local Brackets = 0

            while Position <= #Mask and Depth > 0 do
                local Character = Mask:sub(Position, Position)

                if Character == "(" then
                    Depth = Depth + 1
                elseif Character == ")" then
                    Depth = Depth - 1
                elseif Character == "{" then
                    Braces = Braces + 1
                elseif Character == "}" then
                    Braces = Braces - 1
                elseif Character == "[" then
                    Brackets = Brackets + 1
                elseif Character == "]" then
                    Brackets = Brackets - 1
                end

                if Depth == 0 or (Character == "," and Depth == 1 and Braces == 0 and Brackets == 0) then
                    local Argument = Text:sub(First, Position - 1):match("^%s*(.-)%s*$")

                    if #Argument > 0 then
                        Arguments[#Arguments + 1] = Argument
                    end

                    First = Position + 1
                end

                Position = Position + 1
            end

            assert(Depth == 0, File .. ": Unterminated " .. Name)
            Calls[#Calls + 1] = { Name = Name, Arguments = Arguments, Position = Start }
        end
    end

    return Calls, Mask
end

local function ReadNamespaces(Mask)
    local Namespaces = {}
    local Position = 1

    while Position <= #Mask do
        local Start, Opening, Name = Mask:find("%f[%a_]namespace%s+([%a_][%w_:]*)%s*{", Position)

        if not Start then
            break
        end

        local Closing = Opening + 1
        local Depth = 1

        while Closing <= #Mask and Depth > 0 do
            local Character = Mask:sub(Closing, Closing)
            Depth = Depth + (Character == "{" and 1 or (Character == "}" and -1 or 0))
            Closing = Closing + 1
        end

        assert(Depth == 0, "Unterminated reflection namespace " .. Name)
        Namespaces[#Namespaces + 1] = { Name = Name, Opening = Opening, Closing = Closing }
        Position = Opening + 1
    end

    return Namespaces
end

local function NamespaceAt(Namespaces, Position)
    local Names = {}

    for _, Namespace in ipairs(Namespaces) do
        if Position > Namespace.Opening and Position < Namespace.Closing then
            Names[#Names + 1] = Namespace.Name
        end
    end

    return table.concat(Names, "::")
end

local function ReadDefinitions(Mask, File)
    local Types = {}
    local Position = 1
    local Namespaces = ReadNamespaces(Mask)

    while Position <= #Mask do
        local Start, Finish, Keyword = Mask:find("%f[%a_]([%a_][%w_]*)", Position)

        if not Start then
            break
        end

        Position = Finish + 1

        if (Keyword == "class" or Keyword == "struct") and not Mask:sub(1, Start - 1):match("enum%s*$") then
            local Opening = Mask:find("{", Position, true)
            local Semicolon = Mask:find(";", Position, true)
            local Name = Mask:sub(Position):match("^%s*alignas%s*%b()%s*([%a_][%w_]*)") or Mask:sub(Position):match("^%s*([%a_][%w_]*)")

            if Name and Opening and (not Semicolon or Opening < Semicolon) then
                local Closing = Opening + 1
                local Depth = 1

                while Closing <= #Mask and Depth > 0 do
                    local Character = Mask:sub(Closing, Closing)
                    Depth = Depth + (Character == "{" and 1 or (Character == "}" and -1 or 0))
                    Closing = Closing + 1
                end

                assert(Depth == 0, File .. ": Unterminated type " .. Name)
                local Outer = nil

                for _, Type in ipairs(Types) do
                    if Opening > Type.Opening and Opening < Type.Closing then
                        Outer = Type
                    end
                end

                local QualifiedName = Outer and (Outer.Name .. "::" .. Name) or Name
                local Template = Mask:sub(1, Start - 1):match("template%s*%b<>%s*$") ~= nil or (Outer ~= nil and Outer.Template)

                Types[#Types + 1] = { Name = QualifiedName, ShortName = Name, Namespace = NamespaceAt(Namespaces, Start), File = File, Opening = Opening, Closing = Closing, Outer = Outer, Template = Template, Properties = {}, Members = {}, Flags = {} }
            end
        end
    end

    return Types
end

local function Trim(Text)
    return Text:match("^%s*(.-)%s*$")
end

local function SplitParameters(Text)
    local Parts = {}
    local Depth = 0
    local First = 1

    for Position = 1, #Text do
        local Character = Text:sub(Position, Position)
        Depth = Depth + ((Character == "<" or Character == "(") and 1 or ((Character == ">" or Character == ")") and -1 or 0))

        if Character == "," and Depth == 0 then
            Parts[#Parts + 1] = Text:sub(First, Position - 1)
            First = Position + 1
        end
    end

    if #Trim(Text:sub(First)) > 0 then
        Parts[#Parts + 1] = Text:sub(First)
    end

    return Parts
end

-- const와 참조는 경계에서 의미가 없으므로 지운 타입 이름만 남깁니다.
local function NormalizeType(Text)
    return Trim(Trim(Text:gsub("%f[%w_]const%f[^%w_]", "")):gsub("&$", ""))
end

-- 어노테이션 바로 앞의 선언을 읽습니다. 마스크에는 주석과 문자열이 지워져 있습니다.
local function ReadFunctionDeclaration(Mask, Call, Owner, File)
    local Name = Call.Arguments[1]
    local Label = File .. ": ReflectFunction(" .. tostring(Name) .. ")"
    local Before = Mask:sub(1, Call.Position - 1)
    local Finish = Before:match("^.*();%s*$")
    assert(Name and Finish, Label .. " must follow its member function declaration")

    local Declaration = Before:sub(1, Finish - 1)
    local Last = Declaration:match("^.*[;{}]()")
    Declaration = Declaration:sub(Last or 1)

    for _, Access in ipairs({ "public", "private", "protected" }) do
        Declaration = Declaration:gsub("^%s*" .. Access .. "%s*:", "")
    end

    local Start, _, Parameters = Declaration:find("%f[%w_]" .. Name .. "%s*(%b())")
    assert(Start, Label .. " does not match the preceding declaration")

    local Return = Declaration:sub(1, Start - 1)
    local Static = Return:match("%f[%w_]static%f[^%w_]") ~= nil

    for _, Keyword in ipairs({ "static", "virtual", "inline", "explicit" }) do
        Return = Return:gsub("%f[%w_]" .. Keyword .. "%f[^%w_]", "")
    end

    local Function = { Name = Name, Owner = Owner, File = File, Static = Static, Return = NormalizeType(Return), Parameters = {}, Internal = Call.Arguments[2] == "Internal", Label = Label }

    for _, Parameter in ipairs(SplitParameters(Parameters:sub(2, -2))) do
        local Type, ParameterName = Trim(Parameter:gsub("=.*$", "")):match("^(.-)%s+([%a_][%w_]*)$")
        assert(Type, Label .. ": Unnamed parameter " .. Parameter)
        Function.Parameters[#Function.Parameters + 1] = { Type = NormalizeType(Type), Name = ParameterName }
    end

    return Function
end

-- 값에 문자 리터럴이 올 수 있으므로 마스크로 범위만 찾고 원문을 읽습니다.
local function ReadScriptEnum(Text, Mask, Call, Namespace, File)
    local Name = Call.Arguments[1]
    local Start = Mask:find("%f[%w_]enum%s+class%s+" .. tostring(Name) .. "%f[^%w_]") or Mask:find("%f[%w_]enum%s+" .. tostring(Name) .. "%f[^%w_]")
    assert(Start, File .. ": Missing enum definition for " .. tostring(Name))

    local Opening, Closing = Mask:find("%b{}", Start)
    local Body = Text:sub(Opening + 1, Closing - 1):gsub("/%*.-%*/", ""):gsub("//[^\n]*", "")
    local Values = {}

    for _, Entry in ipairs(SplitParameters(Body)) do
        Values[#Values + 1] = Trim(Entry)
    end

    return { Name = Name, Native = Namespace .. "::" .. Name, Script = Name:gsub("^E", ""), File = File, Values = Values }
end

local function ReadTypes(File)
    local Text = assert(io.readfile(File))
    local Calls, Mask = ReadCalls(Text, File)
    local Types = ReadDefinitions(Mask, File)
    local Enums = {}
    local Functions = {}
    local ScriptEnums = {}
    local Namespaces = ReadNamespaces(Mask)

    for _, Call in ipairs(Calls) do
        local Owner = nil

        for _, Type in ipairs(Types) do
            if Call.Position > Type.Opening and Call.Position < Type.Closing then
                Owner = Type
            end
        end

        assert(not (Owner and Owner.Template), File .. ": Template types cannot use reflection: " .. Call.Name)

        if Call.Name == "ReflectEnum" then
            local Name = Call.Arguments[1]
            local Values = {}
            local Seen = {}
            assert(not Owner and Name and #Call.Arguments > 1, File .. ": Enum reflection requires a type and values")
            assert(Mask:find("enum%s+class%s+" .. Name .. "%f[^%w_]") or Mask:find("enum%s+" .. Name .. "%f[^%w_]"), File .. ": Missing enum definition for " .. Name)

            for Index = 2, #Call.Arguments do
                local Value = Call.Arguments[Index]
                assert(Value:match("^[%a_][%w_]*$") and not Seen[Value], File .. ": Invalid reflected enum value " .. Value)
                Seen[Value] = true
                Values[#Values + 1] = "ReflectEnumValue(" .. Name .. ", " .. Value .. ")"
            end

            Enums[#Enums + 1] = { Name = Name, ShortName = Name, Namespace = NamespaceAt(Namespaces, Call.Position), File = File, Kind = "ReflectEnum", Values = Values }
        elseif Call.Name == "ReflectTypes" then
            assert(Owner and not Owner.Owner and Owner.ShortName == Call.Arguments[1], File .. ": Inconsistent reflected owner")
            Owner.Owner = true
            Owner.Super = Call.Arguments[2]
        elseif Call.Name == "ReflectFunction" then
            assert(Owner, File .. ": ReflectFunction requires a named type")
            Functions[#Functions + 1] = ReadFunctionDeclaration(Mask, Call, Owner, File)
        elseif Call.Name == "ReflectScriptEnum" then
            ScriptEnums[#ScriptEnums + 1] = ReadScriptEnum(Text, Mask, Call, NamespaceAt(Namespaces, Call.Position), File)
        elseif Call.Name == "ReflectStruct" or Call.Name == "ReflectClass" or Call.Name == "ReflectRootClass" then
            assert(Owner and Owner.ShortName == Call.Arguments[1] and not Owner.Kind, File .. ": Inconsistent reflected type")
            Owner.Kind = Call.Name

            for Index = 2, #Call.Arguments do
                Owner.Flags[#Owner.Flags + 1] = Call.Arguments[Index]
            end
        elseif Call.Name == "ReflectMembers" then
            assert(Owner, File .. ": ReflectMembers requires a named type")
            Owner.HasMembers = true
        elseif PropertyArguments[Call.Name] then
            assert(Owner, File .. ": Property annotation requires a named type")
            assert(#Call.Arguments == PropertyArguments[Call.Name], File .. ": Invalid arguments for " .. Call.Name)
            assert(Mask:sub(1, Call.Position - 1):match("(%S)%s*$") == ";", File .. ": Property annotation must follow its member declaration")
            assert(not Owner.Members[Call.Arguments[1]], File .. ": Duplicate reflected member " .. Call.Arguments[1])
            Owner.Members[Call.Arguments[1]] = true
            Owner.Properties[#Owner.Properties + 1] = Call
        end
    end

    local Reflected = {}

    for _, Type in ipairs(Types) do
        if Type.Owner or Type.Kind or Type.HasMembers or #Type.Properties > 0 then
            assert(Type.Owner and Type.Kind and Type.HasMembers, File .. ": Incomplete reflection for " .. Type.Name)
            Reflected[#Reflected + 1] = Type
        end
    end

    for _, Enum in ipairs(Enums) do
        Reflected[#Reflected + 1] = Enum
    end

    return Reflected, Functions, ScriptEnums
end

local function PropertyStatement(Property)
    local Arguments = Property.Arguments
    local Pointers = { "&ReflectionOwner::" .. Arguments[1] }
    local Function = "RegisterProperty"

    if Property.Name == "ReflectAccessor" or Property.Name == "ReflectReadOnlyAccessor" or Property.Name == "ReflectArrayAccessor" then
        Pointers[#Pointers + 1] = "&ReflectionOwner::" .. Arguments[2]
    elseif Property.Name == "ReflectSetter" then
        Pointers[#Pointers + 1] = "nullptr"
    end

    if Property.Name == "ReflectAccessor" or Property.Name == "ReflectArrayAccessor" then
        Pointers[#Pointers + 1] = "&ReflectionOwner::" .. Arguments[3]
    elseif Property.Name == "ReflectSetter" then
        Pointers[#Pointers + 1] = "&ReflectionOwner::" .. Arguments[2]
    end

    if Property.Name == "ReflectReadOnlyProperty" or Property.Name == "ReflectReadOnlyAccessor" then
        Function = "RegisterReadOnlyProperty"
    end

    if Property.Name == "ReflectArray" or Property.Name == "ReflectArrayAccessor" then
        Function = "RegisterArrayProperty"
    end

    return "        CoreUObject::" .. Function .. "<" .. table.concat(Pointers, ", ") .. ">(Information, " .. Arguments[#Arguments - 1] .. ", " .. Arguments[#Arguments] .. ");"
end

local function WriteIfChanged(File, Lines)
    local Text = (table.concat(Lines, "\n") .. "\n"):gsub("\r\n", "\n"):gsub("\n", "\r\n")

    if io.readfile(File) ~= Text then
        os.mkdir(path.getdirectory(File))
        local Output = assert(io.open(File, "wb"))
        Output:write(Text)
        Output:close()
    end
end

-- 경계를 넘는 타입 규칙입니다. Source/Script/Private/ScriptAbi.h의 ToAbi, FromAbi와 맞아야 합니다.
local ScalarTypes = {
    ["float"] = "float",
    ["int"] = "int",
    ["Core::Int32"] = "int",
    ["Core::Uint32"] = "uint",
    ["Core::Uint64"] = "ulong",
}

local StructTypes = {
    ["Core::FVector2"] = { Abi = "Script::Abi::FVector2", Script = "Vector2" },
    ["Core::FVector"] = { Abi = "Script::Abi::FVector3", Script = "Vector3" },
    ["Core::FQuat"] = { Abi = "Script::Abi::FQuat", Script = "Quaternion" },
    ["Core::FTransform"] = { Abi = "Script::Abi::FTransform", Script = "Transform" },
    ["Engine::FWorldHit"] = { Abi = "Script::Abi::FWorldHit", Script = "RaycastHit" },
    ["Script::FScriptValue"] = { Abi = "Script::FScriptValue", Script = "ScriptValue" },
}

local function BindingShortName(Type)
    return Type:match("([%w_]+)$")
end

-- AActor, UActorComponent 계열과 UWorld만 C# 래퍼를 만듭니다. UScriptComponent는 C#의 ScriptComponent와 이름이 겹치고 노출할 내용도 없습니다.
local function CollectWrapperClasses(Types)
    local ByName = {}
    local Classes = {}

    for _, Type in ipairs(Types) do
        if Type.Kind == "ReflectClass" or Type.Kind == "ReflectRootClass" then
            ByName[Type.ShortName] = Type
        end
    end

    local function IsWrapped(Type)
        if Type.ShortName == "AActor" or Type.ShortName == "UActorComponent" or Type.ShortName == "UWorld" then
            return true
        end

        local Super = Type.Super and ByName[BindingShortName(Type.Super)]

        return Super ~= nil and IsWrapped(Super)
    end

    for _, Type in ipairs(Types) do
        if ByName[Type.ShortName] == Type and Type.SourceModule == "Engine" and IsWrapped(Type) then
            Classes[#Classes + 1] = Type
            Type.ScriptName = Type.ShortName:gsub("^[AU]", "")
            Type.ScriptSuper = (Type.ShortName == "AActor" or Type.ShortName == "UActorComponent" or Type.ShortName == "UWorld") and "EngineObject" or BindingShortName(Type.Super):gsub("^[AU]", "")
        end
    end

    return Classes, ByName
end

local function ResolveBindingType(Text, ByName, Enums, Label)
    local Scalar = ScalarTypes[Text]

    if Text == "bool" then
        return { Kind = "Bool", Abi = "Core::Uint8", Script = "bool", ScriptAbi = "byte" }
    elseif Scalar then
        return { Kind = "Scalar", Abi = Text, Script = Scalar, ScriptAbi = Scalar }
    elseif Text == "Core::FString" then
        return { Kind = "String", Abi = "const char*", Script = "string", ScriptAbi = "byte*" }
    elseif StructTypes[Text] then
        return { Kind = "Struct", Abi = StructTypes[Text].Abi, Script = StructTypes[Text].Script, ScriptAbi = StructTypes[Text].Script }
    elseif Enums[Text] then
        return { Kind = "Enum", Native = Text, Abi = "Core::Int32", Script = Enums[Text].Script, ScriptAbi = "int" }
    end

    local Element = Text:match("^Core::TArray<%s*(.-)%*%s*>$")
    local Pointer = Text:match("^(.-)%*$")
    local Object = ByName[BindingShortName(Trim(Element or Pointer or ""))]
    local Script = Object and (Object.ShortName == "UObject" and "EngineObject" or Object.ScriptName)
    assert(Script, Label .. ": Unsupported type " .. Text)

    return { Kind = Element and "ObjectArray" or "Object", Native = Trim(Element or Pointer), Class = Object, Abi = "CoreUObject::FObjectHandle", Script = Script, ScriptAbi = "ObjectHandle" }
end

local function WriteBindings(Types, Functions, Enums)
    local Classes, ByName = CollectWrapperClasses(Types)
    local EnumsByNative = {}
    local Includes = {}
    local IncludeOrder = {}
    local Signatures = {}
    local Thunks = {}
    local Entries = {}
    local Wrappers = {}
    local WrapperOrder = {}

    local function AddInclude(File)
        local Include = File:match("/Public/(.+)$")

        if Include and not Includes[Include] then
            Includes[Include] = true
            IncludeOrder[#IncludeOrder + 1] = Include
        end
    end

    for _, Enum in ipairs(Enums) do
        EnumsByNative[Enum.Native] = Enum
    end

    table.sort(Functions, function(A, B)
        return A.Owner.ShortName .. A.Name .. #A.Parameters < B.Owner.ShortName .. B.Name .. #B.Parameters
    end)

    for Index, Function in ipairs(Functions) do
        local Owner = Function.Owner
        local Native = Owner.Namespace .. "::" .. Owner.Name
        local ScriptClass = Owner.ShortName == "UObject" and "EngineObject" or (ByName[Owner.ShortName] and ByName[Owner.ShortName].ScriptName)
        AddInclude(Function.File)

        if not ScriptClass then
            assert(Function.Static and Owner.ShortName:match("^FScript"), Function.Label .. ": Only static functions on FScript* classes or AActor, UActorComponent and UWorld classes can be bound")
            ScriptClass = Owner.ShortName:gsub("^FScript", "")
        end

        local ThunkParameters = {}
        local Setup = {}
        local Arguments = {}
        local ScriptParameters = {}
        local ScriptAbiTypes = {}
        local ScriptArguments = {}
        local ScriptFixed = {}
        local ScriptLocals = {}
        local Signature = Native .. "::" .. Function.Name .. "("

        if not Function.Static then
            ThunkParameters[#ThunkParameters + 1] = "CoreUObject::FObjectHandle SelfHandle"
            ScriptAbiTypes[#ScriptAbiTypes + 1] = "ObjectHandle"
            ScriptArguments[#ScriptArguments + 1] = "Handle"
            Setup[#Setup + 1] = "        auto* Self{Script::Abi::Resolve<" .. Native .. ">(SelfHandle)};"
            Setup[#Setup + 1] = "        if (Self == nullptr) { return 0; }"
        end

        for ParameterIndex, Parameter in ipairs(Function.Parameters) do
            local Type = ResolveBindingType(Parameter.Type, ByName, EnumsByNative, Function.Label)
            local Argument = "Arg" .. ParameterIndex
            local Pass = "Pass" .. ParameterIndex
            Signature = Signature .. Parameter.Type .. ","
            assert(Type.Kind ~= "ObjectArray", Function.Label .. ": Arrays can only be returned")

            if Type.Kind == "Struct" then
                ThunkParameters[#ThunkParameters + 1] = "const " .. Type.Abi .. "* " .. Argument
                Arguments[#Arguments + 1] = "Script::Abi::FromAbi(*" .. Argument .. ")"
                ScriptAbiTypes[#ScriptAbiTypes + 1] = Type.ScriptAbi .. "*"
                ScriptLocals[#ScriptLocals + 1] = "        " .. Type.Script .. " " .. Pass .. " = " .. Parameter.Name .. ";"
                ScriptArguments[#ScriptArguments + 1] = "&" .. Pass
            else
                ThunkParameters[#ThunkParameters + 1] = Type.Abi .. " " .. Argument
                ScriptAbiTypes[#ScriptAbiTypes + 1] = Type.ScriptAbi

                if Type.Kind == "Bool" then
                    Arguments[#Arguments + 1] = Argument .. " != 0"
                    ScriptArguments[#ScriptArguments + 1] = "(byte)(" .. Parameter.Name .. " ? 1 : 0)"
                elseif Type.Kind == "String" then
                    Arguments[#Arguments + 1] = "Core::FString{" .. Argument .. "}"
                    ScriptFixed[#ScriptFixed + 1] = "        fixed (byte* " .. Pass .. " = Bindings.Utf8(" .. Parameter.Name .. "))"
                    ScriptArguments[#ScriptArguments + 1] = Pass
                elseif Type.Kind == "Enum" then
                    Arguments[#Arguments + 1] = "static_cast<" .. Type.Native .. ">(" .. Argument .. ")"
                    AddInclude(EnumsByNative[Type.Native].File)
                    ScriptArguments[#ScriptArguments + 1] = "(int)" .. Parameter.Name
                elseif Type.Kind == "Object" then
                    AddInclude(Type.Class.File)
                    Setup[#Setup + 1] = "        " .. Type.Native .. "* " .. Pass .. "{};"
                    Setup[#Setup + 1] = "        if (!Script::Abi::ResolveArgument(" .. Argument .. ", " .. Pass .. ")) { return 0; }"
                    Arguments[#Arguments + 1] = Pass
                    ScriptArguments[#ScriptArguments + 1] = "Bindings.HandleOf(" .. Parameter.Name .. ")"
                else
                    Arguments[#Arguments + 1] = Argument
                    ScriptArguments[#ScriptArguments + 1] = Parameter.Name
                end
            end

            ScriptParameters[#ScriptParameters + 1] = Type.Script .. (Type.Kind == "Object" and "? " or " ") .. Parameter.Name
        end

        local Call = (Function.Static and (Native .. "::") or "Self->") .. Function.Name .. "(" .. table.concat(Arguments, ", ") .. ")"
        local Body = {}
        local ScriptReturnType = "void"
        local ScriptReturn = {}
        Signature = Signature .. ")->" .. Function.Return

        if Function.Return == "void" then
            Body[#Body + 1] = "        " .. Call .. ";"
        else
            local Type = ResolveBindingType(Function.Return, ByName, EnumsByNative, Function.Label)
            ScriptReturnType = Type.Script

            if Type.Kind == "ObjectArray" then
                AddInclude(Type.Class.File)
                ThunkParameters[#ThunkParameters + 1] = "const CoreUObject::FObjectHandle** Result"
                ThunkParameters[#ThunkParameters + 1] = "Core::Int32* ResultCount"
                Body[#Body + 1] = "        *Result = Script::Abi::StoreObjects(" .. Call .. ", *ResultCount);"
                ScriptAbiTypes[#ScriptAbiTypes + 1] = "ObjectHandle**"
                ScriptAbiTypes[#ScriptAbiTypes + 1] = "int*"
                ScriptArguments[#ScriptArguments + 1] = "&Ret"
                ScriptArguments[#ScriptArguments + 1] = "&RetCount"
                ScriptLocals[#ScriptLocals + 1] = "        ObjectHandle* Ret = null;"
                ScriptLocals[#ScriptLocals + 1] = "        int RetCount = 0;"
                ScriptReturn[#ScriptReturn + 1] = "        return Bindings.WrapArray<" .. Type.Script .. ">(Ret, RetCount);"
                ScriptReturnType = Type.Script .. "[]"
            else
                local Assign = {
                    Bool = "static_cast<Core::Uint8>((" .. Call .. ") ? 1 : 0)",
                    Scalar = Call,
                    String = "Script::Abi::StoreString(" .. Call .. ")",
                    Struct = "Script::Abi::ToAbi(" .. Call .. ")",
                    Enum = "static_cast<Core::Int32>(" .. Call .. ")",
                    Object = "Script::Abi::ToHandle(" .. Call .. ")",
                }
                ThunkParameters[#ThunkParameters + 1] = Type.Abi .. "* Result"
                Body[#Body + 1] = "        *Result = " .. Assign[Type.Kind] .. ";"
                ScriptAbiTypes[#ScriptAbiTypes + 1] = Type.ScriptAbi .. "*"
                ScriptArguments[#ScriptArguments + 1] = "&Ret"
                ScriptLocals[#ScriptLocals + 1] = "        " .. Type.ScriptAbi .. " Ret = default;"

                if Type.Kind == "Object" then
                    AddInclude(Type.Class.File)
                    ScriptReturnType = Type.Script .. "?"
                    ScriptReturn[#ScriptReturn + 1] = "        return (" .. Type.Script .. "?)Bindings.Wrap(Ret);"
                elseif Type.Kind == "Bool" then
                    ScriptReturn[#ScriptReturn + 1] = "        return Ret != 0;"
                elseif Type.Kind == "String" then
                    ScriptReturn[#ScriptReturn + 1] = "        return Marshal.PtrToStringUTF8((nint)Ret)!;"
                elseif Type.Kind == "Enum" then
                    ScriptReturn[#ScriptReturn + 1] = "        return (" .. Type.Script .. ")Ret;"
                else
                    ScriptReturn[#ScriptReturn + 1] = "        return Ret;"
                end
            end
        end

        Signatures[#Signatures + 1] = Signature
        Entries[#Entries + 1] = "        reinterpret_cast<void*>(&Thunk" .. Index .. "),"
        Thunks[#Thunks + 1] = "    int __stdcall Thunk" .. Index .. "(" .. table.concat(ThunkParameters, ", ") .. ") {"

        for _, Line in ipairs(Setup) do
            Thunks[#Thunks + 1] = Line
        end

        for _, Line in ipairs(Body) do
            Thunks[#Thunks + 1] = Line
        end

        Thunks[#Thunks + 1] = "        return 1;"
        Thunks[#Thunks + 1] = "    }"
        Thunks[#Thunks + 1] = ""

        if not Wrappers[ScriptClass] then
            Wrappers[ScriptClass] = { Static = Function.Static and not ByName[Owner.ShortName], Internal = Function.Internal, Lines = {} }
            WrapperOrder[#WrapperOrder + 1] = ScriptClass
        end

        local Lines = Wrappers[ScriptClass].Lines
        ScriptAbiTypes[#ScriptAbiTypes + 1] = "int"
        Lines[#Lines + 1] = "    " .. (Function.Internal and "internal " or "public ") .. (Function.Static and "static " or "") .. ScriptReturnType .. " " .. Function.Name .. "(" .. table.concat(ScriptParameters, ", ") .. ") {"

        for _, Line in ipairs(ScriptLocals) do
            Lines[#Lines + 1] = Line
        end

        for _, Line in ipairs(ScriptFixed) do
            Lines[#Lines + 1] = Line
        end

        local Invoke = "((delegate* unmanaged<" .. table.concat(ScriptAbiTypes, ", ") .. ">)Bindings.sTable[" .. (Index - 1) .. "])(" .. table.concat(ScriptArguments, ", ") .. ")"

        if Function.Static then
            Lines[#Lines + 1] = "        " .. Invoke .. ";"
        else
            Lines[#Lines + 1] = "        if (" .. Invoke .. " == 0) { throw Bindings.Gone(\"" .. ScriptClass .. "." .. Function.Name .. "\"); }"
        end

        for _, Line in ipairs(ScriptReturn) do
            Lines[#Lines + 1] = Line
        end

        Lines[#Lines + 1] = "    }"
        Lines[#Lines + 1] = ""
    end

    -- 엔진과 Macaw.Engine이 서로 다른 생성 결과로 빌드되면 시작할 때 거부합니다.
    local Hash = 2166136261

    for _, Signature in ipairs(Signatures) do
        for Position = 1, #Signature do
            Hash = ((Hash ~ Signature:byte(Position)) * 16777619) & 0xFFFFFFFF
        end
    end

    table.sort(IncludeOrder)

    local Source = { '#include "ScriptAbi.h"' }

    for _, Include in ipairs(IncludeOrder) do
        Source[#Source + 1] = '#include "' .. Include .. '"'
    end

    Source[#Source + 1] = ""
    Source[#Source + 1] = "namespace {"
    Source[#Source + 1] = ""

    for _, Line in ipairs(Thunks) do
        Source[#Source + 1] = Line
    end

    Source[#Source + 1] = "    void* const sThunks[]{"

    for _, Line in ipairs(Entries) do
        Source[#Source + 1] = Line
    end

    Source[#Source + 1] = "        nullptr"
    Source[#Source + 1] = "    };"
    Source[#Source + 1] = ""
    Source[#Source + 1] = "}"
    Source[#Source + 1] = ""
    Source[#Source + 1] = "Script::Abi::FBindingTable Script::Abi::GetBindingTable() {"
    Source[#Source + 1] = "    return {sThunks, " .. #Functions .. ", " .. Hash .. "};"
    Source[#Source + 1] = "}"

    local Script = { "// <auto-generated/>", "#nullable enable", "using System.Numerics;", "using System.Runtime.InteropServices;", "", "namespace Macaw;", "", "internal static unsafe partial class Bindings {" }
    Script[#Script + 1] = "    public const int Count = " .. #Functions .. ";"
    Script[#Script + 1] = "    public const uint Hash = " .. Hash .. ";"
    Script[#Script + 1] = ""
    Script[#Script + 1] = "    private static readonly Dictionary<string, Func<ObjectHandle, EngineObject>> sFactories = new() {"

    for _, Class in ipairs(Classes) do
        Script[#Script + 1] = '        ["' .. Class.ShortName .. '"] = Handle => new ' .. Class.ScriptName .. "(Handle),"
    end

    Script[#Script + 1] = "    };"
    Script[#Script + 1] = ""
    Script[#Script + 1] = "    private static readonly Dictionary<Type, string> sNativeNames = new() {"

    for _, Class in ipairs(Classes) do
        Script[#Script + 1] = "        [typeof(" .. Class.ScriptName .. ')] = "' .. Class.ShortName .. '",'
    end

    Script[#Script + 1] = "    };"
    Script[#Script + 1] = "}"

    for _, Enum in ipairs(Enums) do
        Script[#Script + 1] = ""
        Script[#Script + 1] = "public enum " .. Enum.Script .. " {"

        for _, Value in ipairs(Enum.Values) do
            Script[#Script + 1] = "    " .. Value .. ","
        end

        Script[#Script + 1] = "}"
    end

    for _, Class in ipairs(Classes) do
        Script[#Script + 1] = ""
        Script[#Script + 1] = "public unsafe partial class " .. Class.ScriptName .. " : " .. Class.ScriptSuper .. " {"
        Script[#Script + 1] = "    internal " .. Class.ScriptName .. "(ObjectHandle Handle) : base(Handle) { }"

        if Wrappers[Class.ScriptName] then
            Script[#Script + 1] = ""

            for _, Line in ipairs(Wrappers[Class.ScriptName].Lines) do
                Script[#Script + 1] = Line
            end

            Script[#Script] = nil
        end

        Script[#Script + 1] = "}"
        Wrappers[Class.ScriptName] = nil
    end

    for _, Name in ipairs(WrapperOrder) do
        local Wrapper = Wrappers[Name]

        if Wrapper then
            Script[#Script + 1] = ""

            if Wrapper.Static then
                Script[#Script + 1] = (Wrapper.Internal and "internal" or "public") .. " static unsafe partial class " .. Name .. " {"
            else
                Script[#Script + 1] = "public abstract unsafe partial class " .. Name .. " {"
            end

            for _, Line in ipairs(Wrapper.Lines) do
                Script[#Script + 1] = Line
            end

            Script[#Script] = nil
            Script[#Script + 1] = "}"
        end
    end

    local Directory = Reflection.GetGeneratedDirectory("Script")
    WriteIfChanged(Directory .. "/ScriptBindings.cpp", Source)
    WriteIfChanged(Directory .. "/ScriptBindings.g.cs", Script)
end

function Reflection.Generate(SelectedModule)
    local Modules = {}
    local AllTypes = {}
    local AllFunctions = {}
    local AllEnums = {}
    local Files = os.matchfiles("Source/**.h")
    table.sort(Files)

    for _, File in ipairs(Files) do
        local Module = File:match("^Source/([^/]+)/")
        Module = Module == "Core" and "CoreUObject" or Module

        local Reflected, Functions, ScriptEnums = ReadTypes(File)

        for _, Type in ipairs(Reflected) do
            Modules[Module] = Modules[Module] or {}
            table.insert(Modules[Module], Type)
            Type.SourceModule = File:match("^Source/([^/]+)/")
            AllTypes[#AllTypes + 1] = Type
        end

        for _, Function in ipairs(Functions) do
            AllFunctions[#AllFunctions + 1] = Function
        end

        for _, Enum in ipairs(ScriptEnums) do
            AllEnums[#AllEnums + 1] = Enum
        end
    end

    for _, File in ipairs(os.matchfiles("Source/**.cpp")) do
        local Calls, Mask = ReadCalls(assert(io.readfile(File), File .. ": Cannot read reflection source"), File)
        assert(not Mask:match("%f[%a_]Implement[%w_]*Reflection%s*%("), File .. ": Reflection implementations must be generated from header annotations")

        for _, Call in ipairs(Calls) do
            assert(Call.Name ~= "ReflectEnum" and Call.Name ~= "ReflectTypes" and Call.Name ~= "ReflectStruct" and Call.Name ~= "ReflectClass" and Call.Name ~= "ReflectMembers" and Call.Name ~= "ReflectFunction" and Call.Name ~= "ReflectScriptEnum" and not PropertyArguments[Call.Name], File .. ": Move reflection annotations into headers: " .. Call.Name)
        end
    end

    Modules.App = Modules.App or {}
    Modules.Game = Modules.Game or {}

    if SelectedModule then
        Modules[SelectedModule] = Modules[SelectedModule] or {}
    end

    for Module, Types in pairs(Modules) do
        if not SelectedModule or Module == SelectedModule then
            local Directory = Reflection.GetGeneratedDirectory(Module)
            local Header = { "#pragma once", "" }
            local Source = { '#include "' .. Module .. 'Reflection.h"', '#include "CoreUObject/Reflection/ReflectionMacros.h"' }
            local Includes = {}

            for _, Type in ipairs(Types) do
                local Include = Type.File:match("/Public/(.+)$") or path.getrelative(path.getabsolute(Directory), path.getabsolute(Type.File))

                if not Includes[Include] then
                    Header[#Header + 1] = '#include "' .. Include .. '"'
                    Includes[Include] = true
                end

                if Type.Kind == "ReflectEnum" then
                    Source[#Source + 1] = ""
                    Source[#Source + 1] = "namespace " .. Type.Namespace .. " {"
                    Source[#Source + 1] = ""
                    Source[#Source + 1] = "    ImplementEnumReflection(" .. Type.Name .. ", " .. table.concat(Type.Values, ", ") .. ")"
                    Source[#Source + 1] = ""
                    Source[#Source + 1] = "}"
                else
                    local Implementation = Type.Kind == "ReflectStruct" and "ImplementStructReflection" or (Type.Kind == "ReflectRootClass" and "ImplementRootReflection" or "ImplementReflection")
                    local Flags = #Type.Flags > 0 and (", " .. table.concat(Type.Flags, ", ")) or ""
                    Source[#Source + 1] = ""
                    Source[#Source + 1] = "namespace " .. Type.Namespace .. " {"
                    Source[#Source + 1] = ""
                    Source[#Source + 1] = "    " .. Implementation .. "(" .. Type.Name .. Flags .. ")"

                    Source[#Source + 1] = ""
                    Source[#Source + 1] = "    void " .. Type.Name .. "::RegisterReflectionMembers(CoreUObject::FReflectionBuilder&" .. (#Type.Properties > 0 and " Information" or "") .. ") {"

                    for _, Child in ipairs(Types) do
                        if Child.Outer == Type then
                            Source[#Source + 1] = "        " .. Child.ShortName .. "::StaticStruct();"
                        end
                    end

                    for _, Property in ipairs(Type.Properties) do
                        Source[#Source + 1] = PropertyStatement(Property)
                    end

                    Source[#Source + 1] = "    }"
                    Source[#Source + 1] = ""
                    Source[#Source + 1] = "}"
                end
            end

            Header[#Header + 1] = ""
            Header[#Header + 1] = "namespace " .. Module .. "::ReflectionModules {"
            Header[#Header + 1] = "    void Register" .. Module .. "Types();"
            Header[#Header + 1] = "}"
            Source[#Source + 1] = ""
            Source[#Source + 1] = "void " .. Module .. "::ReflectionModules::Register" .. Module .. "Types() {"

            if Module == "App" then
                local Names = table.keys(Modules)
                table.sort(Names)

                for _, Name in ipairs(Names) do
                    if Name ~= "App" and Name ~= "Game" then
                        table.insert(Source, 3, '#include "' .. Name .. 'Reflection.h"')
                        Source[#Source + 1] = "    " .. Name .. "::ReflectionModules::Register" .. Name .. "Types();"
                    end
                end
            end

            for _, Type in ipairs(Types) do
                if not Type.Outer or not Type.Outer.Kind then
                    Source[#Source + 1] = "    " .. Type.Namespace .. "::" .. (Type.Kind == "ReflectEnum" and ("StaticEnum" .. Type.Name .. "();") or (Type.Name .. (Type.Kind == "ReflectStruct" and "::StaticStruct();" or "::StaticClass();")))
                end
            end

            Source[#Source + 1] = "}"
            WriteIfChanged(Directory .. "/" .. Module .. "Reflection.h", Header)
            WriteIfChanged(Directory .. "/" .. Module .. "Reflection.cpp", Source)
        end
    end

    if not SelectedModule or SelectedModule == "Script" then
        WriteBindings(AllTypes, AllFunctions, AllEnums)
    end

    return Modules
end

return Reflection
