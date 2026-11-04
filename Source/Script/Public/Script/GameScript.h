#pragma once

#include "Script/LuaVM.h"

#include <filesystem>

namespace Script {

    class FGameScript final {
    public:
        bool Start(const std::filesystem::path& ContentRoot, const Core::FInput& Input, Core::FString& Error, FLuaVM::FLogFunction Log = {});
        void Stop();

        bool Tick(float DeltaTime);

    private:
        FLuaVM mVM{};
        bool mHasTick{};
        bool mTickFailed{};
    };

}
