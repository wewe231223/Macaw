#include "Script/FScriptInput.h"

namespace Script {

    namespace {
        const Core::FInput* sInput{};
    }

    void Script::FScriptInput::SetSource(const Core::FInput* Input) {
        sInput = Input;
    }

    bool Script::FScriptInput::IsDown(Core::EKey Key) {
        return sInput != nullptr && sInput->IsDown(Key, Core::EInputRoute::Game);
    }

    bool Script::FScriptInput::WasPressed(Core::EKey Key) {
        return sInput != nullptr && sInput->WasPressed(Key, Core::EInputRoute::Game);
    }

    bool Script::FScriptInput::WasReleased(Core::EKey Key) {
        return sInput != nullptr && sInput->WasReleased(Key, Core::EInputRoute::Game);
    }

    Core::FVector2 Script::FScriptInput::GetMousePosition() {
        return sInput != nullptr ? sInput->GetMousePosition(Core::EInputRoute::Game) : Core::FVector2{};
    }

    Core::FVector2 Script::FScriptInput::GetMouseDelta() {
        return sInput != nullptr ? sInput->GetMouseDelta(Core::EInputRoute::Game) : Core::FVector2{};
    }

    float Script::FScriptInput::GetMouseWheel() {
        return sInput != nullptr ? sInput->GetMouseWheel(Core::EInputRoute::Game) : 0.0f;
    }

}
