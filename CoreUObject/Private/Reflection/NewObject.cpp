#include "pch.h"
#include "CoreUObject/Reflection/NewObject.h"

namespace {
    thread_local UClass* ConstructingClass{};
    thread_local bool ConstructingDefault{};
}

FObjectInitializer::FObjectInitializer(UClass* Class, bool ClassDefaultObject)
	: mPreviousClass(ConstructingClass),
	  mPreviousDefault(ConstructingDefault) {
    ConstructingDefault = ClassDefaultObject || (ConstructingClass == Class && ConstructingDefault);
    ConstructingClass = Class;
}

FObjectInitializer::~FObjectInitializer() {
    ConstructingClass = mPreviousClass;
    ConstructingDefault = mPreviousDefault;
}

UClass* FObjectInitializer::GetConstructingClass() {
    return ConstructingClass;
}

bool FObjectInitializer::IsConstructingDefault() {
    return ConstructingDefault;
}
