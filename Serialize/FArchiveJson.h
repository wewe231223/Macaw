#pragma once
#include "FArchive.h"
#include <rapidjson/document.h>

class FArchiveJson : public FArchive {
public:
    FArchiveJson(rapidjson::Value& RootNode, rapidjson::Document::AllocatorType& InAllocator);
    FArchiveJson(rapidjson::Value& RootNode);

    // Primitives
    virtual void Serialize(std::string_view Name, bool& Value) override;
    virtual void Serialize(std::string_view Name, Uint8& Value) override;
    virtual void Serialize(std::string_view Name, Int32& Value) override;
    virtual void Serialize(std::string_view Name, Uint32& Value) override;
    virtual void Serialize(std::string_view Name, Int64& Value) override;
    virtual void Serialize(std::string_view Name, Uint64& Value) override;
    virtual void Serialize(std::string_view Name, Float32& Value) override;
    virtual void Serialize(std::string_view Name, Float64& Value) override;

    // Engine Core
    virtual void Serialize(std::string_view Name, FString& Value) override;
    virtual void Serialize(std::string_view Name, FGuid& Value) override;

    // Math Types
    virtual void Serialize(std::string_view Name, FVector2D& Value) override;
    virtual void Serialize(std::string_view Name, FVector3& Value) override;
    virtual void Serialize(std::string_view Name, FVector4& Value) override;
    virtual void Serialize(std::string_view Name, FQuat& Value) override;
    virtual void Serialize(std::string_view Name, FMatrix& Value) override;

    // Scope
    virtual void BeginObjectScope(std::string_view Name) override;

    virtual void EndObjectScope() override;

    // Array
    virtual void BeginArrayScope(std::string_view Name, std::size_t& ArraySize) override;

    virtual void EndArrayScope() override;

private:
    // called when saving in progress
    void AddChildNode(rapidjson::Value* Parent, std::string_view Name, rapidjson::Value& Child);

    // called when loading in progress
    rapidjson::Value* GetChildNode(rapidjson::Value* Parent, std::string_view Name);

private:
    rapidjson::Document::AllocatorType* mAllocator{};
    TArray<rapidjson::Value*> mNodeStack{};
};
