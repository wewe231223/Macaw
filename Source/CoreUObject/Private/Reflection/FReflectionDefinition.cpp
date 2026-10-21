#include "FReflectionDefinition.h"

bool FReflectionDefinition::operator==(const FReflectionDefinition& Other) const {
    return mKind == Other.mKind && mNativeType == Other.mNativeType && mParent == Other.mParent && mFlags == Other.mFlags && mFactory == Other.mFactory && mClassBuilder == Other.mClassBuilder && mStructBuilder == Other.mStructBuilder && mEnumBuilder == Other.mEnumBuilder;
}
