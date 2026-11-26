#include "FLuaRuntime.h"

#include "Core/Base/ErrorHandler.h"
#include "Core/FilePath.h"
#include "Engine/AActor.h"
#include "Engine/Component/ULuaScriptComponent.h"

#include <utility>

using namespace Engine;
using namespace Script;

FLuaRuntime::FLuaRuntime(Engine::UWorld& World)
    : mWorld{World} {
    RegisterBindings(mLuaVM.GetState());
}

FLuaRuntime::~FLuaRuntime() {
    // 플레이가 끝나면 스크립트가 틀어 둔 배경 음악도 끕니다.
    StopLuaMusic();

    for (auto& Entry : mInstances) {
        auto& Instance{Entry.second};
        if (auto* Component{Instance.mComponent.Get()}; Component != nullptr && Component->mRuntimeInstance == &Instance) {
            Component->mRuntimeInstance = nullptr;
        }
        Instance.DisconnectCollision();
    }
}

void FLuaScriptInstance::DisconnectCollision() {
    if (auto* Owner{mOwner.Get()}; Owner != nullptr && mCollisionHandle != 0) {
        Owner->GetOnComponentCollision().Remove(mCollisionHandle);
    }
    mCollisionHandle = {};
}

Engine::FLuaScriptInstance* FLuaRuntime::Start(Engine::ULuaScriptComponent& Component, const std::filesystem::path& Path) {
    auto* Owner{Component.GetOwner()};
    if (Owner == nullptr) {
        return nullptr;
    }

    auto [Instance, Added]{mInstances.try_emplace(&Component)};
    auto& Value{Instance->second};
    if (!Added) {
        return &Value;
    }
    Value.mPath = Path;
    Value.mComponent = CoreUObject::MakeObjectRef(&Component);
    Value.mOwner = CoreUObject::MakeObjectRef(Owner);

    auto [Source, FirstUse]{mSources.try_emplace(Path)};
    if (FirstUse) {
        mPathTracker.Track(Path);
        Source->second.mChunk = LoadChunk(Path);
    }
    ++Source->second.mUsers;
    Value.mCollisionHandle = Owner->GetOnComponentCollision().Add([Target = &Value](Engine::AActor::EComponentCollision Kind, Engine::UPrimitiveComponent*, Engine::AActor* Other, Engine::UPrimitiveComponent*, const Engine::FHitResult* Hit) {
        if (Target->mRunning) {
            Target->mEvents.push_back({Kind, CoreUObject::MakeObjectRef(Other), Hit != nullptr ? *Hit : Engine::FHitResult{}});
        }
    });
    FLuaExecution Execution{};
    if (CreateExecution(Value, Source->second.mChunk, Execution)) {
        Value.mExecution = std::move(Execution);
        Value.mRunning = true;
        Value.mRunning = Value.mExecution.Invoke(ELuaCallback::BeginPlay);
    }
    return &Value;
}

void FLuaRuntime::Tick(Engine::FLuaScriptInstance& Instance, float DeltaTime) {
    if (!Instance.mRunning) {
        return;
    }

    auto& Execution{Instance.mExecution};
    auto Events{std::move(Instance.mEvents)};
    Instance.mEvents.clear();
    for (const auto& Event : Events) {
        switch (Event.mKind) {
        case Engine::AActor::EComponentCollision::BeginOverlap:
            Instance.mRunning = Execution.Invoke(ELuaCallback::Overlap, FActorRef{Event.mOther});
            break;
        case Engine::AActor::EComponentCollision::EndOverlap:
            Instance.mRunning = Execution.Invoke(ELuaCallback::EndOverlap, FActorRef{Event.mOther});
            break;
        case Engine::AActor::EComponentCollision::Hit:
            Instance.mRunning = Execution.Invoke(ELuaCallback::Hit, FActorRef{Event.mOther}, Engine::FHitResult{Event.mHit});
            break;
        }
        if (!Instance.mRunning) {
            return;
        }
    }

    Instance.mRunning = Execution.Invoke(ELuaCallback::Tick, DeltaTime) && Execution.AdvanceTasks(DeltaTime);
}

void FLuaRuntime::Stop(Engine::ULuaScriptComponent& Component, Engine::FLuaScriptInstance& Instance) {
    Instance.DisconnectCollision();
    Instance.mExecution.Invoke(ELuaCallback::EndPlay);

    const auto Source{mSources.find(Instance.mPath)};
    if (--Source->second.mUsers == 0) {
        mPathTracker.Untrack(Source->first);
        mSources.erase(Source);
    }
    mInstances.erase(&Component);
}

void FLuaRuntime::ReloadChangedScripts() {
    for (const auto& Path : mPathTracker.TakeChanges()) {
        if (const auto Source{mSources.find(Path)}; Source != mSources.end()) {
            Reload(Source->first, Source->second);
        }
    }
}

void FLuaRuntime::SetFrameInput(const Core::FInput& Input) {
    mFrameInput = Input;
}

void FLuaRuntime::ClearFrameInput() {
    mFrameInput = {};
}

sol::protected_function FLuaRuntime::LoadChunk(const std::filesystem::path& Path) {
    return mLuaVM.LoadChunk(Path, [&Path](const char* Stage, std::string_view Message) {
        Report(Path, nullptr, Stage, Message);
    });
}

bool FLuaRuntime::CreateExecution(const Engine::FLuaScriptInstance& Instance, const sol::protected_function& Chunk, FLuaExecution& Execution) {
    const auto BindActor{[&Instance](sol::environment& Environment) {
        Environment["Actor"] = FActorRef{Instance.mOwner};
        // 스크립트를 실행하는 컴포넌트 자신입니다. Tick 그룹 변경 등에 사용합니다.
        Environment["Script"] = FLuaComponentRef{Instance.mComponent.Get()};
    }};
    const auto ReportError{[&Instance](const char* Stage, std::string_view Message) {
        Report(Instance.mPath, Instance.mOwner.Get(), Stage, Message);
    }};

    return mLuaVM.CreateExecution(Chunk, BindActor, ReportError, Execution);
}

void FLuaRuntime::Reload(const std::filesystem::path& Path, FSource& Source) {
    auto Chunk{LoadChunk(Path)};
    if (!Chunk.valid()) {
        return;
    }

    std::vector<std::pair<Engine::FLuaScriptInstance*, FLuaExecution>> Replacements{};
    for (auto& Entry : mInstances) {
        auto& Instance{Entry.second};
        if (Instance.mPath != Path) {
            continue;
        }

        FLuaExecution Replacement{};
        if (!CreateExecution(Instance, Chunk, Replacement)) {
            return;
        }
        Replacements.emplace_back(&Instance, std::move(Replacement));
    }

    Source.mChunk = std::move(Chunk);
    for (auto& [Target, Replacement] : Replacements) {
        auto& Instance{*Target};
        auto& Execution{Instance.mExecution};
        Execution.Invoke(ELuaCallback::EndPlay);
        Execution = std::move(Replacement);
        Instance.mRunning = true;
        Instance.mRunning = Execution.Invoke(ELuaCallback::BeginPlay);
    }
}

void FLuaRuntime::Report(const std::filesystem::path& Path, const Engine::AActor* Owner, const char* Event, std::string_view Message) {
    auto Context{Core::FilePath::ToGenericUtf8(Path) + " [" + Event + "] "};
    if (Owner != nullptr) {
        Context += Owner->GetName() + ": ";
    }
    Context += Message;
    Core::ErrorHandler::Report("Lua", Context, Core::ErrorHandler::EErrorLevel::Warning);
}
