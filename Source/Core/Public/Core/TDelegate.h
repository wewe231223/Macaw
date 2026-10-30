/**
 * @file TDelegate.h
 * @brief 핸들로 호출 함수를 등록·해제하고 인수를 전달하는 델리게이트입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Core/Reflection/EPropertyFlags.h"

#include <functional>

namespace Core {

    // Delegate 객체 이름, 전체 Parameter를 매크로에 넘기면 해당 TDelegate 객체 생성

    /**
     * @brief 핸들로 호출 함수를 등록·해제하고 인수를 전달하는 델리게이트입니다.
     */
    template <typename TSignature>
    class TDelegate {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(TDelegate);

        /**
         * @brief 등록 함수의 호출 시그니처입니다.
         */
        using HandlerType = std::function<TSignature>;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
         */
        ReflectTemplate(TDelegate);
        // 일반 함수나 람다 바인딩, 등록 후 해당 Handle에 대한 번호 return
        /**
         * @brief 호출 함수를 델리게이트에 추가하고 해제에 사용할 핸들을 반환합니다.
         * @param[in] Handler 등록할 호출 함수입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        Core::Uint32 Add(const HandlerType& Handler);
        // 클래스 멤버 함수 바인딩, 람다로 handler 생성후 목록에 저장, 등록 후 해당 Handle에 대한 번호 return
        /**
         * @brief 인스턴스의 멤버 함수를 델리게이트에 바인딩합니다.
         * @tparam T 호출 시그니처 또는 인수 타입입니다.
         * @tparam Args 호출 시그니처 또는 인수 타입입니다.
         * @param[in] Instance 멤버 함수의 대상 인스턴스이며 호출 시까지 살아 있어야 합니다.
         * @param[in] Func 등록할 멤버 함수 포인터입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        template <typename T, typename... Args>
        Core::Uint32 AddDynamic(T* Instance, void (T::*Func)(Args...));
        // Handlers 목록에 있는 함수들 전부 실행
        // Todo : Instance가 중간에 Null이 된 경우 등 예외처리
        /**
         * @brief 등록 목록의 사본에 있는 모든 함수를 인수와 함께 호출합니다.
         * @tparam Args 호출 시그니처 또는 인수 타입입니다.
         * @param[in] Arguments 등록된 함수에 전달할 인수입니다.
         */
        template <typename... Args>
        void Broadcast(Args&&... Arguments);
        // Handlers 목록에서 HandleNum을 기반으로 해당 Number의 함수만 호출
        // Todo : Instance가 중간에 Null이 된 경우 등 예외처리
        /**
         * @brief 지정 핸들이 유효하면 해당 함수를 인수와 함께 호출합니다.
         * @tparam Args 호출 시그니처 또는 인수 타입입니다.
         * @param[in] HandleNum 호출할 델리게이트 핸들입니다.
         * @param[in] Arguments 등록된 함수에 전달할 인수입니다.
         */
        template <typename... Args>
        void Execute(Core::Uint32 HandleNum, Args&&... Arguments);
        // Handlers 목록에서 HandleNum을 기반으로 해당 Number의 함수 삭제
        /**
         * @brief 핸들에 해당하는 델리게이트 등록을 제거합니다.
         * @param[in] HandleNumber 제거할 델리게이트 핸들입니다.
         */
        void RemoveHandler(Core::Uint32 HandleNumber);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 등록 핸들별 호출 함수입니다.
         */
        Core::TMap<Core::Uint32, HandlerType> mHandlers{};
        ReflectProperty(mHandlers, "Handlers", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 가장 최근 발급한 델리게이트 핸들입니다.
         */
        Core::Uint32 mCurrentHandleNum{};
        ReflectProperty(mCurrentHandleNum, "CurrentHandleNum", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    template <typename TSignature>
    Core::Uint32 Core::TDelegate<TSignature>::Add(const HandlerType& Handler) {
        ++mCurrentHandleNum;
        mHandlers.emplace(mCurrentHandleNum, Handler);
        return mCurrentHandleNum;
    }

    template <typename TSignature>
    template <typename T, typename... Args>
    Core::Uint32 Core::TDelegate<TSignature>::AddDynamic(T* Instance, void (T::*Func)(Args...)) {
        const HandlerType Handler{[Instance, Func](Args... Arguments) {
            (Instance->*Func)(Arguments...);
        }};
        return Add(Handler);
    }

    template <typename TSignature>
    template <typename... Args>
    void Core::TDelegate<TSignature>::Broadcast(Args&&... Arguments) {
        const Core::TMap<Core::Uint32, HandlerType> Handlers{mHandlers};

        for (const auto& Entry : Handlers) {
            Entry.second(Arguments...);
        }
    }

    template <typename TSignature>
    template <typename... Args>
    void Core::TDelegate<TSignature>::Execute(Core::Uint32 HandleNum, Args&&... Arguments) {
        const auto Position{mHandlers.find(HandleNum)};

        if (Position != mHandlers.end()) {
            const HandlerType Handler{Position->second};
            Handler(Arguments...);
        }
    }

    template <typename TSignature>
    void Core::TDelegate<TSignature>::RemoveHandler(Core::Uint32 HandleNumber) {
        mHandlers.erase(HandleNumber);
    }

}

#include "TDelegateReflection.generated.h"
