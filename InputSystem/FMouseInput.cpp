#include "PCH.h"
#include "FMouseInput.h"
#include "Core/Channel/FMessageChannel.h"


void FMouseInput::Update()
{
    for (int i = 0; i < static_cast<int>(EMouseButton::Count); ++i)
    {
        if (RawButtonDown[i])
        {
            if (PrevButtonDown[i])
                ButtonStates[i] = EButtonState::Held;
            else
            {
                ButtonStates[i] = EButtonState::Pressed;
            }

        }
        else
        {
            if (PrevButtonDown[i])
            {
                ButtonStates[i] = EButtonState::Released;
            }
            else
                ButtonStates[i] = EButtonState::None;
        }

        PrevButtonDown[i] = RawButtonDown[i];

        if (ButtonStates[i] == EButtonState::Pressed)
        {
            InputSender.TryEmplace<FMouseInputMessage>(
                static_cast<EMouseButton>(i),
                ButtonStates[i],
                CurrentMouseX,
                CurrentMouseY
            );
        }
    }

    MouseDeltaX = CurrentMouseX - PrevMouseX;
    MouseDeltaY = CurrentMouseY - PrevMouseY;
    PrevMouseX = CurrentMouseX;
    PrevMouseY = CurrentMouseY;

    WheelDelta = RawWheelDelta;
    RawWheelDelta = 0;

}



const FTypeInfo& FMouseInputMessage::StaticTypeInfo()
{
    static FTypeInfo TypeInfo{ "FMouseInputMessage" };
    return TypeInfo;
}

FMouseInput::FMouseInput(FMessageChannel::FSender InInputSender) : InputSender(InInputSender)
{
}