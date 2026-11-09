/**
 * @file UScriptCameraModifier.h
 * @brief ModifyCamera를 C# ScriptCameraModifier 인스턴스에 맡기는 카메라 모디파이어입니다.
 * @ingroup Script
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Engine/UCameraModifier.h"

#include <cstdint>

#include "UScriptCameraModifier.generated.h"

namespace Script {

    /**
     * @brief ModifyCamera를 C# ScriptCameraModifier 인스턴스에 맡기는 카메라 모디파이어입니다.
     * @details C#에서 PlayerCameraManager.AddCameraModifier&lt;T&gt;()를 부르면 만들어집니다. 카메라 관리자에
     *          추가될 때 C# 인스턴스를 만들고, 해제될 때 함께 해제합니다.
     * @ingroup Script
     */
    UCLASS(NoFactory)
    class UScriptCameraModifier final : public Engine::UCameraModifier {
        GENERATED_BODY()

    public:
        /**
         * @brief 연결할 C# 클래스 이름으로 모디파이어를 구성합니다.
         * @param[in] ClassName 네임스페이스를 포함한 ScriptCameraModifier 파생 클래스 이름입니다.
         */
        explicit UScriptCameraModifier(Core::FString ClassName = {});

        /**
         * @brief 남아 있는 C# 인스턴스를 해제합니다.
         */
        ~UScriptCameraModifier() override;

    public:
        /**
         * @brief C# 인스턴스를 만들고 AddedToCamera를 전달합니다.
         * @param[in] CameraManager 이 모디파이어를 소유할 카메라 관리자입니다.
         * @return 인스턴스를 만들었으면 true입니다.
         */
        bool AddedToCamera(Engine::APlayerCameraManager* CameraManager) override;

        /**
         * @brief C# 인스턴스의 ModifyCamera를 호출합니다.
         * @param[in] DeltaTime 이번 갱신의 초 단위 시간입니다.
         * @param[in,out] ViewInfo 고칠 시점입니다.
         * @return 뒤 순서의 모디파이어를 건너뛰려면 true입니다.
         */
        bool ModifyCamera(float DeltaTime, Engine::FMinimalViewInfo& ViewInfo) override;

    private:
        /**
         * @brief 네임스페이스를 포함한 C# 클래스 이름입니다.
         */
        Core::FString mClassName{};

        /**
         * @brief C# 인스턴스의 GCHandle이며 만들기 전이면 0입니다.
         */
        std::intptr_t mInstance{};
    };

}
