#include "pch.h"
#include "Core/Base/FScopedFlag.h"

FScopedFlag::FScopedFlag(bool& Flag, bool Value)
	: mFlag{Flag},
	  mPreviousValue{Flag} {
    mFlag = Value;
}

FScopedFlag::~FScopedFlag() {
    mFlag = mPreviousValue;
}
