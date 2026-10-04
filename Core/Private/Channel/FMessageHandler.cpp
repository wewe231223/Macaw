#include "pch.h"
#include "Core/Channel/FMessageHandler.h"
#include "Core/Base/ErrorHandler.h"

bool FMessageHandler::Handles(const FMessageTypeInfo* Type) const noexcept {
    return mMessageType->IsExactlyA(Type);
}

const FMessageTypeInfo& FMessageHandler::GetMessageType() const noexcept {
    if (mMessageType == nullptr) {
        ErrorHandler::Report("FMessageHandler::GetMessageType", "The message handler has no registered message type.", ErrorHandler::EErrorLevel::Critical);
    }

    return *mMessageType;
}

void FMessageHandler::Invoke(const FMessage& Message) {
    if (!mFunction) {
        ErrorHandler::Report("FMessageHandler::Invoke", "The message handler has no callable function.", ErrorHandler::EErrorLevel::Critical);
    }

    mFunction(Message);
}
