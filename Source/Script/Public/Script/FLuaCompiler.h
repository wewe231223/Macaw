#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <luajit/lua.hpp>

namespace Script {
    class FLuaCompiler {
    public:
        FLuaCompiler() = default;
        ~FLuaCompiler();
        FLuaCompiler(const Script::FLuaCompiler&) = delete;
        Script::FLuaCompiler& operator=(const Script::FLuaCompiler&) = delete;

    public:
        bool CheckSyntax(const std::filesystem::path& Path, std::string_view Source, std::string& Error);

    private:
        lua_State* mState{};
    };
}
