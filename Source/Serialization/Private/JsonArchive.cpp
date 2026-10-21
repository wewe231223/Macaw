#include "Serialization/JsonArchive.h"
#include "Core/Archive.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <charconv>
#include <cmath>
#include <fstream>
#include <limits>
#include <unordered_set>

namespace {
    bool ValidateJson(const rapidjson::Value& Value, Uint32 Depth) {
        if (Depth > 32) {
            return false;
        }

        if (Value.IsObject()) {
            std::unordered_set<FString> Names{};

            for (auto Position{Value.MemberBegin()}; Position != Value.MemberEnd(); ++Position) {
                if (!Names.emplace(Position->name.GetString(), Position->name.GetStringLength()).second || !ValidateJson(Position->value, Depth + 1)) {
                    return false;
                }
            }
        } else if (Value.IsArray()) {
            if (Value.Size() > 100000) {
                return false;
            }

            for (const auto& Child : Value.GetArray()) {
                if (!ValidateJson(Child, Depth + 1)) {
                    return false;
                }
            }
        }

        return true;
    }
}

struct FJsonArchive::FImplementation {
private:
    friend class FJsonArchive;

public:
    FImplementation();

public:
    rapidjson::Value* Find(std::string_view Name, bool Create);

private:
    rapidjson::Document mDocument{};
    TArray<rapidjson::Value*> mStack{};
};

FJsonArchive::FImplementation::FImplementation() {
    mDocument.SetObject();
    mStack.push_back(&mDocument);
}

rapidjson::Value* FJsonArchive::FImplementation::Find(std::string_view Name, bool Create) {
    rapidjson::Value& Parent{*mStack.back()};

    if (Parent.IsObject()) {
        const auto Position{Parent.FindMember(rapidjson::StringRef(Name.data(), static_cast<rapidjson::SizeType>(Name.size())))};

        if (Position != Parent.MemberEnd()) {
            return &Position->value;
        }

        if (Create) {
            rapidjson::Value Key{};
            Key.SetString(Name.data(), static_cast<rapidjson::SizeType>(Name.size()), mDocument.GetAllocator());

            Parent.AddMember(Key, rapidjson::Value{}, mDocument.GetAllocator());

            return &Parent.MemberEnd()[-1].value;
        }
    } else if (Parent.IsArray()) {
        Uint32 Index{};
        const auto Parsed{std::from_chars(Name.data(), Name.data() + Name.size(), Index)};

        if (Parsed.ec != std::errc{} || Parsed.ptr != Name.data() + Name.size()) {
            return nullptr;
        }

        if (Create && Index == Parent.Size()) {
            Parent.PushBack(rapidjson::Value{}, mDocument.GetAllocator());
        }

        if (Index < Parent.Size()) {
            return &Parent[Index];
        }
    }

    return nullptr;
}

FJsonArchive::FJsonArchive(EArchiveMode Mode)
	: FArchive{Mode},
	  mImplementation{std::make_unique<FImplementation>()} {
}

FJsonArchive::~FJsonArchive() = default;

bool FJsonArchive::Load(const std::filesystem::path& Path) {
    std::ifstream Input{Path, std::ios::binary | std::ios::ate};

    if (!Input.is_open() || Input.tellg() < 0 || Input.tellg() > 16 * 1024 * 1024) {
        SetError("Cannot open scene or scene exceeds 16 MiB");

        return false;
    }

    FString Text{};
    Text.resize(static_cast<std::size_t>(Input.tellg()));
    Input.seekg(0);
    Input.read(Text.data(), static_cast<std::streamsize>(Text.size()));

    if (!Input.good() && !Input.eof()) {
        SetError("Scene read failed");

        return false;
    }

    return Parse(Text);
}

bool FJsonArchive::Save(const std::filesystem::path& Path) {
    if (IsLoading() || HasError() || Path.empty()) {
        return false;
    }

    std::error_code Error{};
    const std::filesystem::path Target{std::filesystem::absolute(Path, Error)};

    if (Error) {
        SetError("Invalid save path");

        return false;
    }

    std::filesystem::create_directories(Target.parent_path(), Error);

    if (Error) {
        SetError("Cannot create scene directory");

        return false;
    }

    std::filesystem::path Temporary{Target};
    Temporary += "." + FGuid::NewGuid().ToString() + ".tmp";

    const FString Text{ToString()};
    {
        std::ofstream Output{Temporary, std::ios::binary | std::ios::trunc};
        Output.write(Text.data(), static_cast<std::streamsize>(Text.size()));
        Output.flush();

        if (!Output.good()) {
            Output.close();
            std::filesystem::remove(Temporary, Error);
            SetError("Scene write failed");

            return false;
        }
    }

    std::filesystem::rename(Temporary, Target, Error);

    if (Error) {
        std::filesystem::remove(Temporary, Error);
        SetError("Cannot replace scene file");

        return false;
    }

    return true;
}

bool FJsonArchive::Parse(const FString& Text) {
    if (!IsLoading()) {
        SetError("Archive is not loading");

        return false;
    }

    mImplementation->mDocument.Parse(Text.data(), Text.size());

    if (mImplementation->mDocument.HasParseError()) {
        SetError(FString{"JSON: "} + rapidjson::GetParseError_En(mImplementation->mDocument.GetParseError()));

        return false;
    }

    if (!mImplementation->mDocument.IsObject() || !ValidateJson(mImplementation->mDocument, 0)) {
        SetError("Invalid scene structure or duplicate JSON field");

        return false;
    }

    mImplementation->mStack = {&mImplementation->mDocument};

    return true;
}

FString FJsonArchive::ToString() const {
    rapidjson::StringBuffer Buffer{};
    rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer{Buffer};

    mImplementation->mDocument.Accept(Writer);

    return FString{Buffer.GetString(), Buffer.GetSize()};
}

bool FJsonArchive::HasField(std::string_view Name) const {
    return mImplementation->Find(Name, false) != nullptr;
}

bool FJsonArchive::BeginObject(std::string_view Name) {
    if (HasError()) {
        return false;
    }

    rapidjson::Value* Value{mImplementation->Find(Name, !IsLoading())};

    if (Value == nullptr || (IsLoading() && !Value->IsObject())) {
        SetError("Expected object: " + FString{Name});

        return false;
    }

    if (!IsLoading()) {
        Value->SetObject();
    }

    mImplementation->mStack.push_back(Value);

    return true;
}

void FJsonArchive::EndObject() {
    if (mImplementation->mStack.size() > 1) {
        mImplementation->mStack.pop_back();
    }
}

bool FJsonArchive::BeginArray(std::string_view Name, Uint32& Count) {
    if (HasError()) {
        return false;
    }

    rapidjson::Value* Value{mImplementation->Find(Name, !IsLoading())};

    if (Value == nullptr || (IsLoading() && !Value->IsArray())) {
        SetError("Expected array: " + FString{Name});

        return false;
    }

    if (IsLoading()) {
        Count = Value->Size();
    } else {
        Value->SetArray();
    }

    mImplementation->mStack.push_back(Value);

    return true;
}

void FJsonArchive::EndArray() {
    EndObject();
}

void FJsonArchive::Serialize(std::string_view Name, bool& Value) {
    rapidjson::Value* Field{mImplementation->Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && !Field->IsBool())) {
        SetError("Expected bool: " + FString{Name});
    } else if (IsLoading()) {
        Value = Field->GetBool();
    } else {
        Field->SetBool(Value);
    }
}

void FJsonArchive::Serialize(std::string_view Name, Int32& Value) {
    rapidjson::Value* Field{mImplementation->Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && !Field->IsInt())) {
        SetError("Expected int32: " + FString{Name});
    } else if (IsLoading()) {
        Value = Field->GetInt();
    } else {
        Field->SetInt(Value);
    }
}

void FJsonArchive::Serialize(std::string_view Name, Uint32& Value) {
    rapidjson::Value* Field{mImplementation->Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && !Field->IsUint())) {
        SetError("Expected uint32: " + FString{Name});
    } else if (IsLoading()) {
        Value = Field->GetUint();
    } else {
        Field->SetUint(Value);
    }
}

void FJsonArchive::Serialize(std::string_view Name, float& Value) {
    rapidjson::Value* Field{mImplementation->Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && (!Field->IsNumber() || !std::isfinite(Field->GetDouble()) || std::abs(Field->GetDouble()) > std::numeric_limits<float>::max()))) {
        SetError("Expected finite float: " + FString{Name});
    } else if (IsLoading()) {
        Value = Field->GetFloat();
    } else if (!std::isfinite(Value)) {
        SetError("Cannot save non-finite float: " + FString{Name});
    } else {
        Field->SetFloat(Value);
    }
}

void FJsonArchive::Serialize(std::string_view Name, FString& Value) {
    rapidjson::Value* Field{mImplementation->Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && (!Field->IsString() || Field->GetStringLength() > 4096))) {
        SetError("Expected string: " + FString{Name});
    } else if (IsLoading()) {
        Value.assign(Field->GetString(), Field->GetStringLength());
    } else {
        Field->SetString(Value.data(), static_cast<rapidjson::SizeType>(Value.size()), mImplementation->mDocument.GetAllocator());
    }
}
