#include "FMessageHandler.h"

bool FMessageHandler::Handles(const FTypeInfo& Type) const noexcept {
	return mMessageType->isExactlyA(Type);
}

const FTypeInfo& FMessageHandler::GetMessageType() const noexcept {
    assert(mMessageType != nullptr);
    return *mMessageType;
}

void FMessageHandler::Invoke(const FMessage& Message) {
    assert(mFunction);
    mFunction(Message);
}