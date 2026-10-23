/**
 * @file JsonArchive.cpp
 * @brief RapidJSON을 사용하는 파일 및 값 직렬화를 제공합니다.
 * @ingroup Serialization
 */

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

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 중첩 깊이를 제한하면서 JSON 값의 유한한 수와 지원 타입을 검사합니다.
     * @param[in] Value 입력 값입니다.
     * @param[in] Depth 현재 JSON 중첩 깊이입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
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

/**
 * @brief FJsonArchive의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FJsonArchive::FJsonArchive(EArchiveMode Mode)
	: FArchive{Mode} {
    mDocument.SetObject();
    mStack.push_back(&mDocument);
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FJsonArchive::~FJsonArchive() = default;

/**
 * @brief JSON 파일을 읽어 루트 문서를 교체합니다.
 */
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

/**
 * @brief JSON 문서를 파일로 저장합니다.
 */
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
    Temporary += "." + FGuid{}.ToString() + ".tmp";

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

/**
 * @brief JSON 텍스트의 구문과 지원 값 범위를 검사하여 문서로 읽습니다.
 */
bool FJsonArchive::Parse(const FString& Text) {
    if (!IsLoading()) {
        SetError("Archive is not loading");

        return false;
    }

    mDocument.Parse(Text.data(), Text.size());

    if (mDocument.HasParseError()) {
        SetError(FString{"JSON: "} + rapidjson::GetParseError_En(mDocument.GetParseError()));

        return false;
    }

    if (!mDocument.IsObject() || !ValidateJson(mDocument, 0)) {
        SetError("Invalid scene structure or duplicate JSON field");

        return false;
    }

    mStack = {&mDocument};

    return true;
}

/**
 * @brief JSON 문서를 사람이 읽을 수 있는 문자열로 출력합니다.
 */
FString FJsonArchive::ToString() const {
    rapidjson::StringBuffer Buffer{};
    rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer{Buffer};

    mDocument.Accept(Writer);

    return FString{Buffer.GetString(), Buffer.GetSize()};
}

/**
 * @brief 현재 컨테이너에 지정한 이름의 필드가 있는지 검사합니다.
 */
bool FJsonArchive::HasField(std::string_view Name) const {
    const rapidjson::Value& Parent{*mStack.back()};

    if (Parent.IsObject()) {
        return Parent.HasMember(rapidjson::StringRef(Name.data(), static_cast<rapidjson::SizeType>(Name.size())));
    }

    if (!Parent.IsArray()) {
        return false;
    }

    Uint32 Index{};
    const auto Parsed{std::from_chars(Name.data(), Name.data() + Name.size(), Index)};

    return Parsed.ec == std::errc{} && Parsed.ptr == Name.data() + Name.size() && Index < Parent.Size();
}

/**
 * @brief 이름으로 지정한 객체 컨테이너에 진입합니다.
 */
bool FJsonArchive::BeginObject(std::string_view Name) {
    if (HasError()) {
        return false;
    }

    rapidjson::Value* Value{Find(Name, !IsLoading())};

    if (Value == nullptr || (IsLoading() && !Value->IsObject())) {
        SetError("Expected object: " + FString{Name});

        return false;
    }

    if (!IsLoading()) {
        Value->SetObject();
    }

    mStack.push_back(Value);

    return true;
}

/**
 * @brief 현재 객체 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
 */
void FJsonArchive::EndObject() {
    if (mStack.size() > 1) {
        mStack.pop_back();
    }
}

/**
 * @brief 배열 컨테이너에 진입하고 로드 시 원소 개수를 읽습니다.
 */
bool FJsonArchive::BeginArray(std::string_view Name, Uint32& Count) {
    if (HasError()) {
        return false;
    }

    rapidjson::Value* Value{Find(Name, !IsLoading())};

    if (Value == nullptr || (IsLoading() && !Value->IsArray())) {
        SetError("Expected array: " + FString{Name});

        return false;
    }

    if (IsLoading()) {
        Count = Value->Size();
    } else {
        Value->SetArray();
    }

    mStack.push_back(Value);

    return true;
}

/**
 * @brief 현재 배열 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
 */
void FJsonArchive::EndArray() {
    EndObject();
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FJsonArchive::Serialize(std::string_view Name, bool& Value) {
    rapidjson::Value* Field{Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && !Field->IsBool())) {
        SetError("Expected bool: " + FString{Name});
    } else if (IsLoading()) {
        Value = Field->GetBool();
    } else {
        Field->SetBool(Value);
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FJsonArchive::Serialize(std::string_view Name, Int32& Value) {
    rapidjson::Value* Field{Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && !Field->IsInt())) {
        SetError("Expected int32: " + FString{Name});
    } else if (IsLoading()) {
        Value = Field->GetInt();
    } else {
        Field->SetInt(Value);
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FJsonArchive::Serialize(std::string_view Name, Uint32& Value) {
    rapidjson::Value* Field{Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && !Field->IsUint())) {
        SetError("Expected uint32: " + FString{Name});
    } else if (IsLoading()) {
        Value = Field->GetUint();
    } else {
        Field->SetUint(Value);
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FJsonArchive::Serialize(std::string_view Name, Uint64& Value) {
    rapidjson::Value* Field{Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && !Field->IsUint64())) {
        SetError("Expected uint64: " + FString{Name});
    } else if (IsLoading()) {
        Value = Field->GetUint64();
    } else {
        Field->SetUint64(Value);
    }
}

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FJsonArchive::Serialize(std::string_view Name, float& Value) {
    rapidjson::Value* Field{Find(Name, !IsLoading())};

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

/**
 * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
 */
void FJsonArchive::Serialize(std::string_view Name, FString& Value) {
    rapidjson::Value* Field{Find(Name, !IsLoading())};

    if (Field == nullptr || (IsLoading() && (!Field->IsString() || Field->GetStringLength() > 4096))) {
        SetError("Expected string: " + FString{Name});
    } else if (IsLoading()) {
        Value.assign(Field->GetString(), Field->GetStringLength());
    } else {
        Field->SetString(Value.data(), static_cast<rapidjson::SizeType>(Value.size()), mDocument.GetAllocator());
    }
}

/**
 * @brief 현재 JSON 컨테이너의 필드나 다음 배열 원소를 조회하고 필요하면 생성합니다.
 */
rapidjson::Value* FJsonArchive::Find(std::string_view Name, bool Create) {
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
