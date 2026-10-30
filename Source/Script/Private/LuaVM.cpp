/**
 * @file LuaVM.cpp
 * @brief Lua 상태의 수명, 엔진 API 바인딩과 스크립트 실행에 대한 구현입니다.
 */

#include "Script/LuaVM.h"
#include "Core/Stats.h"

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

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief Lua 문자열 실행 시간를 측정하는 통계 항목입니다.
     */
    FStat ExecuteStringTime{"script", "Execute string CPU", EStatUnit::Milliseconds};
    /**
     * @brief Lua 파일 실행 시간를 측정하는 통계 항목입니다.
     */
    FStat ExecuteFileTime{"script", "Execute file CPU", EStatUnit::Milliseconds};
    /**
     * @brief Lua 실행 오류 횟수를 측정하는 통계 항목입니다.
     */
    FStat LuaErrors{"script", "Lua errors"};
    /**
     * @brief Lua 기본 타입의 값을 로그에 표시할 문자열로 변환합니다.
     * @param[in] Value 읽거나 기록·변환할 값입니다.
     * @return 입력 값을 변환한 문자열입니다.
     */
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

    /**
     * @brief 벡터의 X·Y·Z 성분을 지정한 값으로 교체합니다.
     * @param[in,out] Vector 성분을 설정하거나 스칼라 연산에 사용할 벡터입니다.
     * @param[in] X 벡터의 X 성분입니다.
     * @param[in] Y 벡터의 Y 성분입니다.
     * @param[in] Z 벡터의 Z 성분입니다.
     */
    void SetVector(FVector& Vector, float X, float Y, float Z) {
        Vector = FVector{X, Y, Z};
    }

    /**
     * @brief 두 벡터의 대응 성분을 더한 벡터를 반환합니다.
     * @param[in] A 연산의 첫 번째 벡터입니다.
     * @param[in] B 연산의 두 번째 벡터입니다.
     * @return 계산하거나 조회한 벡터 또는 회전 값입니다.
     */
    FVector AddVector(const FVector& A, const FVector& B) {
        return A + B;
    }

    /**
     * @brief 첫 벡터에서 둘째 벡터의 대응 성분을 뺀 벡터를 반환합니다.
     * @param[in] A 연산의 첫 번째 벡터입니다.
     * @param[in] B 연산의 두 번째 벡터입니다.
     * @return 계산하거나 조회한 벡터 또는 회전 값입니다.
     */
    FVector SubtractVector(const FVector& A, const FVector& B) {
        return A - B;
    }

    /**
     * @brief 벡터의 각 성분에 스칼라를 곱한 벡터를 반환합니다.
     * @param[in] Vector 성분을 설정하거나 스칼라 연산에 사용할 벡터입니다.
     * @param[in] Scalar 벡터의 각 성분에 곱할 스칼라입니다.
     * @return 계산하거나 조회한 벡터 또는 회전 값입니다.
     */
    FVector MultiplyVector(const FVector& Vector, float Scalar) {
        return Vector * Scalar;
    }

    /**
     * @brief 두 벡터의 성분이 같은지 비교합니다.
     * @param[in] A 연산의 첫 번째 벡터입니다.
     * @param[in] B 연산의 두 번째 벡터입니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    bool EqualVector(const FVector& A, const FVector& B) {
        return A == B;
    }

    /**
     * @brief 벡터 생성·성분 접근·연산 API를 Lua에 등록합니다.
     * @param[in,out] LuaState 엔진 API를 등록할 Lua 상태입니다.
     */
    void BindVectorAPI(sol::state& LuaState) {
        LuaState.new_usertype<FVector>("FVector", sol::constructors<FVector(), FVector(float, float, float)>{}, "X", &FVector::x, "Y", &FVector::y, "Z", &FVector::z, "Set", &SetVector, "Size", &FVector::Length, "SizeSquared", &FVector::LengthSquared, "Length", &FVector::Length, "Normalized", &FVector::Normalized, "Add", &AddVector, "Subtract", &SubtractVector, "Dot", &FVector::Dot, "Cross", &FVector::Cross, "Distance", &FVector::Distance, sol::meta_function::addition, &AddVector, sol::meta_function::subtraction, &SubtractVector, sol::meta_function::multiplication, &MultiplyVector, sol::meta_function::equal_to, &EqualVector);
    }

    /**
     * @brief 오일러 회전 타입과 Pitch·Yaw·Roll 성분을 Lua에 등록합니다.
     * @param[in,out] LuaState 엔진 API를 등록할 Lua 상태입니다.
     */
    void BindRotatorAPI(sol::state& LuaState) {
        LuaState.new_usertype<FRotator>("FRotator", sol::constructors<FRotator(), FRotator(float, float, float)>{}, "Pitch", &FRotator::pitch, "Yaw", &FRotator::yaw, "Roll", &FRotator::roll);
    }

    /**
     * @brief Lua에서 조회할 액터의 이름을 복사하여 반환합니다.
     * @param[in] Actor 조회하거나 변경할 액터이며 소유권을 이전하지 않습니다.
     * @return 입력 값을 변환한 문자열입니다.
     */
    FString GetActorName(const AActor& Actor) {
        return Actor.GetName();
    }

    /**
     * @brief 루트 컴포넌트의 위치를 반환하며 루트가 없으면 영벡터를 반환합니다.
     * @param[in] Actor 조회하거나 변경할 액터이며 소유권을 이전하지 않습니다.
     * @return 계산하거나 조회한 벡터 또는 회전 값입니다.
     */
    FVector GetActorLocation(const AActor& Actor) {
        const USceneComponent* RootComponent{Actor.GetRootComponent()};
        return RootComponent != nullptr ? RootComponent->GetTransform().mLocation : FVector{};
    }

    /**
     * @brief 액터 루트 컴포넌트의 위치를 변경합니다.
     * @param[in,out] Actor 조회하거나 변경할 액터이며 소유권을 이전하지 않습니다.
     * @param[in] NewLocation 액터 루트 컴포넌트에 설정할 위치입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool SetActorLocation(AActor& Actor, const FVector& NewLocation) {
        USceneComponent* RootComponent{Actor.GetRootComponent()};
        if (RootComponent == nullptr) {
            return false;
        }

        FTransform Transform{RootComponent->GetTransform()};
        Transform.mLocation = NewLocation;
        return RootComponent->SetTransform(Transform);
    }

    /**
     * @brief 루트 컴포넌트의 회전을 반환하며 루트가 없으면 기본 회전을 반환합니다.
     * @param[in] Actor 조회하거나 변경할 액터이며 소유권을 이전하지 않습니다.
     * @return 계산하거나 조회한 벡터 또는 회전 값입니다.
     */
    FRotator GetActorRotation(const AActor& Actor) {
        const USceneComponent* RootComponent{Actor.GetRootComponent()};
        return RootComponent != nullptr ? RootComponent->GetTransform().GetRotator() : FRotator{};
    }

    /**
     * @brief 오일러 회전을 검증하여 액터 루트 컴포넌트에 설정합니다.
     * @param[in,out] Actor 조회하거나 변경할 액터이며 소유권을 이전하지 않습니다.
     * @param[in] NewRotation 액터 루트 컴포넌트에 설정할 오일러 회전입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool SetActorRotation(AActor& Actor, const FRotator& NewRotation) {
        USceneComponent* RootComponent{Actor.GetRootComponent()};
        if (RootComponent == nullptr) {
            return false;
        }

        FTransform Transform{RootComponent->GetTransform()};
        return Transform.SetRotator(NewRotation) && RootComponent->SetTransform(Transform);
    }

    /**
     * @brief 루트 컴포넌트의 배율을 반환하며 루트가 없으면 단위 배율을 반환합니다.
     * @param[in] Actor 조회하거나 변경할 액터이며 소유권을 이전하지 않습니다.
     * @return 계산하거나 조회한 벡터 또는 회전 값입니다.
     */
    FVector GetActorScale3D(const AActor& Actor) {
        const USceneComponent* RootComponent{Actor.GetRootComponent()};
        return RootComponent != nullptr ? RootComponent->GetTransform().mScale : FVector{1.0f, 1.0f, 1.0f};
    }

    /**
     * @brief 액터 루트 컴포넌트의 배율을 변경합니다.
     * @param[in,out] Actor 조회하거나 변경할 액터이며 소유권을 이전하지 않습니다.
     * @param[in] NewScale 액터 루트 컴포넌트에 설정할 축별 배율입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool SetActorScale3D(AActor& Actor, const FVector& NewScale) {
        USceneComponent* RootComponent{Actor.GetRootComponent()};
        if (RootComponent == nullptr) {
            return false;
        }

        FTransform Transform{RootComponent->GetTransform()};
        Transform.mScale = NewScale;
        return RootComponent->SetTransform(Transform);
    }

    /**
     * @brief 액터가 속한 월드에 해당 액터의 삭제를 요청합니다.
     * @param[in,out] Actor 조회하거나 변경할 액터이며 소유권을 이전하지 않습니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool DestroyActor(AActor& Actor) {
        UWorld* World{Actor.GetWorld()};
        return World != nullptr && World->DestroyActor(Actor.GetHandle());
    }

    /**
     * @brief 액터의 이름·변환·삭제 API를 Lua에 등록합니다.
     * @param[in,out] LuaState 엔진 API를 등록할 Lua 상태입니다.
     */
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
    const FScopedStatTimer StatTimer{ExecuteStringTime};
    if (mState == nullptr) {
        WriteLog(std::format("[Lua] Cannot execute '{}': VM is not initialized", ChunkName));
        return false;
    }

    sol::protected_function_result Result{mState->safe_script(Source, sol::script_pass_on_error, ChunkName)};
    return CheckLuaResult(Result, ChunkName);
}

bool FLuaVM::ExecuteFile(const FString& ScriptPath) {
    const FScopedStatTimer StatTimer{ExecuteFileTime};
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
    LuaErrors.Add();
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
