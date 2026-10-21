#include "CoreUObject/Reflection/FProperty.h"
#include "Core/Archive.h"
#include "CoreUObject/Reflection/FEnumValue.h"
#include "CoreUObject/Reflection/EPropertyType.h"
#include "CoreUObject/Reflection/EPropertyFlags.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <utility>

FProperty::FProperty(FString Name, EPropertyType Type, EPropertyFlags Flags, FReader Reader, FWriter Writer, const UStruct* Struct, TArray<FEnumValue> Values, FWriter Loader)
	: mName{std::move(Name)},
	  mType{Type},
	  mFlags{Flags},
	  mReader{Reader},
	  mWriter{Writer},
	  mLoader{Loader != nullptr ? Loader : Writer},
	  mStruct{Struct},
	  mEnumValues{std::move(Values)} {
}

const FString& FProperty::GetName() const {
    return mName;
}

EPropertyType FProperty::GetType() const {
    return mType;
}

bool FProperty::HasFlag(EPropertyFlags Flag) const {
    return (static_cast<Uint32>(mFlags) & static_cast<Uint32>(Flag)) != 0;
}

const UStruct* FProperty::GetStruct() const {
    return mStruct;
}

const TArray<FEnumValue>& FProperty::GetEnumValues() const {
    return mEnumValues;
}

FPropertyValue FProperty::ReadValue(const void* Container) const {
    return mReader(Container);
}

bool FProperty::WriteValue(void* Container, const FPropertyValue& Value) const {
    return WriteValue(Container, Value, mWriter);
}

bool FProperty::WriteValue(void* Container, const FPropertyValue& Value, FWriter Writer) const {
    if (Writer == nullptr || Container == nullptr) {
        return false;
    }

    if (mType == EPropertyType::Enum) {
        const Int32* Number{std::get_if<Int32>(&Value)};

        if (Number == nullptr || std::none_of(mEnumValues.begin(), mEnumValues.end(), [Number](const FEnumValue& Entry) {
            return Entry.mValue == *Number;
        })) {
            return false;
        }
    }

    return Writer(Container, Value);
}

void FProperty::SerializeValue(void* Container, FArchive& Archive) const {
    if (!HasFlag(EPropertyFlags::Serialize) || HasFlag(EPropertyFlags::Transient) || Archive.HasError() || (Archive.IsLoading() && !Archive.HasField(mName))) {
        return;
    }

    FPropertyValue Value{ReadValue(Container)};

    if (mStruct != nullptr) {
        if (Archive.BeginObject(mName)) {
            mStruct->SerializeProperties(GetPropertyData(Value), Archive);

            Archive.EndObject();
        }
    } else {
        std::visit([&Archive, this](auto& Typed) {
            Archive.Serialize(mName, Typed);
        }, Value);
    }

    if (Archive.IsLoading() && !Archive.HasError() && !WriteValue(Container, Value, mLoader)) {
        Archive.SetError("Property validation failed: " + mName);
    }
}
