local Reflection = {}

local PropertyArguments = { ReflectProperty = 3, ReflectAccessor = 5, ReflectSetter = 4, ReflectReadOnlyProperty = 3, ReflectReadOnlyAccessor = 4 }

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
                local Template = Mask:sub(1, Start - 1):match("(template%s*<[^<>]*>)%s*$")
                local TemplateName = QualifiedName

                if Template then
                    local Declaration = Mask:sub(Position, Opening - 1)
                    local _, NameEnd = Declaration:find(Name, 1, true)
                    local Specialization = Declaration:sub(NameEnd + 1):match("^%s*(%b<>)")
                    local Arguments = {}

                    for Parameter in Template:match("<(.*)>"):gmatch("[^,]+") do
                        Arguments[#Arguments + 1] = assert(Parameter:match("([%a_][%w_]*)%s*$"), File .. ": Unsupported template parameter")
                    end

                    TemplateName = QualifiedName .. (Specialization or ("<" .. table.concat(Arguments, ", ") .. ">"))
                    Template = Template:gsub("%s+", " ")
                end

                Types[#Types + 1] = { Name = QualifiedName, ShortName = Name, Namespace = NamespaceAt(Namespaces, Start), File = File, Opening = Opening, Closing = Closing, Outer = Outer, Template = Template, TemplateName = TemplateName, Properties = {}, Members = {}, Flags = {} }
            end
        end
    end

    return Types
end

local function ReadTypes(File)
    local Text = assert(io.readfile(File))
    local Calls, Mask = ReadCalls(Text, File)
    local Types = ReadDefinitions(Mask, File)
    local Enums = {}
    local Namespaces = ReadNamespaces(Mask)

    for _, Call in ipairs(Calls) do
        local Owner = nil

        for _, Type in ipairs(Types) do
            if Call.Position > Type.Opening and Call.Position < Type.Closing then
                Owner = Type
            end
        end

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
        elseif Call.Name == "ReflectStruct" or Call.Name == "ReflectClass" or Call.Name == "ReflectRootClass" or Call.Name == "ReflectTemplate" then
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

    for _, Type in ipairs(Types) do
        assert(Type.Owner and Type.Kind and Type.HasMembers, File .. ": Missing reflection for " .. Type.Name)
        assert((Type.Kind == "ReflectTemplate") == (Type.Template ~= nil), File .. ": Inconsistent template reflection for " .. Type.Name)
    end

    for Position, Line in Mask:gmatch("()([^\n]+);") do
        if not Line:match("^%s*Reflect") then
            local Member = Line:match("%f[%a_](m%u[%w_]*)") or Line:match("^%s*float%s+([xyzw])%s*$") or Line:match("^%s*float%s+(pitch)%s*$") or Line:match("^%s*float%s+(yaw)%s*$") or Line:match("^%s*float%s+(roll)%s*$")
            local Owner = nil

            for _, Type in ipairs(Types) do
                if Position > Type.Opening and Position < Type.Closing then
                    Owner = Type
                end
            end

            if Member and Owner then
                assert(Owner.Members[Member], File .. ": Missing property reflection for " .. Owner.Name .. "::" .. Member)
            end
        end
    end

    for _, Enum in ipairs(Enums) do
        Types[#Types + 1] = Enum
    end

    return Types
end

local function PropertyStatement(Property)
    local Arguments = Property.Arguments
    local Pointers = { "&ReflectionOwner::" .. Arguments[1] }
    local Function = "RegisterProperty"

    if Property.Name == "ReflectAccessor" or Property.Name == "ReflectReadOnlyAccessor" then
        Pointers[#Pointers + 1] = "&ReflectionOwner::" .. Arguments[2]
    elseif Property.Name == "ReflectSetter" then
        Pointers[#Pointers + 1] = "nullptr"
    end

    if Property.Name == "ReflectAccessor" then
        Pointers[#Pointers + 1] = "&ReflectionOwner::" .. Arguments[3]
    elseif Property.Name == "ReflectSetter" then
        Pointers[#Pointers + 1] = "&ReflectionOwner::" .. Arguments[2]
    end

    if Property.Name == "ReflectReadOnlyProperty" or Property.Name == "ReflectReadOnlyAccessor" then
        Function = "RegisterReadOnlyProperty"
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

local function GenerateTemplate(Type, Directory)
    local Header = { "#pragma once", "", "namespace " .. Type.Namespace .. " {", "", "    " .. Type.Template, "    CoreUObject::UScriptStruct* " .. Type.TemplateName .. "::StaticStruct() {", "        return CoreUObject::DefineTemplateStruct<ReflectionOwner>(__FUNCSIG__);", "    }", "", "    " .. Type.Template, "    void " .. Type.TemplateName .. "::RegisterReflectionMembers(CoreUObject::FReflectionBuilder&" .. (#Type.Properties > 0 and " Information" or "") .. ") {" }

    for _, Property in ipairs(Type.Properties) do
        Header[#Header + 1] = PropertyStatement(Property)
    end

    Header[#Header + 1] = "    }"
    Header[#Header + 1] = ""
    Header[#Header + 1] = "}"
    WriteIfChanged(Directory .. "/" .. Type.ShortName .. "Reflection.generated.h", Header)
end

function Reflection.Generate(SelectedModule)
    local Modules = {}
    local Files = os.matchfiles("Source/**.h")
    table.sort(Files)

    for _, File in ipairs(Files) do
        local Module = File:match("^Source/([^/]+)/")
        Module = Module == "Core" and "CoreUObject" or Module

        for _, Type in ipairs(ReadTypes(File)) do
            Modules[Module] = Modules[Module] or {}
            table.insert(Modules[Module], Type)
        end
    end

    for _, File in ipairs(os.matchfiles("Source/**.cpp")) do
        local Calls, Mask = ReadCalls(assert(io.readfile(File)), File)
        assert(#ReadDefinitions(Mask, File) == 0, File .. ": Move named class definitions into headers so reflection can be generated")
        assert(not Mask:match("%f[%a_]Implement[%w_]*Reflection%s*%("), File .. ": Reflection implementations must be generated from header annotations")

        for _, Call in ipairs(Calls) do
            assert(Call.Name ~= "ReflectEnum", File .. ": Move enum reflection annotations into headers")
        end
    end

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

                if Type.Kind == "ReflectTemplate" then
                    GenerateTemplate(Type, Directory)
                elseif Type.Kind == "ReflectEnum" then
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
                        if Child.Outer == Type and Child.Kind ~= "ReflectTemplate" then
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
                    if Name ~= "App" then
                        table.insert(Source, 3, '#include "' .. Name .. 'Reflection.h"')
                        Source[#Source + 1] = "    " .. Name .. "::ReflectionModules::Register" .. Name .. "Types();"
                    end
                end
            end

            for _, Type in ipairs(Types) do
                if not Type.Outer and Type.Kind ~= "ReflectTemplate" then
                    Source[#Source + 1] = "    " .. Type.Namespace .. "::" .. (Type.Kind == "ReflectEnum" and ("StaticEnum" .. Type.Name .. "();") or (Type.Name .. (Type.Kind == "ReflectStruct" and "::StaticStruct();" or "::StaticClass();")))
                end
            end

            Source[#Source + 1] = "}"
            WriteIfChanged(Directory .. "/" .. Module .. "Reflection.h", Header)
            WriteIfChanged(Directory .. "/" .. Module .. "Reflection.cpp", Source)
        end
    end

    return Modules
end

return Reflection
