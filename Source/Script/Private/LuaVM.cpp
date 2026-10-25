#include "Script/LuaVM.h"

#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Engine/AActor.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/UWorld.h"

#include <Windows.h>
#include <filesystem>
#include <format>
#include <luajit.h>
#include <utility>

namespace {
    FString LuaValueToString(const sol::object& Value) {
        switch (Value.get_type()) {
        case sol::type::nil:
            return "nil";
        case sol::type::string:
            return Value.as<FString>();
        case sol::type::number:
            return std::format("{}", Value.as<double>());
        case sol::type::boolean:
            return Value.as<bool>() ? "true" : "false";
        default:
            return "<value>";
        }
    }

    void SetVector(FVector& Vector, float X, float Y, float Z) {
        Vector = FVector{X, Y, Z};
    }

    FVector AddVector(const FVector& A, const FVector& B) {
        return A + B;
    }

    FVector SubtractVector(const FVector& A, const FVector& B) {
        return A - B;
    }

    FVector MultiplyVector(const FVector& Vector, float Scalar) {
        return Vector * Scalar;
    }

    bool EqualVector(const FVector& A, const FVector& B) {
        return A == B;
    }

    void BindVectorAPI(sol::state& LuaState) {
        LuaState.new_usertype<FVector>("FVector", sol::constructors<FVector(), FVector(float, float, float)>{}, "X", &FVector::x, "Y", &FVector::y, "Z", &FVector::z, "Set", &SetVector, "Size", &FVector::Length, "SizeSquared", &FVector::LengthSquared, "Length", &FVector::Length, "Normalized", &FVector::Normalized, "Add", &AddVector, "Subtract", &SubtractVector, "Dot", &FVector::Dot, "Cross", &FVector::Cross, "Distance", &FVector::Distance, sol::meta_function::addition, &AddVector, sol::meta_function::subtraction, &SubtractVector, sol::meta_function::multiplication, &MultiplyVector, sol::meta_function::equal_to, &EqualVector);
    }

    void BindRotatorAPI(sol::state& LuaState) {
        LuaState.new_usertype<FRotator>("FRotator", sol::constructors<FRotator(), FRotator(float, float, float)>{}, "Pitch", &FRotator::pitch, "Yaw", &FRotator::yaw, "Roll", &FRotator::roll);
    }

    FString GetActorName(const AActor& Actor) {
        return Actor.GetName();
    }

    FVector GetActorLocation(const AActor& Actor) {
        const USceneComponent* RootComponent{Actor.GetRootComponent()};
        return RootComponent != nullptr ? RootComponent->GetTransform().mLocation : FVector{};
    }

    bool SetActorLocation(AActor& Actor, const FVector& NewLocation) {
        USceneComponent* RootComponent{Actor.GetRootComponent()};
        if (RootComponent == nullptr) {
            return false;
        }

        FTransform Transform{RootComponent->GetTransform()};
        Transform.mLocation = NewLocation;
        return RootComponent->SetTransform(Transform);
    }

    FRotator GetActorRotation(const AActor& Actor) {
        const USceneComponent* RootComponent{Actor.GetRootComponent()};
        return RootComponent != nullptr ? RootComponent->GetTransform().GetRotator() : FRotator{};
    }

    bool SetActorRotation(AActor& Actor, const FRotator& NewRotation) {
        USceneComponent* RootComponent{Actor.GetRootComponent()};
        if (RootComponent == nullptr) {
            return false;
        }

        FTransform Transform{RootComponent->GetTransform()};
        return Transform.SetRotator(NewRotation) && RootComponent->SetTransform(Transform);
    }

    FVector GetActorScale3D(const AActor& Actor) {
        const USceneComponent* RootComponent{Actor.GetRootComponent()};
        return RootComponent != nullptr ? RootComponent->GetTransform().mScale : FVector{1.0f, 1.0f, 1.0f};
    }

    bool SetActorScale3D(AActor& Actor, const FVector& NewScale) {
        USceneComponent* RootComponent{Actor.GetRootComponent()};
        if (RootComponent == nullptr) {
            return false;
        }

        FTransform Transform{RootComponent->GetTransform()};
        Transform.mScale = NewScale;
        return RootComponent->SetTransform(Transform);
    }

    bool DestroyActor(AActor& Actor) {
        UWorld* World{Actor.GetWorld()};
        return World != nullptr && World->DestroyActor(Actor.GetHandle());
    }

    void BindActorAPI(sol::state& LuaState) {
        LuaState.new_usertype<AActor>("AActor", sol::no_constructor, "GetName", &GetActorName, "GetActorLocation", &GetActorLocation, "SetActorLocation", &SetActorLocation, "GetActorRotation", &GetActorRotation, "SetActorRotation", &SetActorRotation, "GetActorScale3D", &GetActorScale3D, "SetActorScale3D", &SetActorScale3D, "Destroy", &DestroyActor);
    }
}

FLuaVM::~FLuaVM() {
    Shutdown();
}

bool FLuaVM::Initialize() {
    if (mState != nullptr) {
        return true;
    }

    try {
        mState = std::make_unique<sol::state>();

        // 게임 스크립트에 필요한 표준 라이브러리만 엽니다.
        // os, io, debug는 파일 및 시스템 접근이 가능하므로 우선 제외합니다.
        mState->open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string, sol::lib::table, sol::lib::math);
        BindEngineAPI();
        WriteLog(std::format("[Lua] VM initialized with {}", LUAJIT_VERSION));
        return true;
    } catch (const std::exception& Exception) {
        WriteLog(std::format("[Lua] VM initialization failed: {}", Exception.what()));
        mState.reset();
        return false;
    }
}

void FLuaVM::Shutdown() {
    if (mState == nullptr) {
        return;
    }

    // sol::state의 소멸자가 내부 lua_State에 lua_close를 호출합니다.
    mState.reset();
}

bool FLuaVM::ExecuteString(const FString& Source, const FString& ChunkName) {
    if (mState == nullptr) {
        WriteLog(std::format("[Lua] Cannot execute '{}': VM is not initialized", ChunkName));
        return false;
    }

    sol::protected_function_result Result{mState->safe_script(Source, sol::script_pass_on_error, ChunkName)};
    return CheckLuaResult(Result, ChunkName);
}

bool FLuaVM::ExecuteFile(const FString& ScriptPath) {
    if (mState == nullptr) {
        WriteLog(std::format("[Lua] Cannot execute '{}': VM is not initialized", ScriptPath));
        return false;
    }

    if (!std::filesystem::exists(ScriptPath)) {
        WriteLog(std::format("[Lua] Script file not found: {}", ScriptPath));
        return false;
    }

    sol::protected_function_result Result{mState->safe_script_file(ScriptPath, sol::script_pass_on_error)};
    return CheckLuaResult(Result, ScriptPath);
}

bool FLuaVM::IsInitialized() const {
    return mState != nullptr;
}

sol::state* FLuaVM::GetState() {
    return mState.get();
}

const sol::state* FLuaVM::GetState() const {
    return mState.get();
}

void FLuaVM::SetLogFunction(FLogFunction Function) {
    mLogFunction = std::move(Function);
}

void FLuaVM::BindEngineAPI() {
    if (mState == nullptr) {
        return;
    }

    BindLoggingAPI();
    BindVectorAPI(*mState);
    BindRotatorAPI(*mState);
    BindActorAPI(*mState);
}

void FLuaVM::BindLoggingAPI() {
    const auto LogFunction{[this](sol::variadic_args Args) {
        FString Message{};

        for (const auto& Argument : Args) {
            if (!Message.empty()) {
                Message += " ";
            }

            const sol::object Value{Argument};
            Message += LuaValueToString(Value);
        }

        WriteLog(std::format("[Lua] {}", Message));
    }};

    // Lua에서 Log("Score:", 100, true)처럼 호출할 수 있습니다.
    mState->set_function("Log", LogFunction);

    // 기본 Lua print도 에디터 Output Log 쪽으로 연결합니다.
    mState->set_function("print", LogFunction);
}

bool FLuaVM::CheckLuaResult(sol::protected_function_result& Result, const FString& Context) const {
    if (Result.valid()) {
        return true;
    }

    const sol::error Error{Result.get<sol::error>()};
    WriteLog(std::format("[Lua] {} failed: {}", Context, Error.what()));
    return false;
}

void FLuaVM::WriteLog(const FString& Message) const {
    if (mLogFunction) {
        mLogFunction(Message);
    }

    const FString DebugMessage{Message + "\n"};
    OutputDebugStringA(DebugMessage.c_str());
}
