#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <functional>
#include <sol/sol.hpp>

class FLuaVM final {
public:
    ReflectTypes(FLuaVM);
    using FLogFunction = std::function<void(const FString&)>;

public:
    FLuaVM() = default;
    ~FLuaVM();
    FLuaVM(const FLuaVM&) = delete;
    FLuaVM& operator=(const FLuaVM&) = delete;
    FLuaVM(FLuaVM&&) = delete;
    FLuaVM& operator=(FLuaVM&&) = delete;

public:
    ReflectStruct(FLuaVM);
    bool Initialize();
    void Shutdown();
    bool ExecuteString(const FString& Source, const FString& ChunkName = "@StringChunk");
    bool ExecuteFile(const FString& ScriptPath);
    bool IsInitialized() const;
    sol::state* GetState();
    const sol::state* GetState() const;
    void SetLogFunction(FLogFunction Function);

private:
    ReflectMembers();
    void BindEngineAPI();
    void BindLoggingAPI();
    bool CheckLuaResult(sol::protected_function_result& Result, const FString& Context) const;
    void WriteLog(const FString& Message) const;

private:
    std::unique_ptr<sol::state> mState{};
    ReflectProperty(mState, "State", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FLogFunction mLogFunction{};
    ReflectProperty(mLogFunction, "LogFunction", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
