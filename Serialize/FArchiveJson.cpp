#include "pch.h"
#include "FArchiveJson.h"

FArchiveJson::FArchiveJson(rapidjson::Value& RootNode, rapidjson::Document::AllocatorType& InAllocator)
    : FArchive(EArchiveMode::Saving),
      mAllocator(&InAllocator) {
    mNodeStack.push_back(&RootNode);
}

FArchiveJson::FArchiveJson(rapidjson::Value& RootNode)
    : FArchive(EArchiveMode::Loading),
      mAllocator(nullptr) {
    mNodeStack.push_back(&RootNode);
}

// ---------------------------------------------------------
// Primitives
// ---------------------------------------------------------
#define IMPLEMENT_JSON_PRIMITIVE(Type, RapidIsFunc, RapidGetFunc)      \
    void FArchiveJson::Serialize(std::string_view Name, Type& Value) { \
        rapidjson::Value* Current = mNodeStack.back();                 \
        if (!Current)                                                  \
            return;                                                    \
        if (IsSaving()) {                                              \
            rapidjson::Value JsonVal(Value);                           \
            AddChildNode(Current, Name, JsonVal);                      \
        } else {                                                       \
            rapidjson::Value* Child = GetChildNode(Current, Name);     \
            if (Child && Child->RapidIsFunc())                         \
                Value = static_cast<Type>(Child->RapidGetFunc());      \
        }                                                              \
    }

IMPLEMENT_JSON_PRIMITIVE(bool, IsBool, GetBool)
IMPLEMENT_JSON_PRIMITIVE(Int32, IsInt, GetInt)
IMPLEMENT_JSON_PRIMITIVE(Uint32, IsUint, GetUint)
IMPLEMENT_JSON_PRIMITIVE(Int64, IsInt64, GetInt64)
IMPLEMENT_JSON_PRIMITIVE(Uint64, IsUint64, GetUint64)
// float32는 RapidJSON에서 Double로 다루는 것이 안전하므로 Number/Double로 매핑
IMPLEMENT_JSON_PRIMITIVE(Float32, IsNumber, GetDouble)
IMPLEMENT_JSON_PRIMITIVE(Float64, IsDouble, GetDouble)
#undef IMPLEMENT_JSON_PRIMITIVE

void FArchiveJson::Serialize(std::string_view Name, Uint8& Value) {
    Uint32 Temp{Value};
    Serialize(Name, Temp);
    if (IsLoading())
        Value = static_cast<Uint8>(Temp);
}

// ---------------------------------------------------------
// Engine Core
// ---------------------------------------------------------
void FArchiveJson::Serialize(std::string_view Name, FString& Value) {
    rapidjson::Value* Current{mNodeStack.back()};
    if (!Current)
        return;

    if (IsSaving()) {
        rapidjson::Value JsonVal{Value.c_str(), static_cast<Uint32>(Value.size()), *mAllocator};
        AddChildNode(Current, Name, JsonVal);
    } else {
        rapidjson::Value* Child{GetChildNode(Current, Name)};
        if (Child && Child->IsString())
            Value = Child->GetString();
    }
}

void FArchiveJson::Serialize(std::string_view Name, FGuid& Value) {
    FString GuidStr{Value.ToString()};
    Serialize(Name, GuidStr);
    if (IsLoading())
        Value.Parse(GuidStr);
}

// ---------------------------------------------------------
// Math Types
// ---------------------------------------------------------
void FArchiveJson::Serialize(std::string_view Name, FVector2D& Value) {
    rapidjson::Value* Current{mNodeStack.back()};
    if (!Current)
        return;

    if (IsSaving()) {
        rapidjson::Value ArrayVal{rapidjson::kArrayType};
        ArrayVal.PushBack(Value.mX, *mAllocator) .PushBack(Value.mY, *mAllocator);
        AddChildNode(Current, Name, ArrayVal);
    } else {
        rapidjson::Value* Child{GetChildNode(Current, Name)};
        if (Child && Child->IsArray() && Child->Size() >= 2) {
            Value.mX = (*Child)[0].GetFloat();
            Value.mY = (*Child)[1].GetFloat();
        }
    }
}

void FArchiveJson::Serialize(std::string_view Name, FVector3& Value) {
    rapidjson::Value* Current{mNodeStack.back()};
    if (!Current)
        return;

    if (IsSaving()) {
        rapidjson::Value ArrayVal{rapidjson::kArrayType};
        ArrayVal.PushBack(Value.mX, *mAllocator) .PushBack(Value.mY, *mAllocator) .PushBack(Value.mZ, *mAllocator);
        AddChildNode(Current, Name, ArrayVal);
    } else {
        rapidjson::Value* Child{GetChildNode(Current, Name)};
        if (Child && Child->IsArray() && Child->Size() >= 3) {
            Value.mX = (*Child)[0].GetFloat();
            Value.mY = (*Child)[1].GetFloat();
            Value.mZ = (*Child)[2].GetFloat();
        }
    }
}

void FArchiveJson::Serialize(std::string_view Name, FVector4& Value) {
    rapidjson::Value* Current{mNodeStack.back()};
    if (!Current)
        return;

    if (IsSaving()) {
        rapidjson::Value ArrayVal{rapidjson::kArrayType};
        ArrayVal.PushBack(Value.mX, *mAllocator) .PushBack(Value.mY, *mAllocator) .PushBack(Value.mZ, *mAllocator) .PushBack(Value.mW, *mAllocator);
        AddChildNode(Current, Name, ArrayVal);
    } else {
        rapidjson::Value* Child{GetChildNode(Current, Name)};
        if (Child && Child->IsArray() && Child->Size() >= 4) {
            Value.mX = (*Child)[0].GetFloat();
            Value.mY = (*Child)[1].GetFloat();
            Value.mZ = (*Child)[2].GetFloat();
            Value.mW = (*Child)[3].GetFloat();
        }
    }
}

void FArchiveJson::Serialize(std::string_view Name, FQuat& Value) {
    FVector4 Temp{Value.mX, Value.mY, Value.mZ, Value.mW};

    // FVector4  재사용
    Serialize(Name, Temp);

    if (IsLoading()) {
        Value.mX = Temp.mX;
        Value.mY = Temp.mY;
        Value.mZ = Temp.mZ;
        Value.mW = Temp.mW;
    }
}

void FArchiveJson::Serialize(std::string_view Name, FMatrix& Value) {
    rapidjson::Value* Current{mNodeStack.back()};
    if (!Current)
        return;

    if (IsSaving()) {
        rapidjson::Value ArrayVal{rapidjson::kArrayType};

        for (int I{0}; I < 4; ++I) {
            for (int J{0}; J < 4; ++J) {
                ArrayVal.PushBack(Value.m_[I][J], *mAllocator);
            }
        }
        AddChildNode(Current, Name, ArrayVal);
    } else {
        rapidjson::Value* Child{GetChildNode(Current, Name)};
        if (Child && Child->IsArray() && Child->Size() >= 16) {
            int Index{0};
            for (int I{0}; I < 4; ++I) {
                for (int J{0}; J < 4; ++J) {
                    Value.m_[I][J] = (*Child)[Index++].GetFloat();
                }
            }
        }
    }
}

// ---------------------------------------------------------
// Scope
// ---------------------------------------------------------
void FArchiveJson::BeginObjectScope(std::string_view Name) {
    rapidjson::Value* Current{mNodeStack.back()};
    if (!Current) {
        mNodeStack.push_back(nullptr);
        return;
    }

    if (IsSaving()) {
        rapidjson::Value NewObj{rapidjson::kObjectType};
        AddChildNode(Current, Name, NewObj);

        if (Current->IsObject()) {
            std::string KeyStr{Name};
            mNodeStack.push_back(&(*Current)[KeyStr.c_str()]);
        } else if (Current->IsArray()) {
            mNodeStack.push_back(&(*Current)[Current->Size() - 1]);
        }
    } else if (IsLoading()) {
        rapidjson::Value* Child{GetChildNode(Current, Name)};
        if (Child && Child->IsObject())
            mNodeStack.push_back(Child);
        else
            mNodeStack.push_back(nullptr);
    }
}

void FArchiveJson::BeginArrayScope(std::string_view Name, std::size_t& ArraySize) {
    rapidjson::Value* Current{mNodeStack.back()};
    if (!Current) {
        mNodeStack.push_back(nullptr);
        return;
    }

    if (IsSaving()) {
        rapidjson::Value NewArray{rapidjson::kArrayType};
        AddChildNode(Current, Name, NewArray);

        if (Current->IsObject()) {
            std::string KeyStr{Name};
            mNodeStack.push_back(&(*Current)[KeyStr.c_str()]);
        } else if (Current->IsArray()) {
            mNodeStack.push_back(&(*Current)[Current->Size() - 1]);
        }
    } else if (IsLoading()) {
        rapidjson::Value* Child{GetChildNode(Current, Name)};
        if (Child && Child->IsArray()) {
            ArraySize = Child->Size();
            mNodeStack.push_back(Child);
        } else {
            ArraySize = 0;
            mNodeStack.push_back(nullptr);
        }
    }
}

void FArchiveJson::AddChildNode(rapidjson::Value* Parent, std::string_view Name, rapidjson::Value& Child) {
    if (Parent->IsObject()) {
        rapidjson::Value JsonKey{Name.data(), static_cast<Uint32>(Name.size()), *mAllocator};
        Parent->AddMember(JsonKey, Child, *mAllocator);
    } else if (Parent->IsArray()) {
        Parent->PushBack(Child, *mAllocator);
    }
}

rapidjson::Value* FArchiveJson::GetChildNode(rapidjson::Value* Parent, std::string_view Name) {
    if (Parent->IsObject()) {
        std::string KeyStr{Name};
        if (Parent->HasMember(KeyStr.c_str())) {
            return &(*Parent)[KeyStr.c_str()];
        }
    } else if (Parent->IsArray()) {
        std::size_t Index{std::stoull(std::string(Name))};
        if (Index < Parent->Size()) {
            return &(*Parent)[static_cast<rapidjson::SizeType>(Index)];
        }
    }
    return nullptr;
}

void FArchiveJson::EndObjectScope() {
    if (!mNodeStack.empty())
        mNodeStack.pop_back();
}

void FArchiveJson::EndArrayScope() {
    if (!mNodeStack.empty())
        mNodeStack.pop_back();
}
