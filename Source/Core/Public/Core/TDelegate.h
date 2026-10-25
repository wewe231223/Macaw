#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Core/Reflection/EPropertyFlags.h"

#include <functional>

// Delegate 객체 이름, 전체 Parameter를 매크로에 넘기면 해당 TDelegate 객체 생성

template <typename TSignature>
class TDelegate {
public:
    ReflectTypes(TDelegate);
    using HandlerType = std::function<TSignature>;

public:
    ReflectTemplate(TDelegate);
    // 일반 함수나 람다 바인딩, 등록 후 해당 Handle에 대한 번호 return
    Uint32 Add(const HandlerType& Handler);
    // 클래스 멤버 함수 바인딩, 람다로 handler 생성후 목록에 저장, 등록 후 해당 Handle에 대한 번호 return
    template <typename T, typename... Args>
    Uint32 AddDynamic(T* Instance, void (T::*Func)(Args...));
    // Handlers 목록에 있는 함수들 전부 실행
    // Todo : Instance가 중간에 Null이 된 경우 등 예외처리
    template <typename... Args>
    void Broadcast(Args&&... Arguments);
    // Handlers 목록에서 HandleNum을 기반으로 해당 Number의 함수만 호출
    // Todo : Instance가 중간에 Null이 된 경우 등 예외처리
    template <typename... Args>
    void Execute(Uint32 HandleNum, Args&&... Arguments);
    // Handlers 목록에서 HandleNum을 기반으로 해당 Number의 함수 삭제
    void RemoveHandler(Uint32 HandleNumber);

private:
    ReflectMembers();

private:
    TMap<Uint32, HandlerType> mHandlers{};
    ReflectProperty(mHandlers, "Handlers", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mCurrentHandleNum{};
    ReflectProperty(mCurrentHandleNum, "CurrentHandleNum", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

template <typename TSignature>
Uint32 TDelegate<TSignature>::Add(const HandlerType& Handler) {
    ++mCurrentHandleNum;
    mHandlers.emplace(mCurrentHandleNum, Handler);
    return mCurrentHandleNum;
}

template <typename TSignature>
template <typename T, typename... Args>
Uint32 TDelegate<TSignature>::AddDynamic(T* Instance, void (T::*Func)(Args...)) {
    const HandlerType Handler{[Instance, Func](Args... Arguments) {
        (Instance->*Func)(Arguments...);
    }};
    return Add(Handler);
}

template <typename TSignature>
template <typename... Args>
void TDelegate<TSignature>::Broadcast(Args&&... Arguments) {
    const TMap<Uint32, HandlerType> Handlers{mHandlers};
    for (const auto& Entry : Handlers) {
        Entry.second(Arguments...);
    }
}

template <typename TSignature>
template <typename... Args>
void TDelegate<TSignature>::Execute(Uint32 HandleNum, Args&&... Arguments) {
    const auto Position{mHandlers.find(HandleNum)};
    if (Position != mHandlers.end()) {
        const HandlerType Handler{Position->second};
        Handler(Arguments...);
    }
}

template <typename TSignature>
void TDelegate<TSignature>::RemoveHandler(Uint32 HandleNumber) {
    mHandlers.erase(HandleNumber);
}

#include "TDelegateReflection.generated.h"
