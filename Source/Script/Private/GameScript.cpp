#include "Script/GameScript.h"

#include <utility>

namespace Script {

    bool FGameScript::Start(const std::filesystem::path& ContentRoot, const Core::FInput& Input, Core::FString& Error, FLuaVM::FLogFunction Log) {
        Stop();

        const std::filesystem::path Path{ContentRoot / "Scripts/Main.lua"};
        std::error_code FileError{};
        const bool Exists{std::filesystem::exists(Path, FileError)};

        if (FileError) {
            Error = "Cannot check the project script Content/Scripts/Main.lua";
            return false;
        }

        if (!Exists) {
            return true;
        }

        mVM.SetInput(&Input);
        mVM.SetLogFunction(std::move(Log));

        if (!mVM.Initialize() || !mVM.ExecuteFile(Path)) {
            Error = "Cannot run the project script Content/Scripts/Main.lua";
            Stop();
            return false;
        }

        mHasTick = (*mVM.GetState())["Tick"].get_type() == sol::type::function;

        return true;
    }

    void FGameScript::Stop() {
        mVM.Shutdown();
        mVM.SetInput(nullptr);
        mVM.SetLogFunction({});
        mHasTick = false;
        mTickFailed = false;
    }

    bool FGameScript::Tick(float DeltaTime) {
        if (mHasTick && !mTickFailed) {
            mTickFailed = !mVM.Tick(DeltaTime);
        }

        return mHasTick;
    }

}
